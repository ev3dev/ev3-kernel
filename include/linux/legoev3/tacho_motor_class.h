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

enum tacho_motor_stop_mode {
	STOP_COAST,
	STOP_BRAKE,
	NUM_STOP_MODES,
};

enum tacho_motor_regulation_mode {
	REGULATION_OFF,
	REGULATION_ON,
	NUM_REGULATION_MODES,
};

enum tacho_motor_ramp_mode {
	RAMP_OFF,
	RAMP_TIME,
	RAMP_TACHO,
	NUM_RAMP_MODES,
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

enum tacho_motor_tacho_type {
	TACHO_TYPE_TACHO,
	TACHO_TYPE_MINITACHO,
	NUM_TACHO_TYPES,
};

enum
{
  UNLIMITED_UNREG,
  UNLIMITED_REG,
  SETUP_RAMP_TIME,
  SETUP_RAMP_ABSOLUTE_TACHO,
  SETUP_RAMP_RELATIVE_TACHO,
  SETUP_RAMP_REGULATION,
  RAMP_UP,
  RAMP_CONST,
  RAMP_DOWN,
  STOP_MOTOR,
  IDLE,
  NUM_TACHO_MOTOR_STATES,
};

struct tacho_motor_device {
	int (*get_tacho    )(struct tacho_motor_device *);
	int (*get_step     )(struct tacho_motor_device *);
	int (*get_time     )(struct tacho_motor_device *);
	int (*get_direction)(struct tacho_motor_device *);
	int (*get_speed    )(struct tacho_motor_device *);
	int (*get_power    )(struct tacho_motor_device *);
	int (*get_state    )(struct tacho_motor_device *);

	int  (*get_stop_mode       )(struct tacho_motor_device *);
	void (*set_stop_mode       )(struct tacho_motor_device *, long stop_mode);

	int  (*get_regulation_mode )(struct tacho_motor_device *);
	void (*set_regulation_mode )(struct tacho_motor_device *, long regulation_mode);

	int  (*get_tacho_mode      )(struct tacho_motor_device *);
	void (*set_tacho_mode      )(struct tacho_motor_device *, long tacho_mode);

	int  (*get_ramp_mode       )(struct tacho_motor_device *);
	void (*set_ramp_mode       )(struct tacho_motor_device *, long ramp_mode);

	int  (*get_tacho_type      )(struct tacho_motor_device *);
	void (*set_tacho_type      )(struct tacho_motor_device *, long tacho_type);




	int  (*get_target_power    )(struct tacho_motor_device *);
	void (*set_target_power    )(struct tacho_motor_device *, long target_power);

	int  (*get_target_tacho    )(struct tacho_motor_device *);
	void (*set_target_tacho    )(struct tacho_motor_device *, long target_tacho);

	int  (*get_target_speed    )(struct tacho_motor_device *);
	void (*set_target_speed    )(struct tacho_motor_device *, long target_speed);

	int  (*get_target_steer    )(struct tacho_motor_device *);
	void (*set_target_steer    )(struct tacho_motor_device *, long target_steer);

	int  (*get_target_time     )(struct tacho_motor_device *);
	void (*set_target_time     )(struct tacho_motor_device *, long target_time);

	int  (*get_target_ramp_up_count  )(struct tacho_motor_device *);
	void (*set_target_ramp_up_count  )(struct tacho_motor_device *, long target_ramp_up_count);

	int  (*get_target_total_count    )(struct tacho_motor_device *);
	void (*set_target_total_count    )(struct tacho_motor_device *, long target_total_count);

	int  (*get_target_ramp_down_count)(struct tacho_motor_device *);
	void (*set_target_ramp_down_count)(struct tacho_motor_device *, long target_ramp_down_count);

	int  (*get_mode     )(struct tacho_motor_device *);
	void (*set_mode     )(struct tacho_motor_device *, long mode);

	int  (*get_run     )(struct tacho_motor_device *);
	void (*set_run     )(struct tacho_motor_device *, long run);

	/* private */
	struct device dev;
};

extern int register_tacho_motor(struct tacho_motor_device *,
				 struct device *);
extern void unregister_tacho_motor(struct tacho_motor_device *);

extern struct class tacho_motor_class;

#endif /* __LINUX_LEGOEV3_TACHO_MOTOR_CLASS_H */
