/*
 * On-board bluetooth support for LEGO Mindstorms EV3
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

#ifndef _LINUX_LEGOEV3_BLUETOOTH_H
#define _LINUX_LEGOEV3_BLUETOOTH_H

struct legoev3_bluetooth_platform_data {
	int bt_ena_gpio;
	int bt_ena2_gpio;
	int pic_ena_gpio;
	int pic_rst_gpio;
	int pic_cts_gpio;
	const char *clk_pwm_dev;
};

#endif /* _LINUX_LEGOEV3_BLUETOOTH_H */
