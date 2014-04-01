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

enum tacho_motor_run_mode {
	RUN_FOREVER,
	RUN_TIME,
	RUN_POSITION,
	NUM_RUN_MODES,
};

enum tacho_motor_tacho_mode {
	TACHO_ABSOLUTE,
	TACHO_RELATIVE,
	NUM_TACHO_MODES,
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
  STATE_SETUP_RAMP_ABSOLUTE_TACHO,
  STATE_SETUP_RAMP_RELATIVE_TACHO,
  STATE_SETUP_RAMP_REGULATION,
  STATE_RAMP_UP,
  STATE_RAMP_CONST,
  STATE_RAMP_DOWN,
  STATE_STOP_MOTOR,
  STATE_IDLE,
  NUM_TACHO_MOTOR_STATES,
};

struct tacho_motor_device {

	int  (*get_type      )(struct tacho_motor_device *);
	void (*set_type      )(struct tacho_motor_device *, long type);

	int  (*get_position )(struct tacho_motor_device *);
	int  (*get_speed    )(struct tacho_motor_device *);
	int  (*get_power    )(struct tacho_motor_device *);
	int  (*get_state    )(struct tacho_motor_device *);

	int  (*get_speed_setpoint       )(struct tacho_motor_device *);
	void (*set_speed_setpoint       )(struct tacho_motor_device *, long speed_setpoint);

	int  (*get_run_mode       )(struct tacho_motor_device *);
	void (*set_run_mode       )(struct tacho_motor_device *, long run_mode);

	int  (*get_regulation_mode )(struct tacho_motor_device *);
	void (*set_regulation_mode )(struct tacho_motor_device *, long regulation_mode);

	int  (*get_brake_mode )(struct tacho_motor_device *);
	void (*set_brake_mode )(struct tacho_motor_device *, long brake_mode);

	int  (*get_hold_mode )(struct tacho_motor_device *);
	void (*set_hold_mode )(struct tacho_motor_device *, long hold_mode);

//
//
//
//
//
//
//	int  (*get_stop_mode       )(struct tacho_motor_device *);
//	void (*set_stop_mode       )(struct tacho_motor_device *, long stop_mode);
//
//	int  (*get_tacho_mode      )(struct tacho_motor_device *);
//	void (*set_tacho_mode      )(struct tacho_motor_device *, long tacho_mode);
//
//
//
//
//	int  (*get_target_power    )(struct tacho_motor_device *);
//	void (*set_target_power    )(struct tacho_motor_device *, long target_power);
//
//	int  (*get_target_tacho    )(struct tacho_motor_device *);
//	void (*set_target_tacho    )(struct tacho_motor_device *, long target_tacho);
//
//	int  (*get_target_speed    )(struct tacho_motor_device *);
//	void (*set_target_speed    )(struct tacho_motor_device *, long target_speed);
//
//	int  (*get_target_steer    )(struct tacho_motor_device *);
//	void (*set_target_steer    )(struct tacho_motor_device *, long target_steer);
//
//	int  (*get_target_time     )(struct tacho_motor_device *);
//	void (*set_target_time     )(struct tacho_motor_device *, long target_time);
//
//	int  (*get_target_ramp_up_count  )(struct tacho_motor_device *);
//	void (*set_target_ramp_up_count  )(struct tacho_motor_device *, long target_ramp_up_count);
//
//	int  (*get_target_total_count    )(struct tacho_motor_device *);
//	void (*set_target_total_count    )(struct tacho_motor_device *, long target_total_count);
//
//	int  (*get_target_ramp_down_count)(struct tacho_motor_device *);
//	void (*set_target_ramp_down_count)(struct tacho_motor_device *, long target_ramp_down_count);
//
//	int  (*get_mode     )(struct tacho_motor_device *);
//	void (*set_mode     )(struct tacho_motor_device *, long mode);

	int  (*get_run     )(struct tacho_motor_device *);
	void (*set_run     )(struct tacho_motor_device *, long run);

//	int (*get_step     )(struct tacho_motor_device *);
//	int (*get_time     )(struct tacho_motor_device *);
//	int (*get_direction)(struct tacho_motor_device *);
	/* private */
	struct device dev;
};

extern int register_tacho_motor(struct tacho_motor_device *,
				 struct device *);
extern void unregister_tacho_motor(struct tacho_motor_device *);

extern struct class tacho_motor_class;

#endif /* __LINUX_LEGOEV3_TACHO_MOTOR_CLASS_H */
