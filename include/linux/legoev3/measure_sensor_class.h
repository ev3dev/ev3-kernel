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

/**
 * struct measure_sensor_scale_info
 * @units: The name of the units of the scaled value.
 * @min: The minimum scaled value.
 * @max: The maximum scaled value.
 * @dp: The number of decimal places in the scaled value.
 *
 * The scaled value is calculated as:
 * scaled_value = cal_value * (scaled_max - scaled_min) / (cal_max - cal_min)
 *
 * The actual scaled value in units is scaled_value / pow(10, dp), but
 * kernel code doesn't do floating point, so it is up to the user to do
 * this last part of the conversion if they want to use the actual value.
 */
struct measure_sensor_scale_info {
	const char *units;
	int min;
	int max;
	unsigned dp;
};

/**
 * struct measure_sensor_device - Measurement sensor device
 * @raw_value: Function that returns the current uncalibrated raw value
 *	of the sensor.
 * @raw_min: The minimum uncalibrated raw value of the sensor.
 * @raw_max: The maximum uncalibrated raw value of the sensor.
 * @cal_value: Function that returns the current calibrated raw value
 *	of the sensor.
 * @cal_min: The minimum calibrated raw value of the sensor.
 * @cal_max: The maximum calibrated raw value of the sensor.
 * @scale_info: Pointer to an array of scaling information for the sensor.
 *	Array must be terminated with END_SCALE_INFO.
 * @scale_idx: The index of the currently selected scaling.
 * @dev: The sysfs device.
 */
struct measure_sensor_device {
	const char *name;
	int id;
	int (*raw_value)(struct measure_sensor_device *);
	int raw_min;
	int raw_max;
	int (*cal_value)(struct measure_sensor_device *);
	int cal_min;
	int cal_max;
	struct measure_sensor_scale_info *scale_info;
	unsigned scale_idx;
	/* private */
	struct device dev;
};

#define END_SCALE_INFO { .units = NULL }

extern int register_measure_sensor(struct measure_sensor_device *,
				   struct device *);
extern void unregister_measure_sensor(struct measure_sensor_device *);

extern struct class measure_sensor_class;

#endif /* _LINUX_LEGOEV3_MEASURE_SENSOR_CLASS_H */
