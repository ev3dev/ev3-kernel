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

#endif /* _LINUX_LEGOEV3_UART_H_ */
