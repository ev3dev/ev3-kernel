/*
 * i2c-legoev3 interface to platform code
 *
 * Copyright (C) 2013 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_I2C_LEGOEV3_H
#define _LINUX_I2C_LEGOEV3_H

/**
 * struct i2c_legoev3_platform_data - Platform-dependent data for i2c-legoev3
 * @sda_pin: GPIO pin ID to use for SDA
 * @scl_pin: GPIO pin ID to use for SCL
 */
struct i2c_legoev3_platform_data {
	unsigned int	sda_pin;
	unsigned int	scl_pin;
};

#endif /* _LINUX_I2C_LEGOEV3_H */
