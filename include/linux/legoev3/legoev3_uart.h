/*
 * tty line discipline for LEGO Mindstorms EV3 UART sensors
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

#ifndef _LINUX_LEGOEV3_UART_H_
#define _LINUX_LEGOEV3_UART_H_

#define LEGOEV3_UART_NAME_SIZE	11
#define LEGOEV3_UART_UNITS_SIZE	4
#define LEGOEV3_UART_TYPE_MAX	101
#define LEGOEV3_UART_MODE_MAX	7

enum legoev3_uart_data_type {
	LEGOEV3_UART_DATA_8,
	LEGOEV3_UART_DATA_16,
	LEGOEV3_UART_DATA_32,
	LEGOEV3_UART_DATA_FLOAT,
};

/**
 * struct legoev3_uart_sensor_mode_info
 * @mode: The id of this mode. (0-7)
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
struct legoev3_uart_mode_info {
	u8 mode;
	char name[LEGOEV3_UART_NAME_SIZE + 1];
	unsigned raw_min;
	unsigned raw_max;
	unsigned pct_min;
	unsigned pct_max;
	unsigned si_min;
	unsigned si_max;
	char units[LEGOEV3_UART_UNITS_SIZE + 1];
	u8 data_sets;
	enum legoev3_uart_data_type format;
	u8 figures;
	u8 decimals;
	u8 raw_data[32];
};

/**
 * struct legoev3_uart_sensor_platform_data
 * @tty: The TTY device this sensor is connected to.
 * @mode_info: Array of mode information for the sensor.
 * @type: The type id of the sensor.
 * @num_modes: The number of valid modes.
 * @num_view_modes: The number of valid modes for data logging.
 */
struct legoev3_uart_sensor_platform_data {
	struct tty_struct *tty;
	struct legoev3_uart_mode_info *mode_info;
	u8 num_modes;
	u8 num_view_modes;
};

extern int legoev3_uart_get_mode(struct tty_struct *tty);
extern int legoev3_uart_set_mode(struct tty_struct *tty, const u8 mode);

#endif /* _LINUX_LEGOEV3_UART_H_ */
