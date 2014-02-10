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

#include <linux/device.h>
#include <linux/types.h>

#define MSENSOR_PORT_NAME_SIZE	30
#define MSENSOR_FW_VERSION_SIZE	8
#define MSENSOR_MODE_NAME_SIZE	11
/* Do not change these 3 values without replacing them in legoev3_uart.c first */
#define MSENSOR_UNITS_SIZE	4
#define MSENSOR_MODE_MAX	7
#define MSENSOR_RAW_DATA_SIZE	32

/*
 * Be sure to add the size to msensor_data_size[] when adding values
 * to msensor_data_type.
 */
enum msensor_data_type {
	MSENSOR_DATA_U8,
	MSENSOR_DATA_S8,
	MSENSOR_DATA_U16,
	MSENSOR_DATA_S16,
	MSENSOR_DATA_S16_BE,
	MSENSOR_DATA_U32,
	MSENSOR_DATA_S32,
	MSENSOR_DATA_FLOAT,
	NUM_MSENSOR_DATA_TYPE
};

extern size_t msensor_data_size[];

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
 * @data_sets: Number of data points in raw data.
 * @data_type: Data type of raw data.
 * @figures: Number of digits that should be displayed, including decimal point.
 * @decimals: Decimal point position.
 * @raw_data: Raw data read from the sensor.
 */
struct msensor_mode_info {
	char name[MSENSOR_MODE_NAME_SIZE + 1];
	int raw_min;
	int raw_max;
	int pct_min;
	int pct_max;
	int si_min;
	int si_max;
	char units[MSENSOR_UNITS_SIZE + 1];
	u8 data_sets;
	enum msensor_data_type data_type;
	u8 figures;
	u8 decimals;
	u8 raw_data[MSENSOR_RAW_DATA_SIZE];
};

/**
 * struct msensor_device
 * @type_id: The type id of the sensor.
 * @port_name: The name of the port that this sensor is connected to.
 * @num_modes: The number of valid modes.
 * @num_view_modes: The number of valid modes for data logging.
 * @mode_info: Array of mode information for the sensor.
 * @get_mode: Callback to get the current sensor mode.
 * @set_mode: Callback to set the sensor mode.
 * @write_data: Write data to sensor.
 * @get_poll_ms: Get the polling period in milliseconds (optional).
 * @set_poll_ms: Set the polling period in milliseconds (optional).
 * @context: Pointer to data structure used by callbacks.
 * @fw_version: Firmware version of sensor (optional).
 * @i2c_addr: I2C address if this is an I2C sensor (optional).
 * @dev: The device data structure.
 */
struct msensor_device {
	u8 type_id;
	char port_name[MSENSOR_PORT_NAME_SIZE + 1];
	u8 num_modes;
	u8 num_view_modes;
	struct msensor_mode_info *mode_info;
	u8 (* get_mode)(void *context);
	int (* set_mode)(void *context, u8 mode);
	ssize_t (* write_data)(void *context, char *data, loff_t off, size_t count);
	int (* get_poll_ms)(void *context);
	int (* set_poll_ms)(void *context, unsigned value);
	void *context;
	char fw_version[MSENSOR_FW_VERSION_SIZE + 1];
	unsigned i2c_addr;
	/* private */
	struct device dev;
};

extern int msensor_ftoi(u32 f, unsigned dp);
extern u32 msensor_itof(int i, unsigned dp);

extern int register_msensor(struct msensor_device *, struct device *);
extern void unregister_msensor(struct msensor_device *);

extern struct class msensor_class;

#endif /* _LINUX_LEGOEV3_MEASURE_SENSOR_CLASS_H */
