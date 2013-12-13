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

#ifndef __LINUX_PLATFORM_DATA_ADS79XX_H
#define __LINUX_PLATFORM_DATA_ADS79XX_H

#define ADS79XX_MAX_CHANNELS 16

struct ads79xx_platform_data {
	u8 range[ADS79XX_MAX_CHANNELS];
	u32 vref;
};

#endif /* __LINUX_PLATFORM_DATA_ADS79XX_H */
