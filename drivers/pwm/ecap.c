/*
 * eCAP driver for PWM output generation
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Modifications for ev3dev:
 * Copyright (C) 2014 David Lechner <david@lechnology.com>
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/pwm/pwm.h>
#include <linux/slab.h>

#define TIMER_CTR_REG			0x00
#define PHASE_CTR_REG			0x04
#define CAPTURE_1_REG			0x08
#define CAPTURE_2_REG			0x0C
#define CAPTURE_3_REG			0x10
#define CAPTURE_4_REG			0x14
#define CAPTURE_CTRL2_REG		0x2A

#define ECTRL2_APWMPOL			BIT(10)
#define ECTRL2_APWM			BIT(9)
#define ECTRL2_SYNCO_SEL		(0x03 << 6)
#define ECTRL2_SYNCI_EN			BIT(5)
#define ECTRL2_TSCTRSTOP		BIT(4)

struct ecap_pwm {
	struct pwm_device pwm;
	struct pwm_device_ops ops;
	spinlock_t	lock;
	struct clk	*clk;
	void __iomem	*mmio_base;
};

static inline struct ecap_pwm *to_ecap_pwm(const struct pwm_device *p)
{
	return pwm_get_drvdata(p);
}

static int ecap_pwm_stop(struct pwm_device *p)
{
	unsigned long flags;
	struct ecap_pwm *ep = to_ecap_pwm(p);

	if (p->flags & FLAG_RUNNING)
		return 0;

	spin_lock_irqsave(&ep->lock, flags);
	__raw_writew(__raw_readw(ep->mmio_base + CAPTURE_CTRL2_REG) &
		~ECTRL2_TSCTRSTOP, ep->mmio_base + CAPTURE_CTRL2_REG);
	spin_unlock_irqrestore(&ep->lock, flags);

	clear_bit(FLAG_RUNNING, &p->flags);

	return 0;
}

static int ecap_pwm_start(struct pwm_device *p)
{
	int ret = 0;
	unsigned long flags;
	struct ecap_pwm *ep = to_ecap_pwm(p);

	if (!(p->flags & FLAG_RUNNING))
		return 0;

	spin_lock_irqsave(&ep->lock, flags);
	__raw_writew(__raw_readw(ep->mmio_base + CAPTURE_CTRL2_REG) |
		ECTRL2_TSCTRSTOP, ep->mmio_base + CAPTURE_CTRL2_REG);
	spin_unlock_irqrestore(&ep->lock, flags);
	set_bit(FLAG_RUNNING, &p->flags);

	return ret;
}

static int ecap_pwm_set_polarity(struct pwm_device *p, char pol)
{
	unsigned long flags;
	struct ecap_pwm *ep = to_ecap_pwm(p);

	spin_lock_irqsave(&ep->lock, flags);
	 __raw_writew((__raw_readw(ep->mmio_base + CAPTURE_CTRL2_REG) &
		 ~ECTRL2_APWMPOL) | (!pol << 10), ep->mmio_base + CAPTURE_CTRL2_REG);
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}

static int ecap_pwm_config_period(struct pwm_device *p)
{
	unsigned long flags;
	struct ecap_pwm *ep = to_ecap_pwm(p);

	spin_lock_irqsave(&ep->lock, flags);
	__raw_writel((p->period_ticks) - 1, ep->mmio_base + CAPTURE_3_REG);
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}

static int ecap_pwm_config_duty(struct pwm_device *p)
{
	unsigned long flags;
	struct ecap_pwm *ep = to_ecap_pwm(p);

	spin_lock_irqsave(&ep->lock, flags);
	if (p->duty_ticks > 0)
		__raw_writel(p->duty_ticks, ep->mmio_base + CAPTURE_4_REG);
	else {
		__raw_writel(p->duty_ticks, ep->mmio_base + CAPTURE_2_REG);
		__raw_writel(0, ep->mmio_base + TIMER_CTR_REG);
	}
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}

static int ecap_pwm_config(struct pwm_device *p,
				struct pwm_config *c)
{
	int ret = 0;
	switch (c->config_mask) {

	case BIT(PWM_CONFIG_DUTY_TICKS):
		p->duty_ticks = c->duty_ticks;
		ret = ecap_pwm_config_duty(p);
		break;

	case BIT(PWM_CONFIG_PERIOD_TICKS):
		p->period_ticks = c->period_ticks;
		ret = ecap_pwm_config_period(p);
		break;

	case BIT(PWM_CONFIG_POLARITY):
		ret = ecap_pwm_set_polarity(p, c->polarity);
		break;

	case BIT(PWM_CONFIG_START):
		ret = ecap_pwm_start(p);
		break;

	case BIT(PWM_CONFIG_STOP):
		ret = ecap_pwm_stop(p);
		break;
	}

	return ret;
}

static int ecap_pwm_request(struct pwm_device *p)
{
	struct ecap_pwm *ep = to_ecap_pwm(p);

	p->tick_hz = clk_get_rate(ep->clk);
	return 0;
}

static int ecap_frequency_transition_cb(struct pwm_device *p)
{
	struct ecap_pwm *ep = to_ecap_pwm(p);
	unsigned long duty_ns;

	p->tick_hz = clk_get_rate(ep->clk);
	duty_ns = p->duty_ns;
	if (pwm_is_running(p)) {
		pwm_stop(p);
		pwm_set_duty_ns(p, 0);
		pwm_set_period_ns(p, p->period_ns);
		pwm_set_duty_ns(p, duty_ns);
		pwm_start(p);
	} else {
		pwm_set_duty_ns(p, 0);
		pwm_set_period_ns(p, p->period_ns);
		pwm_set_duty_ns(p, duty_ns);
	}
		return 0;
}

static int __devinit ecap_probe(struct platform_device *pdev)
{
	struct ecap_pwm *ep = NULL;
	struct resource *r;
	int ret = 0;

	ep = kzalloc(sizeof(*ep), GFP_KERNEL);
	if (!ep) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_ecap_pwm_alloc;
	}

	ep->clk = clk_get(&pdev->dev, "ecap");
	if (IS_ERR(ep->clk)) {
		ret = PTR_ERR(ep->clk);
		goto err_free;
	}

	spin_lock_init(&ep->lock);
	ep->ops.config = ecap_pwm_config;
	ep->ops.request = ecap_pwm_request;
	ep->ops.freq_transition_notifier_cb = ecap_frequency_transition_cb;
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&pdev->dev, "no memory resource defined\n");
		ret = -ENODEV;
		goto err_free_clk;
	}

	r = request_mem_region(r->start, resource_size(r), pdev->name);
	if (!r) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto err_free_clk;
	}

	ep->mmio_base = ioremap(r->start, resource_size(r));
	if (!ep->mmio_base) {
		dev_err(&pdev->dev, "failed to ioremap() registers\n");
		ret = -ENODEV;
		goto err_free_mem;
	}

	__raw_writew(ECTRL2_APWM | ECTRL2_SYNCO_SEL,
		     ep->mmio_base + CAPTURE_CTRL2_REG);
	__raw_writel(0, ep->mmio_base + TIMER_CTR_REG);
	__raw_writel(0, ep->mmio_base + PHASE_CTR_REG);
	__raw_writel(~0, ep->mmio_base + CAPTURE_1_REG); /* period */
	__raw_writel(0, ep->mmio_base + CAPTURE_2_REG); /* duty */

	ep->pwm.ops = &ep->ops;
	pwm_set_drvdata(&ep->pwm, ep);
	ret =  pwm_register(&ep->pwm, &pdev->dev, -1);
	platform_set_drvdata(pdev, ep);
	return 0;

err_free_mem:
	release_mem_region(r->start, resource_size(r));
err_free_clk:
	clk_put(ep->clk);
err_free:
	kfree(ep);
err_ecap_pwm_alloc:
	return ret;
}

#ifdef CONFIG_PM
static int ecap_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ecap_pwm *ep = platform_get_drvdata(pdev);

	clk_disable(ep->clk);

	return 0;
}

static int ecap_resume(struct platform_device *pdev)
{
	struct ecap_pwm *ep = platform_get_drvdata(pdev);

	clk_enable(ep->clk);

	return 0;
}

#else
#define ecap_suspend NULL
#define ecap_resume NULL
#endif

static int __devexit ecap_remove(struct platform_device *pdev)
{
	struct ecap_pwm *ep = platform_get_drvdata(pdev);
	struct resource *r;

	pwm_unregister(&ep->pwm);
	iounmap(ep->mmio_base);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, resource_size(r));
	platform_set_drvdata(pdev, NULL);
	clk_put(ep->clk);
	kfree(ep);

	return 0;
}

static struct platform_driver ecap_driver = {
	.driver	= {
		.name	= "ecap",
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
MODULE_ALIAS("platform:ecap");
