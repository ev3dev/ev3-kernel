/*
 * Analog framework for LEGO Mindstorms EV3
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

#ifndef __LINUX_LEGOEV3_ANALOG_H
#define __LINUX_LEGOEV3_ANALOG_H

#include <linux/spi/spi.h>
#include <mach/legoev3.h>

#define to_legoev3_analog_device(x) container_of((x), struct legoev3_analog_device, dev)

struct legoev3_analog_platform_data {
	u8 in_pin1_ch[LEGOEV3_NUM_PORT_IN];
	u8 in_pin6_ch[LEGOEV3_NUM_PORT_IN];
	u8 out_pin5_ch[LEGOEV3_NUM_PORT_OUT];
	u8 batt_volt_ch;
	u8 batt_curr_ch;
};

struct legoev3_analog_device;

extern u16 legoev3_analog_in_pin1_value(struct legoev3_analog_device *alg,
					enum legoev3_input_port port);
extern u16 legoev3_analog_in_pin6_value(struct legoev3_analog_device *alg,
					enum legoev3_input_port port);
extern u16 legoev3_analog_out_pin5_value(struct legoev3_analog_device *alg,
					 enum legoev3_output_port port);
extern u16 legoev3_analog_batt_volt_value(struct legoev3_analog_device *alg);
extern u16 legoev3_analog_batt_curr_value(struct legoev3_analog_device *alg);

extern struct spi_driver legoev3_analog_driver;

#endif /* __LINUX_LEGOEV3_ANALOG_H */
