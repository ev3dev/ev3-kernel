/*
 * DC motor device class for LEGO MINDSTORMS EV3
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

#ifndef _LINUX_LEGOEV3_DC_MOTOR_CLASS_H
#define _LINUX_LEGOEV3_DC_MOTOR_CLASS_H

#include <linux/device.h>
#include <linux/types.h>

#define DC_MOTOR_NAME_SIZE	30

enum dc_motor_commands {
	DC_MOTOR_COMMAND_FORWARD	= BIT(0),
	DC_MOTOR_COMMAND_REVERSE	= BIT(1),
	DC_MOTOR_COMMAND_COAST		= BIT(2),
	DC_MOTOR_COMMAND_BRAKE		= BIT(3),
};
#define NUM_DC_MOTOR_COMMANDS 4

/**
 * @get_supported_commands: Return the supported commands as bit flags.
 * @get_command: Return the current command or negative error.
 * @set_command: Set the command for the motor. Returns 0 on success or
 * 	negative error;
 * @get_ramp_up_ms: Gets the ramp up time in milliseconds or negative error.
 * 	(optional)
 * @set_ramp_up_ms: Sets the ramp up time in milliseconds. Returns 0 on success
 * 	or negative error. (optional)
 * @get_ramp_down_ms: Gets the ramp down time in milliseconds or negative error.
 * 	(optional)
 * @set_ramp_down_ms: Sets the ramp down time in milliseconds. Returns 0 on
 * 	success or negative error. (optional)
 * @context: Pointer to data structure passed back to the functions.
 */
struct dc_motor_ops {
	unsigned (*get_supported_commands)(void* context);
	int (*get_command)(void* context);
	int (*set_command)(void* context, unsigned command);
	int (*get_ramp_up_ms)(void* context);
	int (*set_ramp_up_ms)(void* context, unsigned ms);
	int (*get_ramp_down_ms)(void* context);
	int (*set_ramp_down_ms)(void* context, unsigned ms);
	void *context;
};

/**
 * struct dc_motor_device
 * @name: The name of dc controller.
 * @port_name: The name of the port that this motor is connected to.
 * @ops: Function pointers to the controller that registered this dc.
 * @context: Data struct passed back to the ops.
 * @min_pulse_ms: The size of the pulse to drive the motor to 0 degrees.
 * @mid_pulse_ms: The size of the pulse to drive the motor to 90 degrees.
 * @max_pulse_ms: The size of the pulse to drive the motor to 180 degrees.
 */
struct dc_motor_device {
	char name[DC_MOTOR_NAME_SIZE + 1];
	char port_name[DC_MOTOR_NAME_SIZE + 1];
	struct dc_motor_ops ops;
	/* private */
	struct device dev;
	unsigned duty_cycle;
};

#define to_dc_motor_device(_dev) container_of(_dev, struct dc_motor_device, dev)

extern int register_dc_motor(struct dc_motor_device *, struct device *);
extern void unregister_dc_motor(struct dc_motor_device *);

#endif /* _LINUX_LEGOEV3_DC_MOTOR_CLASS_H */
