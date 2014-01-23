/*
 * Sensor controls device class for LEGO Mindstorms EV3
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

#ifndef _LINUX_LEGOEV3_SENSOR_CONTROLS_CLASS_H
#define _LINUX_LEGOEV3_SENSOR_CONTROLS_CLASS_H

/**
 * struct sensor_controls_mode_info
 * @name: The name of the mode.
 * @id: Numeric identifier for the mode.
 */
struct sensor_controls_mode_info {
	const char *name;
	int id;
};

/**
 * struct sensor_controls_device - Sensor controls device
 * @name: Name of the sensor control device used for device node name.
 * @id: Unique identifier used for device node name.
 * @mode_info: Pointer to an array of mode information for the sensor.
 *	Array must be terminated with END_MODE_INFO.
 * @get_mode: Function that returns the current mode of the sensor.
 * @set_mode: Function that sets the current mode of the sensor.
 * @dev: The sysfs device.
 *
 * If we add other controls, mode will become optional.
 */
struct sensor_controls_device {
	const char *name;
	int id;
	struct sensor_controls_mode_info *mode_info;
	int (*get_mode)(struct sensor_controls_device *);
	int (*set_mode)(struct sensor_controls_device *, int mode);
	/* private */
	struct device dev;
};

#define END_MODE_INFO { .name = NULL }

extern int register_sensor_controls(struct sensor_controls_device *,
				    struct device *);
extern void unregister_sensor_controls(struct sensor_controls_device *);

extern struct class sensor_controls_class;

#endif /* _LINUX_LEGOEV3_SENSOR_CONTROLS_CLASS_H */
