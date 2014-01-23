/* -------------------------------------------------------------------------
 * i2c-algo-legoev3.c i2c driver algorithms for LEGO MINDSTORMS EV3
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2013 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LINUX_I2C_ALGO_LEGOEV3_H
#define _LINUX_I2C_ALGO_LEGOEV3_H

#include <linux/hrtimer.h>

struct i2c_algo_legoev_data {
	void *data;		/* private data for lowlevel routines */
	void (*setsda) (void *data, int state);
	void (*setsdadir) (void *data, int dir);
	void (*setscl) (void *data, int state);
	int  (*getsda) (void *data);

	int udelay;		/* half clock cycle time in us,
				   minimum 2 us for fast-mode I2C,
				   minimum 5 us for standard-mode I2C and SMBus,
				   maximum 50 us for SMBus */
	int timeout;		/* in jiffies */

	/* private */
	struct hrtimer timer;
};

int i2c_legoev3_add_bus(struct i2c_adapter *);
int i2c_legoev_add_numbered_bus(struct i2c_adapter *);

#endif /* _LINUX_I2C_ALGO_LEGOEV3_H */
