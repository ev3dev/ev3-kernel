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

enum
{
  FORWARD,
  BACKWARD,
  BRAKE,
  COAST,
};

enum
{
  UNLIMITED_UNREG,
  UNLIMITED_REG,
  LIMITED_REG_STEPUP,
  LIMITED_REG_STEPCONST,
  LIMITED_REG_STEPDOWN,
  LIMITED_UNREG_STEPUP,
  LIMITED_UNREG_STEPCONST,
  LIMITED_UNREG_STEPDOWN,
  LIMITED_REG_TIMEUP,
  LIMITED_REG_TIMECONST,
  LIMITED_REG_TIMEDOWN,
  LIMITED_UNREG_TIMEUP,
  LIMITED_UNREG_TIMECONST,
  LIMITED_UNREG_TIMEDOWN,
  LIMITED_STEP_SYNC,
  LIMITED_TURN_SYNC,
  LIMITED_DIFF_TURN_SYNC,
  SYNCED_SLAVE,
  RAMP_DOWN_SYNC,
  HOLD,
  BRAKED,
  STOP_MOTOR,
  IDLE,
  NUM_TACHO_MOTOR_MODES,
};

struct tacho_motor_device {
	int (*get_tacho    )(struct tacho_motor_device *);
	int (*get_step     )(struct tacho_motor_device *);
	int (*get_time     )(struct tacho_motor_device *);
	int (*get_direction)(struct tacho_motor_device *);
	int (*get_speed    )(struct tacho_motor_device *);
	int (*get_power    )(struct tacho_motor_device *);
	int (*get_state    )(struct tacho_motor_device *);

	int  (*get_target_power    )(struct tacho_motor_device *);
	void (*set_target_power    )(struct tacho_motor_device *, long target_power);

	int  (*get_target_tacho    )(struct tacho_motor_device *);
	void (*set_target_tacho    )(struct tacho_motor_device *, long target_tacho);

	int  (*get_target_step     )(struct tacho_motor_device *);
	void (*set_target_step     )(struct tacho_motor_device *, long target_step);

	int  (*get_target_speed    )(struct tacho_motor_device *);
	void (*set_target_speed    )(struct tacho_motor_device *, long target_speed);

	int  (*get_target_time     )(struct tacho_motor_device *);
	void (*set_target_time     )(struct tacho_motor_device *, long target_time);

	/* private */
	struct device dev;
};

extern int register_tacho_motor(struct tacho_motor_device *,
				 struct device *);
extern void unregister_tacho_motor(struct tacho_motor_device *);

extern struct class tacho_motor_class;

#endif /* __LINUX_LEGOEV3_TACHO_MOTOR_CLASS_H */
