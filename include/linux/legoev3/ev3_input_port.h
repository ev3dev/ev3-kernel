/*
 * Input Port driver for LEGO Mindstorms EV3
 *
 * Copyright (C) 2013-2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_LEGOEV3_EV3_INPUT_PORT_H
#define __LINUX_LEGOEV3_EV3_INPUT_PORT_H

#include <mach/legoev3.h>

struct ev3_input_port_platform_data {
	enum ev3_input_port_id id;
	unsigned pin1_gpio;
	unsigned pin2_gpio;
	unsigned pin5_gpio;
	unsigned pin6_gpio;
	unsigned buf_ena_gpio;
	unsigned i2c_clk_gpio;
	unsigned i2c_dev_id;
	unsigned i2c_pin_mux;
	unsigned uart_pin_mux;
};

struct ev3_input_port_device {
	int (*pin1_mv)(struct ev3_input_port_device *ipd);
	int (*pin6_mv)(struct ev3_input_port_device *ipd);
	/* private */
	struct device dev;
};

/* resistor ids for EV3 dumb sensor devices */
enum ev3_in_dev_id {
	EV3_IN_DEV_ID_01,
	EV3_IN_DEV_ID_02,
	EV3_IN_DEV_ID_03,
	EV3_IN_DEV_ID_04,
	EV3_IN_DEV_ID_05,
	EV3_IN_DEV_ID_06,
	EV3_IN_DEV_ID_07,
	EV3_IN_DEV_ID_08,
	EV3_IN_DEV_ID_09,
	EV3_IN_DEV_ID_10,
	EV3_IN_DEV_ID_11,
	EV3_IN_DEV_ID_12,
	EV3_IN_DEV_ID_13,
	EV3_IN_DEV_ID_14,
	NUM_EV3_IN_DEV_ID,
	EV3_IN_DEV_ID_ERR = -1
};

#endif /* __LINUX_LEGOEV3_EV3_INPUT_PORT_H */
