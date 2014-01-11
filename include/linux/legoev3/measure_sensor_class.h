/*
 * Measurement sensor device class for LEGO Mindstorms EV3
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

#ifndef _LINUX_LEGOEV3_MEASURE_SENSOR_CLASS_H
#define _LINUX_LEGOEV3_MEASURE_SENSOR_CLASS_H

struct measure_sensor_device {
	int (*raw_value)(struct measure_sensor_device *);
	/* private */
	struct device dev;
};

extern int register_measure_sensor(struct measure_sensor_device *,
				   struct device *);
extern void unregister_measure_sensor(struct measure_sensor_device *);

extern struct class measure_sensor_class;

#endif /* _LINUX_LEGOEV3_MEASURE_SENSOR_CLASS_H */
