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

#include <mach/legoev3.h>

#define to_legoev3_analog_device(x) container_of((x), struct legoev3_analog_device, dev)

struct legoev3_analog_device;

struct legoev3_analog_ops  {
	u16 (*get_in_pin1_value)(struct legoev3_analog_device *alg,
				 enum legoev3_input_port port);
	u16 (*get_in_pin6_value)(struct legoev3_analog_device *alg,
				 enum legoev3_input_port port);
	u16 (*get_out_pin5_value)(struct legoev3_analog_device *alg,
				  enum legoev3_output_port port);
	u16 (*get_batt_volt_value)(struct legoev3_analog_device *alg);
	u16 (*get_batt_curr_value)(struct legoev3_analog_device *alg);
	int (*set_nxt_color_read)(struct legoev3_analog_device *alg,
				  enum legoev3_input_port port, bool enable);
	bool (*get_nxt_color_read_busy)(struct legoev3_analog_device *alg,
					enum legoev3_input_port port);
};

struct legoev3_analog_device {
	const char *name;
	struct legoev3_analog_ops *ops;

	/* private */
	struct device *dev;
};

extern int legoev3_analog_device_register(struct device *parent,
					  struct legoev3_analog_device *alg);
extern void legoev3_analog_device_unregister(struct legoev3_analog_device *alg);

extern struct class *legoev3_analog_class;

#endif /* __LINUX_LEGOEV3_ANALOG_H */
