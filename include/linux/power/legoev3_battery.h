/*
 * Battery driver for the LEGO Mindstorms EV3
 *
 * Copyright (c) 2013 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 */

#ifndef __LINUX_POWER_LEGOEV3_BATTERY_H
#define __LINUX_POWER_LEGOEV3_BATTERY_H

struct legoev3_battery_platform_data {
	unsigned int batt_type_gpio;
};

#endif /* __LINUX_POWER_LEGOEV3_BATTERY_H */
