/*
 * FIQ backend for I2C bus driver for LEGO MINDSTORMS EV3
 * Copyright (C) 2013-2015 David Lechner <david@lechnology.com>
 *
 * Based on davinci_iic.c from lms2012
 * That file does not contain a copyright, but comes from the LEGO Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * In order to achieve realtime performance for sound playback and I2C
 * on the input ports, we use FIQs (fast interrupts). These interrupts
 * can interrupt any regular IRQ, which is what makes it possible to get
 * the near-realtime performance, but this also makes them dangerous,
 * so be sure you know what you are doing before making changes here.
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <asm/fiq.h>
#include <mach/cp_intc.h>
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

struct legoev3_fiq_port_i2c_data {
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

struct legoev3_fiq_ehrpwm_data {
	u8 *dma_area;
	int volume;
	unsigned playback_ptr;
	size_t frame_bytes;
	size_t buffer_bytes;
	snd_pcm_uframes_t period_size;
	unsigned callback_count;
	void (*period_elapsed)(void *);
	void *period_elapsed_data;
	unsigned requested_flag:1;
	unsigned period_elapsed_flag:1;
	int ramp_step;
	int ramp_value;
};

struct legoev3_fiq_data {
	struct legoev3_fiq_port_i2c_data port_data[NUM_EV3_PORT_IN];
	struct legoev3_fiq_ehrpwm_data ehrpwm_data;
	struct platform_device *pdev;
	struct fiq_handler fiq_handler;
	struct legoev3_fiq_gpio status_gpio;
	void __iomem *gpio_base;
	void __iomem *intc_base;
	void __iomem *ehrpwm_base;
	int timer_irq;
	int ehrpwm_irq;
	int status_gpio_irq;
	int port_req_flags;
};

static struct legoev3_fiq_data legoev3_fiq_init_data = {
	.fiq_handler = {
		.name = "legoev3-fiq-handler",
	},
};

static struct legoev3_fiq_data *legoev3_fiq_data;

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

#define GPIR 0x80 /* Global Prioritized Index Register */
static inline int legoev3_fiq_get_irq(void)
{
	return __raw_readl(legoev3_fiq_data->intc_base + GPIR);
}

#define TBPRD 0xA /* Time-Base Period Register */
static inline int fiq_ehrpwm_get_period_ticks(void)
{
	return __raw_readw(legoev3_fiq_data->ehrpwm_base + TBPRD);
}

#define CMPB 0x14 /* Counter-Compare B Register */
static inline void fiq_ehrpwm_set_duty_ticks(unsigned ticks)
{
	__raw_writew(ticks, legoev3_fiq_data->ehrpwm_base + CMPB);
}

#define ETSEL 0x32 /* Event-Trigger Selection Register */
#define INTEN (BIT(3)) /* Interrupt enable bit */
#define INTSEL_MASK (BIT(2)|BIT(1)|BIT(0)) /* Interrupt selection mask */
#define INTSEL_TBPRD 2
static inline void fiq_ehrpwm_et_int_enable(void)
{
	short dir = __raw_readw(legoev3_fiq_data->ehrpwm_base + ETSEL);
	dir |= INTEN;
	__raw_writew(dir, legoev3_fiq_data->ehrpwm_base + ETSEL);
}

static inline void fiq_ehrpwm_et_int_disable(void)
{
	short dir = __raw_readw(legoev3_fiq_data->ehrpwm_base + ETSEL);
	dir &= ~INTEN;
	__raw_writew(dir, legoev3_fiq_data->ehrpwm_base + ETSEL);
}

#define ETPS 0x34 /* Event-Trigger Prescale Register */
#define INTPRD_MASK (BIT(1)|BIT(0))
static inline void fiq_ehrpwm_et_int_set_period(unsigned char period)
{
	short dir = __raw_readw(legoev3_fiq_data->ehrpwm_base + ETSEL);
	dir &= ~INTSEL_MASK;
	dir |= INTSEL_TBPRD;
	__raw_writew(dir, legoev3_fiq_data->ehrpwm_base + ETSEL);

	dir = __raw_readw(legoev3_fiq_data->ehrpwm_base + ETPS);
		dir &= ~INTPRD_MASK;
		dir |= period & INTPRD_MASK;
	__raw_writew(dir, legoev3_fiq_data->ehrpwm_base + ETPS);
}

#define ETFLG 0x36 /* Event-Trigger Flag Register */
static inline int fiq_ehrpwm_test_irq(void)
{
	return __raw_readw(legoev3_fiq_data->ehrpwm_base + ETFLG) & 0x1;
}

#define ETCLR 0x38 /* Event-Trigger Clear Register */
static inline void fiq_ehrpwm_clear_irq(void)
{
	__raw_writew(0x1, legoev3_fiq_data->ehrpwm_base + ETCLR);
}

static enum fiq_timer_restart
legoev3_fiq_timer_callback(struct legoev3_fiq_port_i2c_data *data)
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
		/* no break */

	case TRANSFER_WRITE:
		if (data->transfer_state == TRANSFER_WRITE)
			data->data_byte = msg->buf[data->buf_offset++];
		data->transfer_state = TRANSFER_WBIT;
		data->bit_mask  = 0x80;
		/* no break */

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
		/* no break */

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
		/*
		 * Note: The official LEGO firmware does not generate stop
		 * condition except for in the middle of reads (see below).
		 * We are going by the book and doing a stop when we are
		 * supposed to. We can change it back if there are problems.
		 */
		fiq_gpio_dir_out(&data->gpio[FIQ_I2C_PIN_SDA], 0);

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
		 * Generate stop condition - sda low to high while clock
		 * is high. Leave sda in input position when not in use so
		 * that we can detect when a sensor is disconnected. (Device
		 * detection is implemented in the ev3-input-ports driver.)
		 */
		fiq_gpio_dir_in(&data->gpio[FIQ_I2C_PIN_SDA]);
		data->transfer_state = TRANSFER_COMPLETE;
		/* no break */

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

static void legoev3_fiq_ehrpwm_callback(struct legoev3_fiq_ehrpwm_data *data)
{
	int sample;
	unsigned long duty_ticks, period_ticks;

	if (unlikely(!data->requested_flag || !data->dma_area))
		return;

	period_ticks = fiq_ehrpwm_get_period_ticks();

	if (unlikely(data->ramp_step))
	{
		duty_ticks = (data->ramp_value * period_ticks) >> 16;
		if (duty_ticks==0)
			duty_ticks = 1;
		fiq_ehrpwm_set_duty_ticks(duty_ticks);

		data->ramp_value += data->ramp_step;
		if (data->ramp_value >= 0x8000)
		{ // end of ramp up
			data->ramp_step = 0;
		}
		else if (data->ramp_value < 0)
		{ // end of ramp down
			/* do not reset ramp_step here so that we stay at 1 until PWM is 
			   turned off or playback is started again */
			data->ramp_value = 1;
		}

		return;
	}

	sample = *(short *)(data->dma_area + data->playback_ptr);
	sample = (sample * data->volume) >> 8;
	duty_ticks = ((sample + 0x7FFF) * period_ticks) >> 16;

	fiq_ehrpwm_set_duty_ticks(duty_ticks);

	data->playback_ptr += data->frame_bytes;
	if (data->playback_ptr >= data->buffer_bytes)
		data->playback_ptr = 0;

	if (++data->callback_count >= data->period_size)
	{
		data->callback_count =  0;
		data->period_elapsed_flag = 1;
	}

	/* toggle gpio to trigger callback */
	if (data->period_elapsed_flag)
		fiq_gpio_set_value(&legoev3_fiq_data->status_gpio,
			!fiq_gpio_get_value(&legoev3_fiq_data->status_gpio));
}

void legoev3_fiq_handler(void)
{
	int irq = legoev3_fiq_get_irq();

	if (irq == legoev3_fiq_data->timer_irq) {
		struct legoev3_fiq_port_i2c_data *port_data;
		int restart_timer = 0;
		int i;

		for (i = 0; i < NUM_EV3_PORT_IN; i++) {
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
	if (fiq_ehrpwm_test_irq()) {
		legoev3_fiq_ehrpwm_callback(&legoev3_fiq_data->ehrpwm_data);
		legoev3_fiq_ack(legoev3_fiq_data->ehrpwm_irq);
		fiq_ehrpwm_clear_irq();
	}
}

/* --------------- END OF CODE THAT IS CALLED IN FIQ CONTEXT -----------------*/

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

static irqreturn_t legoev3_fiq_gpio_irq_i2c_port_callback(int irq, void *port_data)
{
	struct legoev3_fiq_port_i2c_data *data = port_data;
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
 * Returns 0 if the port is available or -EBUSY if it has already be requested.
 */
int legoev3_fiq_request_port(enum legoev3_input_port_id port_id, int sda_pin,
			     int scl_pin)
{
	struct legoev3_fiq_port_i2c_data *data;
	int ret;

	if (!legoev3_fiq_data)
		return -ENODEV;
	if (legoev3_fiq_data->port_req_flags & BIT(port_id))
		return -EBUSY;

	data = &legoev3_fiq_data->port_data[port_id];
	legoev3_fiq_set_gpio(sda_pin, &data->gpio[FIQ_I2C_PIN_SDA]);
	legoev3_fiq_set_gpio(scl_pin, &data->gpio[FIQ_I2C_PIN_SCL]);

	ret = request_irq(legoev3_fiq_data->status_gpio_irq,
			  legoev3_fiq_gpio_irq_i2c_port_callback,
			  IRQF_TRIGGER_RISING | IRQF_SHARED,
			  legoev3_fiq_data->pdev->name, data);
	if (ret < 0) {
		dev_err(&legoev3_fiq_data->pdev->dev,
			"Unable to claim irq %d; error %d\n",
			legoev3_fiq_data->status_gpio_irq, ret);
		return ret;
	}

	data->transfer_state = TRANSFER_IDLE;
	data->clock_state = 1;
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
	struct legoev3_fiq_port_i2c_data *data;

	if (!legoev3_fiq_data)
		return;

	data =  &legoev3_fiq_data->port_data[port_id];
	data->transfer_state = TRANSFER_IDLE;
	legoev3_fiq_data->port_req_flags &= ~BIT(port_id);
	free_irq(legoev3_fiq_data->status_gpio_irq, data);
}
EXPORT_SYMBOL_GPL(legoev3_fiq_release_port);

/**
 * legoev3_fiq_start_xfer- Start an I2C transfer on the specified port.
 * @port_id: The port identifier that was previously requested.
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
	struct legoev3_fiq_port_i2c_data *data;
	int i;

	if (!legoev3_fiq_data)
		return -ENODEV;
	if (port_id >= NUM_EV3_PORT_IN)
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
	 * we also have to hang on to the real messages so that we can
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

void legoev3_fiq_cancel_xfer(enum legoev3_input_port_id port_id)
{
	struct legoev3_fiq_port_i2c_data *data;

	if (!legoev3_fiq_data)
		return;
	if (port_id >= NUM_EV3_PORT_IN)
		return;

	data = &legoev3_fiq_data->port_data[port_id];
	data->transfer_state = TRANSFER_IDLE;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_cancel_xfer);

static irqreturn_t
legoev3_fiq_gpio_irq_period_elapsed_callback(int irq, void *ehrpwm_data)
{
	struct legoev3_fiq_ehrpwm_data *data = ehrpwm_data;

	local_fiq_disable();
	if (data->period_elapsed) {
		data->period_elapsed(data->period_elapsed_data);
		data->period_elapsed_flag = 0;
		fiq_gpio_set_value(&legoev3_fiq_data->status_gpio, 0);
	}
	local_fiq_enable();

	return IRQ_HANDLED;
}

int legoev3_fiq_ehrpwm_int_enable(void)
{
	if (!legoev3_fiq_data || !legoev3_fiq_data->ehrpwm_data.requested_flag)
		return -ENODEV;

	fiq_ehrpwm_et_int_enable();
	return 0;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_int_enable);

int legoev3_fiq_ehrpwm_int_disable(void)
{
	if (!legoev3_fiq_data || !legoev3_fiq_data->ehrpwm_data.requested_flag)
		return -ENODEV;

	fiq_ehrpwm_et_int_disable();
	return 0;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_int_disable);

/*
 * Only called when the ehrpwm interrupt is configured as regular IRQ and
 * not as a FIQ. In other words, this is for debugging (when assigned to IRQ)
 * and serves as a dummy callback during normal usage (when assigned to FIQ).
 */
static irqreturn_t legoev3_fiq_ehrpwm_et_callback(int irq, void *data)
{
	fiq_c_handler_t handler = get_fiq_c_handler();

	if (handler)
		handler();

	return IRQ_HANDLED;
}

int legoev3_fiq_ehrpwm_request(void)
{
	int err;

	if (!legoev3_fiq_data)
		return -ENODEV;

	if (legoev3_fiq_data->ehrpwm_data.requested_flag)
		return -EBUSY;
	legoev3_fiq_data->ehrpwm_data.requested_flag = 1;

	err = request_irq(legoev3_fiq_data->status_gpio_irq,
			  legoev3_fiq_gpio_irq_period_elapsed_callback,
			  IRQF_TRIGGER_RISING | IRQF_SHARED,
			  legoev3_fiq_data->pdev->name,
			  &legoev3_fiq_data->ehrpwm_data);
	if (err) {
		dev_err(&legoev3_fiq_data->pdev->dev,
			"Unable to claim irq %d; error %d\n",
			legoev3_fiq_data->status_gpio_irq, err);
		legoev3_fiq_data->ehrpwm_data.requested_flag = 0;
		return err;
	}

	err = request_irq(legoev3_fiq_data->ehrpwm_irq,
			  legoev3_fiq_ehrpwm_et_callback, 0,
			  "legoev3_fiq_ehrpwm_debug",
			  &legoev3_fiq_data->ehrpwm_data);
	if (err)
		dev_warn(&legoev3_fiq_data->pdev->dev,
			 "Failed to request ehrpwm irq.");

	return 0;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_request);

void legoev3_fiq_ehrpwm_release(void)
{
	int i;
	if (!legoev3_fiq_data)
		return;

	if ((legoev3_fiq_data->ehrpwm_data.ramp_step < 0) &&
	    (legoev3_fiq_data->ehrpwm_data.ramp_value > 0))
	{
		/*
		 * we are still in the process of ramping down,
		 * wait for competition
		 */
		for (i=0; i<5; ++i)
		{
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(10 * HZ / 1000);
			
			if (legoev3_fiq_data->ehrpwm_data.ramp_value <= 0)
				break;
		}
	}
	
	local_fiq_disable();
	free_irq(legoev3_fiq_data->status_gpio_irq,
		 &legoev3_fiq_data->ehrpwm_data);
	free_irq(legoev3_fiq_data->ehrpwm_irq, &legoev3_fiq_data->ehrpwm_data);
	legoev3_fiq_data->ehrpwm_data.period_elapsed_flag = 0;
	legoev3_fiq_data->ehrpwm_data.requested_flag = 0;
	local_fiq_enable();
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_release);

int legoev3_fiq_ehrpwm_prepare(struct snd_pcm_substream *substream, int volume,
			       unsigned char int_period,
			       void (*period_elapsed)(void *), void *context)
{
	struct legoev3_fiq_ehrpwm_data *data;

	if (!legoev3_fiq_data || !legoev3_fiq_data->ehrpwm_data.requested_flag)
		return -ENODEV;

	if (int_period > 3)
		return -EINVAL;

	data = &legoev3_fiq_data->ehrpwm_data;

	local_fiq_disable();

	data->dma_area			= substream->runtime->dma_area;
	data->playback_ptr		= 0;
	data->volume			= volume;
	data->frame_bytes		= frames_to_bytes(substream->runtime, 1);
	data->buffer_bytes		= snd_pcm_lib_buffer_bytes(substream);
	data->period_size		= substream->runtime->period_size;
	data->callback_count		= 0;
	data->period_elapsed		= period_elapsed;
	data->period_elapsed_data	= context;
	data->period_elapsed_flag	= 0;
	data->ramp_step			= 0;
	data->ramp_value		= 0;
	fiq_ehrpwm_et_int_set_period(int_period);

	local_fiq_enable();

	return 0;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_prepare);

void legoev3_fiq_ehrpwm_ramp(struct snd_pcm_substream *substream,
			     int direction, unsigned ramp_ms)
{
	struct legoev3_fiq_ehrpwm_data *data;
	int ramp_samples = substream->runtime->rate * ramp_ms / 1000;
	if (ramp_samples < 2)
		return;

	if (!legoev3_fiq_data)
		return;

	if (direction==0)
		return;

	data = &legoev3_fiq_data->ehrpwm_data;

	local_fiq_disable();

	if (direction > 0)
	{ // ramp up
		data->ramp_step  = 0x8000 / ramp_samples;
		data->ramp_value = 1;
	}
	else
	{ // ramp down
		data->ramp_step  = 0x8000 / -ramp_samples;
		data->ramp_value = 0x8000;
	}

	local_fiq_enable();
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_ramp);

unsigned legoev3_fiq_ehrpwm_get_playback_ptr(void)
{
	return legoev3_fiq_data->ehrpwm_data.playback_ptr;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_get_playback_ptr);

void legoev3_fiq_ehrpwm_set_volume(int volume)
{
	legoev3_fiq_data->ehrpwm_data.volume = volume;
}
EXPORT_SYMBOL_GPL(legoev3_fiq_ehrpwm_set_volume);

static int legoev3_fiq_probe(struct platform_device *pdev)
{
	struct legoev3_fiq_data *fiq_data;
	struct legoev3_fiq_platform_data *pdata;
	int ret;

	if (legoev3_fiq_data)
		return -EINVAL;

	fiq_data = &legoev3_fiq_init_data;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "Missing platform data.\n");
		return -EINVAL;
	}

	fiq_data->gpio_base = devm_ioremap(&pdev->dev, pdata->gpio_mem_base,
					   pdata->gpio_mem_size);
	if (WARN_ON(!fiq_data->gpio_base))
		return -EADDRNOTAVAIL;

	fiq_data->intc_base = devm_ioremap(&pdev->dev, pdata->intc_mem_base,
					   pdata->intc_mem_size);
	if (WARN_ON(!fiq_data->intc_base))
		return -EADDRNOTAVAIL;

	fiq_data->ehrpwm_base = devm_ioremap(&pdev->dev, pdata->ehrpwm_mem_base,
					     pdata->ehrpwm_mem_size);
	if (WARN_ON(!fiq_data->ehrpwm_base))
		return -EADDRNOTAVAIL;

	fiq_data->timer_irq = pdata->timer_irq;
	fiq_data->ehrpwm_irq = pdata->ehrpwm_irq;

	ret = gpio_request_one(pdata->status_gpio, GPIOF_INIT_LOW, "fiq status");
	if (ret < 0) {
		dev_err(&pdev->dev,
			"Unable to request GPIO %d, error %d\n",
			pdata->status_gpio, ret);
		return ret;
	}
	legoev3_fiq_set_gpio(pdata->status_gpio, &fiq_data->status_gpio);

	ret = gpio_to_irq(pdata->status_gpio);
	if (ret < 0) {
		dev_err(&pdev->dev,
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

	cp_intc_fiq_enable();

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
