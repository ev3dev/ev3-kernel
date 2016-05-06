/*
 * Global defines for LEGO MINDSTORMS EV3
 *
 * Copyright (C) 2013 Ralph Hempel
 * Copyright (C) 2013-2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_DAVINCI_LEGOEV3_H
#define __ASM_ARCH_DAVINCI_LEGOEV3_H

enum legoev3_input_port_id {
	EV3_PORT_IN1,
	EV3_PORT_IN2,
	EV3_PORT_IN3,
	EV3_PORT_IN4,
	NUM_EV3_PORT_IN,
};

enum legoev3_output_port_id {
	EV3_PORT_OUT1,
	EV3_PORT_OUT2,
	EV3_PORT_OUT3,
	EV3_PORT_OUT4,
	NUM_EV3_PORT_OUT,
};

#endif /* __ASM_ARCH_DAVINCI_LEGOEV3_H */
