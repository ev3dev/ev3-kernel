/*
 * GPIO PWM driver
 *
 * Copyright (C) 2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/platform_data/pwm-gpio.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define PWM_GPIO_NAME_SIZE 30

struct pwm_gpio_chip {
	struct pwm_chip chip;
};

struct pwm_gpio_device {
	char name[PWM_GPIO_NAME_SIZE + 1];
	struct pwm_device *pwm;
	struct hrtimer timer;
	struct work_struct work;
	unsigned gpio;
	unsigned gpio_value:1;
	unsigned enabled:1;
};

static void pwm_gpio_work (struct work_struct *work)
{
	struct pwm_gpio_device *data =
			container_of(work, struct pwm_gpio_device, work);

	gpio_direction_output(data->gpio, data->gpio_value);
}

static enum hrtimer_restart pwm_gpio_handle_timer(struct hrtimer *timer)
{
	struct pwm_gpio_device *data =
			container_of(timer, struct pwm_gpio_device, timer);
	ktime_t tnew;

	if (data->gpio_value ^ pwm_get_polarity(data->pwm))
		tnew = ktime_set(0, pwm_get_period(data->pwm)
				    - pwm_get_duty_cycle(data->pwm));
	else
		tnew = ktime_set(0, pwm_get_duty_cycle(data->pwm));
	hrtimer_forward_now(&data->timer, tnew);

	data->gpio_value = !data->gpio_value;
	if (gpio_cansleep(data->gpio))
		schedule_work(&data->work);
	else
		gpio_direction_output(data->gpio, data->gpio_value);

	return HRTIMER_RESTART;
}

static int pwm_gpio_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	struct pwm_gpio_device *data = pwm_get_chip_data(pwm);

	if (duty_ns == 0 || duty_ns == period_ns || !data->enabled) {
		hrtimer_cancel(&data->timer);
		data->gpio_value = pwm_get_polarity(data->pwm);
		if (duty_ns == period_ns)
			data->gpio_value = !data->gpio_value;
		gpio_direction_output(data->gpio, data->gpio_value);
	} else
		hrtimer_start(&data->timer, ktime_set(0, 0), HRTIMER_MODE_REL);

	return 0;
}

static int pwm_gpio_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm,
				 enum pwm_polarity polarity)
{
	return pwm_gpio_config(chip, pwm, pwm_get_duty_cycle(pwm),
			       pwm_get_period(pwm));
}

static int pwm_gpio_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct pwm_gpio_device *data = pwm_get_chip_data(pwm);

	data->enabled= true;
	return pwm_gpio_config(chip, pwm, pwm_get_duty_cycle(pwm),
			       pwm_get_period(pwm));
}

static void pwm_gpio_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct pwm_gpio_device *data = pwm_get_chip_data(pwm);

	data->enabled= false;
	pwm_gpio_config(chip, pwm, 0, 1);
}

static struct pwm_ops pwm_gpio_device_ops = {
	.config = pwm_gpio_config,
	.set_polarity = pwm_gpio_set_polarity,
	.enable = pwm_gpio_enable,
	.disable = pwm_gpio_disable,
	.owner = THIS_MODULE,
};

static int init_pwm_gpio(struct pwm_device *pwm, struct pwm_gpio_device *data,
			 struct pwm_gpio *pdata)
{
	int ret;

	strncpy(data->name, pdata->name, PWM_GPIO_NAME_SIZE);
	data->gpio = pdata->gpio;
	ret = gpio_request_one(pdata->gpio, pwm_get_polarity(pwm) ?
			       GPIOF_INIT_HIGH : GPIOF_INIT_LOW, data->name);
	if (ret < 0)
		return ret;
	hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->timer.function = pwm_gpio_handle_timer;
	INIT_WORK(&data->work, pwm_gpio_work);
	pwm_set_chip_data(pwm, data);
	data->pwm = pwm;

	return 0;
}

static void free_pwm_gpio(struct pwm_device *pwm) {
	struct pwm_gpio_device *data = pwm_get_chip_data(pwm);

	hrtimer_cancel(&data->timer);
	cancel_work_sync(&data->work);
	gpio_direction_input(data->gpio);
	gpio_free(data->gpio);
}

static int pwm_gpio_probe(struct platform_device *pdev)
{
	struct pwm_gpio_platform_data *pdata;
	struct pwm_gpio_chip *chip;
	struct pwm_gpio_device *pwms;
	int ret, i;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		dev_err(&pdev->dev, "missing platform_data\n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&pdev->dev, sizeof(struct pwm_gpio_chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwms = devm_kzalloc(&pdev->dev, pdata->num_pwms
		* sizeof(struct pwm_gpio_device), GFP_KERNEL);
	if (!pwms) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_pwms_devm_kzalloc;
	}

	platform_set_drvdata(pdev, chip);

	chip->chip.dev = &pdev->dev;
	chip->chip.ops = &pwm_gpio_device_ops;
	chip->chip.base = -1;
	chip->chip.npwm = pdata->num_pwms;

	ret = pwmchip_add(&chip->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		goto err_pwmchip_add;
	}

	for (i = 0; i < pdata->num_pwms; i++) {
		ret = init_pwm_gpio(&chip->chip.pwms[i], &pwms[i], &pdata->pwms[i]);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request gpio %u\n",
				pdata->pwms[i].gpio);
			for (i = i - 1; i >= 0; i--)
				free_pwm_gpio(&chip->chip.pwms[i]);
			goto err_init_pwm_gpio;
		}
		chip->chip.can_sleep |= gpio_cansleep(pdata->pwms[i].gpio);
	}

	return 0;

err_init_pwm_gpio:
	pwmchip_remove(&chip->chip);
err_pwmchip_add:
	devm_kfree(&pdev->dev, pwms);
err_pwms_devm_kzalloc:
	devm_kfree(&pdev->dev, chip);
	return ret;
}

static int pwm_gpio_remove(struct platform_device *pdev)
{
	struct pwm_gpio_chip *chip = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < chip->chip.npwm; i++)
		free_pwm_gpio(&chip->chip.pwms[i]);

	return pwmchip_remove(&chip->chip);
	platform_set_drvdata(pdev, NULL);
}

static struct platform_driver pwm_gpio_driver = {
	.driver = {
		.name = "pwm-gpio",
		.owner = THIS_MODULE,
	},
	.probe = pwm_gpio_probe,
	.remove = pwm_gpio_remove,
};
module_platform_driver(pwm_gpio_driver);

MODULE_DESCRIPTION("GPIO PWM driver");
MODULE_AUTHOR("David Lechner <david@lechnology.com>");
MODULE_LICENSE("GPL");
