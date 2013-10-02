/*
 * eCAP driver for PWM Capture
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/kdev_t.h>
#include <linux/pwm/pwm.h>
#include <linux/pwm/ecap_cap.h>

/* eCAP register offsets */
#define CAP1					0x08
#define CAP2					0x0C
#define CAP3					0x10
#define CAP4					0x14
#define ECCTL1					0x28
#define ECCTL2					0x2A
#define ECEINT					0x2C
#define ECFLG					0x2E
#define ECCLR					0x30

/* Interrupt enable bits */
#define CAP1_INT				0x2
#define CAP2_INT				0x4
#define CAP3_INT				0x8
#define CAP4_INT				0x10
#define OVF_INT					0x20
#define INT_FLAG_CLR				0x3F

/* Generic */
#define EC_WRAP4				0x3
#define EC_RUN					0x1
#define EC_SYNCO_DIS				0x2

#define EC_DISABLE				0x0
#define EC_ENABLE				0x1
#define TIMEOUT					10
#define TOTAL_EVENTS				4

struct ecap_dev {
	struct ecap_cap ecap_cap;
	struct clk *clk;
	int *ecap_ptr;
	struct completion comp;
	void __iomem	*mmio_base;
};

static int get_cap_value(struct ecap_dev *);
static void ecap_cap_polarity(void __iomem *base, short polarity);
static void ecap_cap_mode(void __iomem *base, short mode);
static int ecap_config(struct ecap_dev *ecap);
static unsigned int get_freq(unsigned int diff, int prescale);
static DEFINE_MUTEX(ecap_mutex);

static int ecap_val[4];
static int sys_freq;

static inline unsigned int ecap_read_long(void __iomem *base, int offset)
{
	return __raw_readl(base + offset);
}
static inline unsigned short ecap_read_short(void __iomem *base, int offset)
{
	return __raw_readw(base + offset);
}
static inline void ecap_write_short(void __iomem *base, int offset, short val)
{
	__raw_writew(val, base + offset);
}

/**
 * Capture event polarity select
 * @base	: base address of the ecap module
 * @polarity=0	: Capture events 1, 2, 3 and 4 triggers on rising edge
 * @polarity=1	: Capture events 1 and 3 triggers on falling edge,
			 event 0 and 2 on rising edge
 */
static void ecap_cap_polarity(void __iomem *base, short polarity)
{
	ecap_write_short(base, ECCTL1,
		(ecap_read_short(base, ECCTL1) & ~BIT(0)) | EC_RISING);
	ecap_write_short(base, ECCTL1,
		(ecap_read_short(base, ECCTL1) & ~BIT(4)) | (EC_RISING<<4));
	ecap_write_short(base, ECCTL1,
		(ecap_read_short(base, ECCTL1) & ~BIT(2)) | (polarity<<2));
	ecap_write_short(base, ECCTL1,
		(ecap_read_short(base, ECCTL1) & ~BIT(6)) | (polarity<<6));
}

/**
 * Capture mode selection
 * @base	: base address of the ecap module
 * @mode=0	: Absolute mode
 * @mode=1	: Delta mode
*/
static void ecap_cap_mode(void __iomem *base, short mode)
{
	int i;

	/* Bit 1, 3, 5 and 7 represents the
	 * aboslute and delta mode in the control reg 1 */
	for (i = 1; i <= 7; i += 2)
		ecap_write_short(base, ECCTL1,
			(ecap_read_short(base, ECCTL1) & ~BIT(i)) |
			((mode & 0x1)<<i));
}

/**
 * Enable loading of cap1-4 register on capture event
 * @base	: base address of the ecap module
*/
static inline void ecap_enable(void __iomem *base)
{
	ecap_write_short(base, ECCTL1,
		ecap_read_short(base, ECCTL1) | BIT(8));
}

/**
 * Selects continuous or one shot mode control
 * @base	: base address of the ecap module
 * @val=0	: Operates in continuous mode
 * @val=1	: Operates in one-shot mode
  */
static inline void ecap_op_mode(void __iomem *base, short val)
{
	ecap_write_short(base, ECCTL2,
		(ecap_read_short(base, ECCTL2) & ~BIT(0)) | (val & 0x1));
}

/**
 * ecap_prescale - Prescales the capture signals
 * @base          : base address of ecap modules
 * @prescale      : Divides the capture signal by this value
 */
static inline void ecap_prescale(void __iomem *base, int prescale)
{
	ecap_write_short(base, ECCTL1,
			(ecap_read_short(base, ECCTL1) & 0x1FF)
			| (prescale << 9));
}

/**
 * Enables the specific interrupt of the ecap module
 */
static inline void ecap_int_enable(void __iomem *base, short flag)
{
	ecap_write_short(base, ECCLR, INT_FLAG_CLR);
	ecap_write_short(base, ECEINT, flag);
}

/**
 * Starts time stamp counter
 */
static inline void ecap_free_run(void __iomem *base)
{
	ecap_write_short(base, ECCTL2,
		ecap_read_short(base, ECCTL2) | (EC_RUN << 4));
}

/**
 * Stops times stamp counter
 */
static inline void ecap_freeze_counter(void __iomem *base)
{
	ecap_write_short(base, ECCTL2,
	ecap_read_short(base, ECCTL2) & ~(EC_RUN << 4));
}

/**
 * Rearm the capture register load in
 * single shot mode
 */
static inline void ecap_rearm(void __iomem *base)
{
	ecap_write_short(base, ECCTL2,
		ecap_read_short(base, ECCTL2) | BIT(3));
}

/**
 * Initializes and configures the ecap module
 */
static int ecap_config(struct ecap_dev *ecap)
{
	struct ecap_cap *ecap_cap = &ecap->ecap_cap;

	if (ecap_cap->edge > EC_RISING_FALLING ||
			ecap_cap->prescale > 0x1F)
		return -EINVAL;

	mutex_lock(&ecap_mutex);
	clk_enable(ecap->clk);
	ecap_cap_polarity(ecap->mmio_base, EC_RISING);
	ecap_cap_mode(ecap->mmio_base, EC_ABS_MODE);
	ecap_enable(ecap->mmio_base);
	ecap_prescale(ecap->mmio_base, ecap_cap->prescale);

	/* Selects capture as an operating mode */
	ecap_write_short(ecap->mmio_base, ECCTL2,
			(ecap_read_short(ecap->mmio_base, ECCTL2)
						& ~BIT(9)));
	/* configures to one shot mode */
	ecap_op_mode(ecap->mmio_base, EC_ONESHOT);
	/* Disables the sync out */
	ecap_write_short(ecap->mmio_base, ECCTL2,
			ecap_read_short(ecap->mmio_base, ECCTL2)
						| (EC_SYNCO_DIS << 6));
	/* Disables the sync in */
	ecap_write_short(ecap->mmio_base, ECCTL2,
			  ecap_read_short(ecap->mmio_base, ECCTL2)
						| (EC_DISABLE << 6));
	/* Wrap after capture event 4 in continuous mode,
	 * stops after capture event 4 in single shot mode
	 */
	ecap_write_short(ecap->mmio_base, ECCTL2,
			ecap_read_short(ecap->mmio_base, ECCTL2)
						| (EC_WRAP4 << 1));
	clk_disable(ecap->clk);
	mutex_unlock(&ecap_mutex);

	return 0;
}

static irqreturn_t ecap_davinci_isr(int this_irq, void *dev_id)
{
	struct ecap_dev *ecap = dev_id;

	*ecap->ecap_ptr++ = ecap_read_long(ecap->mmio_base, CAP1);
	*ecap->ecap_ptr++ = ecap_read_long(ecap->mmio_base, CAP2);
	*ecap->ecap_ptr++ = ecap_read_long(ecap->mmio_base, CAP3);
	*ecap->ecap_ptr++ = ecap_read_long(ecap->mmio_base, CAP4);

	complete(&ecap->comp);
	ecap_write_short(ecap->mmio_base, ECCLR, INT_FLAG_CLR);
	return IRQ_HANDLED;
}

static ssize_t config_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecap_dev *ecap = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ecap_config(ecap));
}

static ssize_t prescale_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecap_dev *ecap = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n",
			ecap->ecap_cap.prescale);
}

static ssize_t prescale_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long result;
	struct ecap_dev *ecap = dev_get_drvdata(dev);

	kstrtoul(buf, 10, &result);

	ecap_prescale(ecap->mmio_base, result);
	ecap->ecap_cap.prescale = result;

	return len;
}

static ssize_t freq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ecap_dev *ecap = dev_get_drvdata(dev);
	unsigned int freq1, freq2, freq3;
	int prescale_val, ret;

	mutex_lock(&ecap_mutex);
	prescale_val = ecap->ecap_cap.prescale;

	if (!prescale_val)
		prescale_val = 1;
	else
		/*
		 * Divides the input pwm with the prescaler
		 * value multiplied by 2
		 */
		prescale_val *= 2;

	ecap->ecap_ptr = ecap_val;
	ret = get_cap_value(ecap);
	mutex_unlock(&ecap_mutex);
	if (!ret)
		return sprintf(buf, "-1\n");

	freq1 = get_freq((ecap_val[1] - ecap_val[0]), prescale_val);
	freq2 = get_freq((ecap_val[2] - ecap_val[1]), prescale_val);
	freq3 = get_freq((ecap_val[3] - ecap_val[2]), prescale_val);

	return sprintf(buf, "%d,%d,%d\n", freq1, freq2, freq3);
}

static ssize_t duty_percentage_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	unsigned int diff1, diff2, duty;
	struct ecap_dev *ecap = dev_get_drvdata(dev);

	mutex_lock(&ecap_mutex);
	/* Setting ecap register to calculate duty cycle */
	ecap_cap_polarity(ecap->mmio_base,
					EC_RISING_FALLING);
	if (ecap->ecap_cap.prescale > 0)
		ecap_prescale(ecap->mmio_base, 0);

	ecap->ecap_ptr = ecap_val;
	ret = get_cap_value(ecap);

	/* Resetting it back*/
	ecap_cap_polarity(ecap->mmio_base, EC_RISING);
	if (ecap->ecap_cap.prescale)
		ecap_prescale(ecap->mmio_base,
				 ecap->ecap_cap.prescale);
	mutex_unlock(&ecap_mutex);
	if (!ret)
		return sprintf(buf, "-1\n");
	diff1 = ecap_val[1] - ecap_val[0];
	diff2 = ecap_val[2] - ecap_val[0];
	duty = (diff1 * 100)/diff2;

	return sprintf(buf, "%d%%\n", duty);
}

static int get_cap_value(struct ecap_dev *ecap)
{
	int r = 0;
	clk_enable(ecap->clk);
	ecap_free_run(ecap->mmio_base);
	init_completion(&ecap->comp);
	ecap_rearm(ecap->mmio_base);
	ecap_int_enable(ecap->mmio_base, CAP4_INT);

	/* Waits for interrupt to occur */
	r = wait_for_completion_interruptible_timeout(&ecap->comp,
							TIMEOUT*HZ);

	ecap_freeze_counter(ecap->mmio_base);
	clk_disable(ecap->clk);

	return r;
}

static unsigned int get_freq(unsigned int diff, int prescale)
{
	unsigned int freq;

	if (diff == 0)
		return 0;
	return freq = sys_freq / (diff / prescale);
}

int ecap_cap_config(int instance, struct ecap_cap *ecap_cap)
{
	struct device *device = NULL;
	struct ecap_dev *ecap;
	char name[20];

	scnprintf(name, sizeof name, "%s.%d", "ecap_cap", instance);
	device = capture_request_device(name);
	if (!device)
		return -EINVAL;
	ecap = dev_get_drvdata(device);
	memcpy(&ecap->ecap_cap, ecap_cap, sizeof(ecap_cap));
	return ecap_config(ecap);
}
EXPORT_SYMBOL(ecap_cap_config);

int ecap_get_cap_value(int *cap_value, int instance)
{
	struct device *device = NULL;
	struct ecap_dev *ecap;
	char name[20];
	int ret;

	scnprintf(name, sizeof name, "%s.%d", "ecap_cap", instance);
	device = capture_request_device(name);
	if (!device)
		return -EINVAL;
	ecap = dev_get_drvdata(device);
	ecap->ecap_ptr = cap_value;

	mutex_lock(&ecap_mutex);
	ret = get_cap_value(ecap);
	mutex_unlock(&ecap_mutex);
	if (ret != 0)
		ret = -EINVAL;

	return ret;
}
EXPORT_SYMBOL(ecap_get_cap_value);

static DEVICE_ATTR(config, S_IRUGO | S_IWUSR, config_show, NULL);
static DEVICE_ATTR(prescale, S_IRUGO | S_IWUSR, prescale_show, prescale_store);
static DEVICE_ATTR(freq, S_IRUGO | S_IWUSR, freq_show, NULL);
static DEVICE_ATTR(duty_percentage, S_IRUGO | S_IWUSR,
		duty_percentage_show, NULL);

static const struct attribute *ecap_attrs[] = {
	&dev_attr_config.attr,
	&dev_attr_prescale.attr,
	&dev_attr_duty_percentage.attr,
	&dev_attr_freq.attr,
	NULL,
};

static const struct attribute_group ecap_device_attr_group = {
	.name  = NULL,
	.attrs = (struct attribute **) ecap_attrs,
};

static int __init ecap_probe(struct platform_device *pdev)
{
	struct resource *r;
	int irq_start, ret;
	struct ecap_dev *ecap;
	struct device *new_dev;

	ecap = kzalloc(sizeof(struct ecap_dev), GFP_KERNEL);
	if (!ecap) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_ret;
	}
	ecap->clk = clk_get(&pdev->dev, "ecap");
	if (IS_ERR(ecap->clk)) {
		dev_err(&pdev->dev, "clk_get function failed\n");
		ret = PTR_ERR(ecap->clk);
		goto err_free_alloc;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&pdev->dev, "no memory resource defined\n");
		ret = -ENODEV;
		goto err_free_clk;
	}

	irq_start = platform_get_irq(pdev, 0);
	if (irq_start == -ENXIO) {
		dev_err(&pdev->dev, "no irq resource?\n");
		ret = -ENODEV;
		goto err_free_clk;
	}

	r = request_mem_region(r->start, resource_size(r), pdev->name);
	if (!r) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto err_free_clk;
	}

	ecap->mmio_base = ioremap(r->start, resource_size(r));
	if (!ecap->mmio_base) {
		dev_err(&pdev->dev, "failed to ioremap() registers\n");
		ret = -ENODEV;
		goto err_free_mem;
	}
	ret = request_irq(irq_start, ecap_davinci_isr, 0, pdev->name, ecap);
	if (ret) {
		dev_err(&pdev->dev, "failure in requesting irq\n");
		ret = -ENODEV;
		goto err_free_io_mem;
	}
	sys_freq = clk_get_rate(ecap->clk);
	new_dev = capture_dev_register(&pdev->dev);
	if (!new_dev) {
		dev_err(&pdev->dev, "failure in registering the device\n");
		ret = -ENODEV;
		goto err_free_irq;
	}

	ret = sysfs_create_group(&new_dev->kobj, &ecap_device_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "failed to create sysfs files\n");
		ret = -ENFILE;
		goto err_unreg_dev;
	}
	dev_set_drvdata(new_dev, ecap);
	return 0;

err_unreg_dev:
	device_unregister(&pdev->dev);
err_free_irq:
	free_irq(irq_start, ecap);
err_free_io_mem:
	iounmap(ecap->mmio_base);
err_free_mem:
	release_mem_region(r->start, resource_size(r));
err_free_clk:
	clk_put(ecap->clk);
err_free_alloc:
	kfree(ecap);
err_ret:
	return ret;
}

#define ecap_resume NULL
#define ecap_suspend NULL

static int __devexit ecap_remove(struct platform_device *pdev)
{
	struct resource *r, *irq;
	struct ecap_dev *ecap;
	struct device *device;
	char name[20];

	scnprintf(name, sizeof name, "%s.%d", "ecap_cap", pdev->id);
	device = capture_request_device(name);
	ecap = dev_get_drvdata(device);

	clk_put(ecap->clk);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, resource_size(r));
	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	free_irq(irq->start, ecap);
	iounmap(ecap->mmio_base);
	sysfs_remove_group(&device->kobj, &ecap_device_attr_group);
	device_unregister(device);
	kfree(ecap);

	return 0;
}

static struct platform_driver ecap_driver = {
	.driver	= {
		.name	= "ecap_cap",
		.owner	= THIS_MODULE,
	},
	.probe		= ecap_probe,
	.remove		= __devexit_p(ecap_remove),
	.suspend	= ecap_suspend,
	.resume		= ecap_resume,
};

static int __init ecap_init(void)
{
	return platform_driver_register(&ecap_driver);
}

static void __exit ecap_exit(void)
{
	platform_driver_unregister(&ecap_driver);
}

module_init(ecap_init);
module_exit(ecap_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("Driver for Davinci eCAP peripheral");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:ecap_cap");
