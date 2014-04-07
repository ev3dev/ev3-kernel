/*
 * Tacho motor device class for LEGO Mindstorms EV3
 *
 * Copyright (C) 2013-2014 Ralph Hempel ,rhempel@hempeldesigngroup.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_LEGOEV3_TACHO_MOTOR_CLASS_H
#define __LINUX_LEGOEV3_TACHO_MOTOR_CLASS_H

#include <linux/device.h>

enum tacho_motor_regulation_mode {
	REGULATION_OFF,
	REGULATION_ON,
	NUM_REGULATION_MODES,
};

enum tacho_motor_brake_mode {
	BRAKE_OFF,
	BRAKE_ON,
	NUM_BRAKE_MODES,
};

enum tacho_motor_hold_mode {
	HOLD_OFF,
	HOLD_ON,
	NUM_HOLD_MODES,
};

enum tacho_motor_position_mode {
	POSITION_ABSOLUTE,
	POSITION_RELATIVE,
	NUM_POSITION_MODES,
};

enum tacho_motor_run_mode {
	RUN_FOREVER,
	RUN_TIME,
	RUN_POSITION,
	NUM_RUN_MODES,
};

enum tacho_motor_polarity_mode {
	POLARITY_POSITIVE,
	POLARITY_NEGATIVE,
	NUM_POLARITY_MODES,
};

enum tacho_motor_type {
	TACHO_TYPE_TACHO,
	TACHO_TYPE_MINITACHO,
	NUM_TACHO_TYPES,
};

enum
{
  STATE_RUN_FOREVER,
  STATE_SETUP_RAMP_TIME,
  STATE_SETUP_RAMP_POSITION,
  STATE_SETUP_RAMP_REGULATION,
  STATE_RAMP_UP,
  STATE_RAMP_CONST,
  STATE_POSITION_RAMP_DOWN,
  STATE_RAMP_DOWN,
  STATE_STOP,
  STATE_IDLE,
  NUM_TACHO_MOTOR_STATES,
};


struct function_pointers;

struct tacho_motor_device {
	const struct function_pointers const *fp;

	/* private */
	struct device dev;
};

struct function_pointers {
	int  (*get_type)(struct tacho_motor_device *tm);
	void (*set_type)(struct tacho_motor_device *tm, long type);

	int  (*get_position)(struct tacho_motor_device *tm);
	void (*set_position)(struct tacho_motor_device *tm, long position);

	int  (*get_speed)(struct tacho_motor_device *tm);
	int  (*get_power)(struct tacho_motor_device *tm);
	int  (*get_state)(struct tacho_motor_device *tm);
	int  (*get_pulses_per_second)(struct tacho_motor_device *tm);

	int  (*get_speed_setpoint)(struct tacho_motor_device *tm);
	void (*set_speed_setpoint)(struct tacho_motor_device *tm, long speed_setpoint);

	int  (*get_time_setpoint)(struct tacho_motor_device *tm);
	void (*set_time_setpoint)(struct tacho_motor_device *tm, long time_setpoint);

	int  (*get_position_setpoint)(struct tacho_motor_device *tm);
	void (*set_position_setpoint)(struct tacho_motor_device *tm, long position_setpoint);

	int  (*get_run_mode)(struct tacho_motor_device *tm);
	void (*set_run_mode)(struct tacho_motor_device *tm, long run_mode);

 	int  (*get_regulation_mode)(struct tacho_motor_device *tm);
 	void (*set_regulation_mode)(struct tacho_motor_device *tm, long regulation_mode);

 	int  (*get_brake_mode)(struct tacho_motor_device *tm);
 	void (*set_brake_mode)(struct tacho_motor_device *tm, long brake_mode);

 	int  (*get_hold_mode)(struct tacho_motor_device *tm);
 	void (*set_hold_mode)(struct tacho_motor_device *tm, long hold_mode);

 	int  (*get_position_mode)(struct tacho_motor_device *tm);
 	void (*set_position_mode)(struct tacho_motor_device *tm, long position_mode);

 	int  (*get_polarity_mode)(struct tacho_motor_device *tm);
 	void (*set_polarity_mode)(struct tacho_motor_device *tm, long polarity_mode);

 	int  (*get_ramp_up)(struct tacho_motor_device *tm);
 	void (*set_ramp_up)(struct tacho_motor_device *tm, long ramp_up);

 	int  (*get_ramp_down)(struct tacho_motor_device *tm);
 	void (*set_ramp_down)(struct tacho_motor_device *tm, long ramp_down);
 
	int  (*get_run)(struct tacho_motor_device *tm);
	void (*set_run)(struct tacho_motor_device *tm, long);

	void (*set_reset)(struct tacho_motor_device *tm, long reset);
};

extern int register_tacho_motor(struct tacho_motor_device *,
				 struct device *);
extern void unregister_tacho_motor(struct tacho_motor_device *);

extern struct class tacho_motor_class;

#endif /* __LINUX_LEGOEV3_TACHO_MOTOR_CLASS_H */
