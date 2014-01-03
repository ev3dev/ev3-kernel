/*
 * FIQ backend for I2C bus driver for LEGO Mindstorms EV3
 * Copyright (C) 2013 David Lechner <david@lechnology.com>
 *
 * Based on davinci_iic.c from lms2012
 * The file does not contain a copyright, but comes from the LEGO Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * In order to acheive realtime I2C performance on the input ports, we use
 * a FIQ (fast interrupt). These interrupts can interrupt any regular IRQ,
 * which is what makes it possible to get the near-realtime performance,
 * but this also makes them dangerous, so be sure you know what you are
 * doing before making changes here.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <asm/fiq.h>
#include <mach/legoev3-fiq.h>

enum transfer_states {
	TRANSFER_IDLE,
	TRANSFER_START,
	TRANSFER_START2,
	TRANSFER_ADDR,
	TRANSFER_WRITE,
	TRANSFER_READ,
	TRANSFER_WBIT,
	TRANSFER_RBIT,
	TRANSFER_WACK,
	TRANSFER_RACK,
	TRANSFER_STOP,
	TRANSFER_STOP2,
	TRANSFER_STOP3,
	TRANSFER_RESTART,
	TRANSFER_WAIT,
	TRANSFER_COMPLETE,
};

enum fiq_timer_restart {
	FIQ_TIMER_NORESTART,
	FIQ_TIMER_RESTART,
};

enum fiq_i2c_pin {
	FIQ_I2C_PIN_SDA,
	FIQ_I2C_PIN_SCL,
	NUM_FIQ_I2C_PIN
};

struct legoev3_fiq_gpio {
	int dir_reg;
	int set_reg;
	int clr_reg;
	int in_reg;
	int reg_mask;
};

struct legoev3_fiq_port_data {
	struct legoev3_fiq_gpio gpio[NUM_FIQ_I2C_PIN];
	void (*complete)(int, void *);
	void *context;
	struct i2c_msg msgs[2];
	struct i2c_msg *xfer_msgs;
	unsigned num_msg;
	unsigned cur_msg;
	unsigned wait_cycles;
	int xfer_result;
	u16 buf_offset;
	u8 bit_mask;
	u8 data_byte;
	enum transfer_states transfer_state;
	unsigned clock_state:1;
	unsigned nacked:1;
};

struct legoev3_fiq_data {
	struct legoev3_fiq_port_data port_data[LEGOEV3_NUM_PORT_IN];
	struct irq_chip *irq_chip;
	struct platform_device *pdev;
	struct fiq_handler fiq_handler;
	struct legoev3_fiq_gpio status_gpio;
	void __iomem *gpio_base;
	void __iomem *intc_base;
	int timer_irq;
	int status_gpio_irq;
	int port_req_flags;
};

static struct legoev3_fiq_data legoev3_fiq_init_data = {
	.fiq_handler = {
		.name = "legoev3-fiq-handler",
	},
};

struct legoev3_fiq_data *legoev3_fiq_data;

/*
 * IMPORTANT: The following functions called from an fiq (fast interrupt).
 * They cannot call any linux API functions. This is why we have our own
 * gpio and fiq ack functions.
 *
 * See set_fiq_c_handler in arch/arm/kernel/fiq.c for more details on
 * this and other important restrictions.
 */

static inline bool fiq_gpio_get_value(struct legoev3_fiq_gpio *gpio)
{
	return (__raw_readl(legoev3_fiq_data->gpio_base + gpio->in_reg)
		& gpio->reg_mask) ? 1 : 0;
}

static inline void fiq_gpio_set_value(struct legoev3_fiq_gpio *gpio, bool value)
{
	if (value)
		__raw_writel(gpio->reg_mask,
			     legoev3_fiq_data->gpio_base + gpio->set_reg);
	else
		__raw_writel(gpio->reg_mask,
			     legoev3_fiq_data->gpio_base + gpio->clr_reg);
}

static inline void fiq_gpio_dir_in(struct legoev3_fiq_gpio *gpio)
{
	int dir = __raw_readl(legoev3_fiq_data->gpio_base + gpio->dir_reg);
	dir |= gpio->reg_mask;
	__raw_writel(dir, legoev3_fiq_data->gpio_base + gpio->dir_reg);
}

static inline void fiq_gpio_dir_out(struct legoev3_fiq_gpio *gpio, bool value)
{
	int dir;

	fiq_gpio_set_value(gpio, value);
	dir = __raw_readl(legoev3_fiq_data->gpio_base + gpio->dir_reg);
	dir &= ~gpio->reg_mask;
	__raw_writel(dir, legoev3_fiq_data->gpio_base + gpio->dir_reg);
}

#define SICR 0x24 /* System Interrupt Status Indexed Clear Register */
static inline void legoev3_fiq_ack(int irq)
{
	__raw_writel(irq, legoev3_fiq_data->intc_base + SICR);
}

#define EISR 0x28 /* System Interrupt Enable Indexed Set Register */
static inline void legoev3_fiq_enable(int irq)
{
	__raw_writel(irq, legoev3_fiq_data->intc_base + EISR);
}

#define EICR 0x2C /* System Interrupt Enable Indexed Clear Register */
static inline void legoev3_fiq_disable(int irq)
{
	__raw_writel(irq, legoev3_fiq_data->intc_base + EICR);
}

static enum fiq_timer_restart
legoev3_fiq_timer_callback(struct legoev3_fiq_port_data *data)
{
	struct i2c_msg *msg = &data->msgs[data->cur_msg];

	fiq_gpio_set_value(&data->gpio[FIQ_I2C_PIN_SCL], data->clock_state);

	switch (data->transfer_state)
	{
	case TRANSFER_START:
		/*
		 * Make sure to SYNC into Timer settings
		 * to ensure first bit time having full length
		 */
		data->cur_msg = 0;
		data->xfer_result = 0;
		data->transfer_state = TRANSFER_START2;
		break;

	case TRANSFER_START2:
		/* Generate start condition - sda low to high while clk high */
		fiq_gpio_dir_out(&data->gpio[FIQ_I2C_PIN_SDA], 0);
		data->clock_state = 0;
		data->nacked = false;
		data->transfer_state = TRANSFER_ADDR;
		break;

	case TRANSFER_ADDR:
		data->data_byte = (msg->addr << 1);
		if (msg->flags & I2C_M_RD)
			data->data_byte |= 1;
		data->buf_offset = 0;

	case TRANSFER_WRITE:
		if (data->transfer_state == TRANSFER_WRITE)
			data->data_byte = msg->buf[data->buf_offset++];
		data->transfer_state = TRANSFER_WBIT;
		data->bit_mask  = 0x80;

	case TRANSFER_WBIT:
		if (!data->clock_state) {
			fiq_gpio_dir_out(&data->gpio[FIQ_I2C_PIN_SDA],
					 data->data_byte & data->bit_mask);
			data->bit_mask >>= 1;
		}

		if (!data->bit_mask && data->clock_state)
			data->transfer_state = TRANSFER_RACK;
		data->clock_state ^= 1;
		break;

	case TRANSFER_READ:
		fiq_gpio_dir_in(&data->gpio[FIQ_I2C_PIN_SDA]);
		data->transfer_state = TRANSFER_RBIT;
		data->bit_mask  = 0x80;
		data->data_byte = 0;

	case TRANSFER_RBIT:
		if (data->clock_state) {
			data->data_byte |= fiq_gpio_get_value(&data->gpio[FIQ_I2C_PIN_SDA])
					   ? data->bit_mask : 0;
			data->bit_mask >>= 1;

			if (!data->bit_mask) {
				msg->buf[data->buf_offset++] = data->data_byte;
				data->transfer_state = TRANSFER_WACK;
			}
		}
		data->clock_state ^= 1;
		break;

	case TRANSFER_RACK:
		if (!data->clock_state)
			fiq_gpio_dir_in(&data->gpio[FIQ_I2C_PIN_SDA]);
		else {
			if (!fiq_gpio_get_value(&data->gpio[FIQ_I2C_PIN_SDA])) {
				if (data->buf_offset < msg->len) {
					data->wait_cycles = 4;
					data->transfer_state = TRANSFER_WAIT;
				}
				else
					data->transfer_state = TRANSFER_STOP;
			} else {
				data->nacked = true;
				data->xfer_result = -ENXIO;
				data->transfer_state = TRANSFER_STOP;
			}
		}
		data->clock_state ^= 1;
		break;

	case TRANSFER_WACK:
		if (!data->clock_state)
			/* ACK (or NACK the last byte read) */
			fiq_gpio_dir_out(&data->gpio[FIQ_I2C_PIN_SDA],
					 data->buf_offset == msg->len);
		else {
			if (data->buf_offset < msg->len) {
				data->wait_cycles = 2;
				data->transfer_state = TRANSFER_WAIT;
			} else
				data->transfer_state = TRANSFER_STOP;
		}
		data->clock_state ^= 1;
		break;

	case TRANSFER_WAIT:
		if (data->wait_cycles--)
			break;
		else if (msg->flags & I2C_M_RD)
			data->transfer_state = TRANSFER_READ;
		else
			data->transfer_state = TRANSFER_WRITE;
		break;

	case TRANSFER_STOP:
		/* generate stop condition - sda low to high while clock is high */
//		if ((msg->flags & I2C_M_RD) && data->buf_offset < msg->len)
			fiq_gpio_dir_out(&data->gpio[FIQ_I2C_PIN_SDA], 0);
//		else
//			fiq_gpio_dir_in(&data->gpio[FIQ_I2C_PIN_SDA]);

		if (data->clock_state)
			data->transfer_state = TRANSFER_STOP2;
		data->clock_state = 1;
		break;

	case TRANSFER_STOP2:
		if ((data->cur_msg + 1) < data->num_msg && !data->nacked)
		{
			/*
			 * This is some non-standard i2c weirdness for
			 * compatibility with the NXT ultrasonic sensor.
			 *
			 * Normal i2c would just send a restart (sda high
			 * to low while clk is high) between writing the
			 * address and reading the data. Instead, we send
			 * a stop (sda low to high while clk is high) and
			 * then do an extra clock cycle (low then high)
			 * before sending a start and reading the data.
			 */
			fiq_gpio_dir_out(&data->gpio[FIQ_I2C_PIN_SDA], 1);
			data->clock_state ^= 1;
			if  (data->clock_state) {
				data->cur_msg++;
				data->transfer_state = TRANSFER_RESTART;
			}
		} else
			data->transfer_state = TRANSFER_STOP3;
		break;

	case TRANSFER_RESTART:
		data->transfer_state = TRANSFER_START2;
		break;

	case TRANSFER_STOP3:
		/*
		 * Leave sda in input position when not in use so that
		 * we can detect when a sensor is disconnected. (Device
		 * detection is implemented in the ev3-input-ports driver.)
		 */
		fiq_gpio_dir_in(&data->gpio[FIQ_I2C_PIN_SDA]);
		data->transfer_state = TRANSFER_COMPLETE;

	case TRANSFER_COMPLETE:
		/*
		 * Keep toggling the status gpio until the gpio irq handler
		 * acks it by setting transfer_state to TRANSFER_IDLE.
		 */
		fiq_gpio_set_value(&legoev3_fiq_data->status_gpio,
			!fiq_gpio_get_value(&legoev3_fiq_data->status_gpio));
		break;
	case TRANSFER_IDLE:
		return FIQ_TIMER_NORESTART;

	default:
		break;
	}

	return FIQ_TIMER_RESTART;
}

void legoev3_fiq_handler(void)
{
	struct legoev3_fiq_port_data *port_data;
	int restart_timer = 0;
	int i;

	for (i = 0; i < LEGOEV3_NUM_PORT_IN; i++) {
		if (!(legoev3_fiq_data->port_req_flags & BIT(i)))
			continue;
		port_data = &legoev3_fiq_data->port_data[i];
		if (port_data != TRANSFER_IDLE)
			restart_timer |= legoev3_fiq_timer_callback(port_data);
	}

	if (!restart_timer)
		legoev3_fiq_disable(legoev3_fiq_data->timer_irq);

	legoev3_fiq_ack(legoev3_fiq_data->timer_irq);
}

void legoev3_fiq_set_gpio(int gpio_pin, struct legoev3_fiq_gpio *gpio)
{
	int bank = gpio_pin >> 4;
	int bank_offset = (bank >> 1) * 0x28;
	int index = gpio_pin & 0xF;

	gpio->dir_reg	= bank_offset + 0x10;
	gpio->set_reg	= bank_offset + 0x18;
	gpio->clr_reg	= bank_offset + 0x1C;
	gpio->in_reg	= bank_offset + 0x20;
	gpio->reg_mask	= BIT(index + (bank & 1) * 16);
}

static irqreturn_t legoev3_fiq_gpio_irq_callback(int irq, void *port_data)
{
	struct legoev3_fiq_port_data *data = port_data;
	int i;

	local_fiq_disable();

	if (data->transfer_state == TRANSFER_COMPLETE) {
		for (i = 0; i < data->num_msg; i++)
			memcpy(&data->xfer_msgs[i], &data->msgs[i],
			       sizeof(struct i2c_msg));
		fiq_gpio_set_value(&legoev3_fiq_data->status_gpio, 0);
		data->transfer_state = TRANSFER_IDLE;
		if (data->complete)
			data->complete(data->xfer_result, data->context);
	}
	local_fiq_enable();

	return IRQ_HANDLED;
}

/**
 * legoev3_fiq_request_port - Requests ownership of the I2C backend for the
 *	specified port.
 * @port_id: The port identifier.
 * @sda_pin: The GPIO pin to use for the I2C data line. This GPIO should
 *	already be requested and set high by the calling code and not
 *	touched again until legoev3_fiq_release_port is called.
 * @scl_pin: The GPIO pin to use for the I2C clock line. Same notes apply
 *	as on the sda_pin.
 *
 * Returns 0 if the port is availible or -EBUSY if it has already be requested.
 */
int legoev3_fiq_request_port(enum legoev3_input_port_id port_id, int sda_pin,
			     int scl_pin)
{
	struct legoev3_fiq_port_data *data;
	int ret;

	if (!legoev3_fiq_data)
		return -ENODEV;
	if (legoev3_fiq_data->port_req_flags & BIT(port_id))
		return -EBUSY;

	data = &legoev3_fiq_data->port_data[port_id];
	legoev3_fiq_set_gpio(sda_pin, &data->gpio[FIQ_I2C_PIN_SDA]);
	legoev3_fiq_set_gpio(scl_pin, &data->gpio[FIQ_I2C_PIN_SCL]);

	ret = request_irq(legoev3_fiq_data->status_gpio_irq,
			  legoev3_fiq_gpio_irq_callback,
			  IRQF_TRIGGER_RISING | IRQF_SHARED,
			  legoev3_fiq_data->pdev->name, data);
	if (ret < 0) {
		dev_err(&legoev3_fiq_data->pdev->dev,
			"Unable to claim irq %d; error %d\n",
			legoev3_fiq_data->status_gpio_irq, ret);
		return ret;
	}

	data->transfer_state = TRANSFER_IDLE;
	legoev3_fiq_data->port_req_flags |= BIT(port_id);

	return 0;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_request_port);

/**
 * legoev3_fiq_release_port- Releases ownership of the I2C backend for the
 *	specified port.
 * @port_id: The port identifier.
 */
void legoev3_fiq_release_port(enum legoev3_input_port_id port_id)
{
	struct legoev3_fiq_port_data *data = &legoev3_fiq_data->port_data[port_id];

	if (!legoev3_fiq_data)
		return;

	while (data->transfer_state != TRANSFER_IDLE);
	legoev3_fiq_data->port_req_flags &= ~BIT(port_id);
	free_irq(legoev3_fiq_data->status_gpio_irq, data);
}
EXPORT_SYMBOL_GPL(legoev3_fiq_release_port);

/**
 * legoev3_fiq_start_xfer- Start an I2C transfer on the specified port.
 * @port_id: The port identifer that was previously requested.
 * @msgs: The message data to transfer.
 * @num_msg: Number of messages in msgs[].
 * @complete: Function to call when transfer is complete.
 * @context: Pointer that is passed as an argument to the complete function.
 *
 * You should only call this for a port that returned successfully from
 * legoev3_fiq_request_port.
 */
int legoev3_fiq_start_xfer(enum legoev3_input_port_id port_id,
			   struct i2c_msg msgs[], int num_msg,
			   void (*complete)(int, void *), void *context)
{
	struct legoev3_fiq_port_data *data;
	int i;

	if (!legoev3_fiq_data)
		return -ENODEV;
	if (port_id >= LEGOEV3_NUM_PORT_IN)
		return -EINVAL;
	if (num_msg < 1 || num_msg > 2)
		return -EINVAL;
	if (!(legoev3_fiq_data->port_req_flags & BIT(port_id)))
		return -EINVAL;

	data = &legoev3_fiq_data->port_data[port_id];
	if (data->transfer_state != TRANSFER_IDLE)
		return -EBUSY;

	/* copy the messages so that fiq has exclusive access */
	for (i = 0; i < num_msg; i++)
		memcpy(&data->msgs[i], &msgs[i], sizeof(struct i2c_msg));
	/*
	 * we also have to hand on to the real messages so that we can
	 * copy any data read back to them when the transfer is complete.
	 */
	data->xfer_msgs = msgs;
	data->num_msg = num_msg;
	data->complete = complete;
	data->context = context;
	data->transfer_state = TRANSFER_START;
	legoev3_fiq_enable(legoev3_fiq_data->timer_irq);

	return 0;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_start_xfer);


static int __devinit legoev3_fiq_probe(struct platform_device *pdev)
{
	struct legoev3_fiq_data *fiq_data;
	struct legoev3_fiq_platform_data *pdata;
	struct resource *res;
	int ret;

	if (legoev3_fiq_data)
		return -EINVAL;

	fiq_data = &legoev3_fiq_init_data;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "Missing platform data.\n");
		return -EINVAL;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "gpio-mem");
	if (!res) {
		dev_err(&pdev->dev, "Missing gpio-mem resource.\n");
		return -EINVAL;
	}
	fiq_data->gpio_base = devm_request_and_ioremap(&pdev->dev, res);
	if (WARN_ON(!fiq_data->gpio_base))
		return -EADDRNOTAVAIL;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "intc-mem");
	if (!res) {
		dev_err(&pdev->dev, "Missing intc-mem resource.\n");
		return -EINVAL;
	}
	fiq_data->intc_base = devm_request_and_ioremap(&pdev->dev, res);
	if (WARN_ON(!fiq_data->intc_base))
		return -EADDRNOTAVAIL;

	ret = platform_get_irq_byname(pdev, "timer-irq");
	if (ret < 0) {
		dev_err(&pdev->dev, "Missing timer-irq resource.\n");
		return -EINVAL;
	}
	fiq_data->timer_irq = ret;

	ret = gpio_request_one(pdata->status_gpio, GPIOF_INIT_LOW, "fiq status");
	if (ret < 0) {
		dev_err(&legoev3_fiq_data->pdev->dev,
			"Unable to request GPIO %d, error %d\n",
			pdata->status_gpio, ret);
		return ret;
	}
	legoev3_fiq_set_gpio(pdata->status_gpio, &fiq_data->status_gpio);

	ret = gpio_to_irq(pdata->status_gpio);
	if (ret < 0) {
		dev_err(&legoev3_fiq_data->pdev->dev,
			"Unable to get irq number for GPIO %d, error %d\n",
			pdata->status_gpio, ret);
		goto err_gpio_to_irq;
	}
	fiq_data->status_gpio_irq = ret;

	ret = claim_fiq(&fiq_data->fiq_handler);
	if (ret < 0)
		goto err_claim_fiq;
	set_fiq_c_handler(legoev3_fiq_handler);

	fiq_data->pdev = pdev;
	legoev3_fiq_data = fiq_data;

	return 0;

err_claim_fiq:
err_gpio_to_irq:
	gpio_free(pdata->status_gpio);

	return ret;
}

static struct platform_driver legoev3_fiq_driver = {
	.probe = legoev3_fiq_probe,
	.driver = {
		.name = "legoev3-fiq",
		.owner = THIS_MODULE,
	},
};
module_platform_driver(legoev3_fiq_driver);
