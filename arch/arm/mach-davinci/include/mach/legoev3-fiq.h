/*
 * FIQ backend for I2C bus driver for LEGO Mindstorms EV3
 * Copyright (C) 2013-2014 David Lechner <david@lechnology.com>
 *
 * Based on davinci_iic.c from lms2012
 * The file does not contain a copyright, but comes from the LEGO Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MACH_LEGOEV3_FIQ_H
#define __MACH_LEGOEV3_FIQ_H

#include <linux/i2c.h>
#include <mach/legoev3.h>

/**
 * struct legoev3_fiq_platform_data - platform specific data
 * @status_gpio: GPIO that is not physically connected that can be used to
 *	to generate interrupts so that the FIQ can notify external code
 *	that its status has changed.
 */
struct legoev3_fiq_platform_data {
	int status_gpio;
};

extern int legoev3_fiq_request_port(enum ev3_input_port_id port_id,
				    int sda_pin, int scl_pin);
extern void legoev3_fiq_release_port(enum ev3_input_port_id port_id);
extern int legoev3_fiq_start_xfer(enum ev3_input_port_id port_id,
				  struct i2c_msg msgs[], int num_msg,
				  void (*complete)(int, void *), void *context);

#endif /* __MACH_LEGOEV3_FIQ_H */
