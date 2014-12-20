/*
 * LEGO LED device class
 *
 * Copyright (C) 2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LEGO_LED_CLASS_H_
#define _LEGO_LED_CLASS_H_

#include <linux/leds.h>

#include <lego.h>

struct lego_led_device {
	const char *name;
	const char *port_name;
	struct led_classdev led_cdev;
};

extern int lego_led_register(struct device *parent, struct lego_led_device *led);
extern void lego_led_unregister(struct lego_led_device *led);

#define to_lego_led_device(_dev) \
	container_of(dev_get_drvdata(dev), struct lego_led_device, led_cdev)

#endif /* _LEGO_SENSOR_CLASS_H_ */
