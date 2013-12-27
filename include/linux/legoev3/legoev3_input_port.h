/*
 * Touch sensor device class for LEGO Mindstorms EV3
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

#ifndef __LINUX_LEGOEV3_INPUT_PORT_H
#define __LINUX_LEGOEV3_INPUT_PORT_H

struct legoev3_input_port_device {
	int (*pin1_mv)(struct legoev3_input_port_device *ipd);
	/* private */
	struct device dev;
};

#endif /* __LINUX_LEGOEV3_INPUT_PORT_H */
