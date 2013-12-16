/*
 * Support for TI ADS79xx family of A/D converters
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

#ifndef __LINUX_HWMON_ADS79XX_H
#define __LINUX_HWMON_ADS79XX_H

#define ADS79XX_RANGE_2_5V	0x0
#define ADS79XX_RANGE_5V	0x1

#define ADS79XX_MODE_MANUAL	0x1
#define ADS79XX_MODE_AUTO	0x2

struct ads79xx_platform_data {
	u8 range;
	u8 mode;
	u64 auto_update_ns;
	u32 vref_mv;
};

struct ads79xx_device;

extern u32 ads79xx_get_data_for_ch(struct ads79xx_device *ads, u8 channel);

#endif /* __LINUX_HWMON_ADS79XX_H */
