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

/*
 * Note: The comment block below is used to generate docs on the ev3dev website.
 * Use kramdown (markdown) syntax. Use a '.' as a placeholder when blank lines
 * or leading whitespace is important for the markdown syntax.
 */

/**
 * DOC: website
 *
 * LEGO LED Class
 *
* The 'lego-led-class` module is a bit different than the other LEGO drivers.
* Rather than being an actual device class, it is a wrapper around the existing
* Linux [leds-class].
* .
* ### Identifying LEDs
* .
* Since the name of the `sensor<N>` device node does not correspond to the port
* that a sensor is plugged in to, you must look at the port_name attribute if
* you need to know which port a sensor is plugged in to. However, if you don't
* have more than one sensor of each type, you can just look for a matching
* `name`. Then it will not matter which port a sensor is plugged in to - your
* program will still work. In the case of I2C sensors, you may also want to
* check the `address` since it is possible to have more than one I2C sensor
* connected to a single input port at a time.
* .
* ### sysfs Attributes
* .
* LEDs can be found at `/sys/class/leds/<port>:<color>:<name>`, where
* `<port>` is the port that the LED is connected to, <color> is the color of
* the LED or omitted when the color is unknown and <name> differentiates
* between LEDs connected to the same port.
* .
* `brightness`: (read/write)
* : Reading returns the current brightness value. Writing controls the
*   brightness of the LED. Valid values are 0 (off) to `max_brightness` (100%).
* .
* `delay_on`: (read/write)
* : Only present when trigger is set to timer. Use `timer_delay_on` instead.
* .
* `delay_off`: (read/write)
* : Only present when trigger is set to timer. Use `timer_delay_off` instead.
* .
* `device_name` (read-only)
* : Returns the name of the sensor device/driver.
* .
* `max_brightness`: (read-only)
* : The maximum possible brightness value of the LED.
* .
* `port_name` (read-only)
* : Returns the name of the port that the sensor is connected to.
* .
* `timer_delay_on`: (read/write)
* : Reading returns the current delay. Writing sets the duration of the off
*   cycle when `trigger` is set to timer. Returns an error if `trigger` is not
*   set to timer.
* .
* `timer_delay_off`: (read/write)
* : Reading returns the current delay. Writing sets the duration of the on
*   cycle when `trigger` is set to timer. Returns an error if `trigger` is not
*   set to timer.
* .
* `trigger`: (read-write)
* : Reading returns a list of available triggers with the current trigger
*   indicated by square brackets. Writing sets the trigger.
* .
* [leds-class]: https://www.kernel.org/doc/Documentation/leds/leds-class.txt
*/

#include <linux/device.h>
#include <linux/leds.h>
#include <linux/module.h>

#include <lego_led_class.h>

static ssize_t device_name_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lego_led_device *led = to_lego_led_device(dev);

	return snprintf(buf, LEGO_NAME_SIZE, "%s\n", led->name);
}

static ssize_t port_name_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct lego_led_device *led = to_lego_led_device(dev);

	return snprintf(buf, LEGO_PORT_NAME_SIZE, "%s\n", led->port_name);
}

DEVICE_ATTR_RO(device_name);
DEVICE_ATTR_RO(port_name);

static struct attribute *lego_led_attrs[] = {
	&dev_attr_device_name.attr,
	&dev_attr_port_name.attr,
};

ATTRIBUTE_GROUPS(lego_led);

static int lego_led_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct lego_led_device *led = to_lego_led_device(dev);
	int ret;

	ret = add_uevent_var(env, "LEGO_DEVICE_NAME=%s", led->name);
	if (ret) {
		dev_err(dev, "failed to add uevent LEGO_DEVICE_NAME\n");
		return ret;
	}

	add_uevent_var(env, "LEGO_PORT_NAME=%s", led->port_name);
	if (ret) {
		dev_err(dev, "failed to add uevent LEGO_PORT_NAME\n");
		return ret;
	}

	return 0;
}

const struct device_type lego_led_type = {
	.name	= "lego-led",
	.groups	= lego_led_groups,
	.uevent	= lego_led_uevent,
};

int lego_led_register(struct device *parent, struct lego_led_device *led)
{
	if (!led || !led->name || !led->port_name)
		return -EINVAL;

	//led->led_cdev.dev->type = lego_led_type;

	return led_classdev_register(parent, &led->led_cdev);
}
EXPORT_SYMBOL_GPL(lego_led_register);

void lego_led_unregister(struct lego_led_device *led)
{
	led_classdev_unregister(&led->led_cdev);
}
EXPORT_SYMBOL_GPL(lego_led_unregister);

MODULE_DESCRIPTION("LEGO LED device class");
MODULE_AUTHOR("David Lechner <david@lechnology.com>");
MODULE_LICENSE("GPL");
