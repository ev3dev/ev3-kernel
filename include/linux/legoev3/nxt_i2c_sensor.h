/*
 * NXT i2c sensor shared defines for LEGO Mindstorms EV3
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

#ifndef _LINUX_LEGOEV3_NXT_I2C_SENSOR_H
#define _LINUX_LEGOEV3_NXT_I2C_SENSOR_H

#include <linux/i2c.h>

#define FIRMWARE_REG		0x00	/* Firmware version (8 registers) */
#define	VENDOR_ID_REG		0x08	/* Vendor ID (8 registers) */
#define DEVICE_ID_REG		0x10	/* Device ID (8 registers) */
#define FACTORY_ZERO_REG	0x18
#define FACTORY_SCALE_REG	0x19
#define FACTORY_DIV_REG		0x1A
#define UNITS_REG		0x1B	/* Measurement units (7 registers) */
#define VAR_REG_BASE		0x40
#define CMD_REG_BASE		0x80

#define ID_STR_LEN	8	/* length of ID string registers */
#define UNITS_STR_LEN	7

/* these are aliases for smbus commands so that we know which ones to use */
static inline int nxt_i2c_read_byte(struct i2c_client *client, unsigned reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline int nxt_i2c_read_word(struct i2c_client *client, unsigned reg)
{
	return i2c_smbus_read_word_data(client, reg);
}

static inline int nxt_i2c_read_string(struct i2c_client *client, unsigned reg,
				      char *buf, unsigned len)
{
	return i2c_smbus_read_i2c_block_data(client, reg, len, buf);
}

static inline int nxt_i2c_test_id_string(struct i2c_client *client, unsigned reg,
					 const char *expected)
{
	char id_str[ID_STR_LEN + 1] = { 0 };
	int ret;

	ret =  nxt_i2c_read_string(client, reg, id_str, ID_STR_LEN);
	if (ret < 0)
		return ret;

	return strcmp(id_str, expected);
}

#endif /* _LINUX_LEGOEV3_NXT_I2C_SENSOR_H */
