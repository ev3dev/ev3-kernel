/*
 * Mindstorms sensor device class for LEGO Mindstorms EV3
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

#ifndef _LINUX_LEGOEV3_MSENSOR_CLASS_H
#define _LINUX_LEGOEV3_MSENSOR_CLASS_H

#define MSENSOR_NAME_SIZE	11
#define MSENSOR_UNITS_SIZE	4
#define MSENSOR_MODE_MAX	7
#define MSENSOR_RAW_DATA_SIZE	32

enum legoev3_msensor_data_type {
	MSENSOR_DATA_8		= 0,
	MSENSOR_DATA_16		= 1,
	MSENSOR_DATA_32		= 2,
	MSENSOR_DATA_FLOAT	= 3,
};

extern size_t legoev3_msensor_data_size[];

/**
 * struct msensor_mode_info
 * @name: The name of this mode
 * @raw_min: The minimum raw value of the data read.
 * @raw_min: The maximum raw value of the data read.
 * @pct_min: The minimum percentage value of the data read.
 * @pct_min: The maximum percentage value of the data read.
 * @si_min: The minimum scaled value of the data read.
 * @si_min: The maximum scaled value of the data read.
 * @units: Units of the scaled value.
 * @data_sets: Number of data points in DATA message.
 * @format: Format of data in DATA message.
 * @figures: Number of digits that should be displayed, including decimal point.
 * @decimals: Decimal point position.
 * @raw_data: Raw data read from the sensor.
 *
 * All of the min an max values are actually 32-bit float data type,
 * but kernel drivers don't do floating point, so we use unsigned instead.
 */
struct msensor_mode_info {
	char name[MSENSOR_NAME_SIZE + 1];
	unsigned raw_min;
	unsigned raw_max;
	unsigned pct_min;
	unsigned pct_max;
	unsigned si_min;
	unsigned si_max;
	char units[MSENSOR_UNITS_SIZE + 1];
	u8 data_sets;
	enum legoev3_msensor_data_type format;
	u8 figures;
	u8 decimals;
	u8 raw_data[MSENSOR_RAW_DATA_SIZE];
};

/**
 * struct msensor_device
 * @type_id: The type id of the sensor.
 * @num_modes: The number of valid modes.
 * @num_view_modes: The number of valid modes for data logging.
 * @mode_info: Array of mode information for the sensor.
 * @get_mode: Callback to get the current sensor mode.
 * @set_mode: Callback to set the sensor mode.
 * @write_data: Write data to sensor.
 * @context: Pointer to data structure used by callbacks.
 * @dev: The device data structure.
 */
struct msensor_device {
	u8 type_id;
	u8 num_modes;
	u8 num_view_modes;
	struct msensor_mode_info *mode_info;
	u8 (* get_mode)(void *context);
	int (* set_mode)(void *context, u8 mode);
	ssize_t (* write_data)(void *context, char *data, loff_t off, size_t count);
	void *context;
	/* private */
	struct device dev;
};

extern int register_msensor(struct msensor_device *, struct device *);
extern void unregister_msensor(struct msensor_device *);

extern struct class msensor_class;

#endif /* _LINUX_LEGOEV3_MEASURE_SENSOR_CLASS_H */
