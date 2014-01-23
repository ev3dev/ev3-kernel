/*
 * Switch sensor device class for LEGO Mindstorms EV3
 *
 * Copyright (C) 2013-2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_LEGOEV3_SWITCH_SENSOR_CLASS_H
#define __LINUX_LEGOEV3_SWITCH_SENSOR_CLASS_H

#include <linux/device.h>


struct switch_sensor_device {
	bool (*get_value)(struct switch_sensor_device *);
	/* private */
	struct device dev;
};

extern int register_switch_sensor(struct switch_sensor_device *,
				 struct device *);
extern void unregister_switch_sensor(struct switch_sensor_device *);

extern struct class switch_sensor_class;

#endif /* __LINUX_LEGOEV3_SWITCH_SENSOR_CLASS_H */
