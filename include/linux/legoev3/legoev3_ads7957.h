/*
 * Support for TI ADS7957 A/D converter on LEGO Mindstorms EV3 platform
 *
 * Copyright (C) 2013 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_LEGOEV3_ADS79XX_H
#define __LINUX_LEGOEV3_ADS79XX_H

#include <mach/legoev3.h>

struct legoev3_ads7957_platform_data {
	u8 in_pin1_ch[LEGOEV3_NUM_PORT_IN];
	u8 in_pin6_ch[LEGOEV3_NUM_PORT_IN];
	u8 out_pin5_ch[LEGOEV3_NUM_PORT_OUT];
	u8 batt_volt_ch;
	u8 batt_curr_ch;
};

#endif /* __LINUX_LEGOEV3_ADS79XX_H */
