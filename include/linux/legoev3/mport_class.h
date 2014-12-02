/*
 * Mindstorms port device class for LEGO MINDSTORMS EV3
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

#ifndef _LINUX_LEGOEV3_MPORT_CLASS_H
#define _LINUX_LEGOEV3_MPORT_CLASS_H

#include <linux/device.h>
#include <linux/types.h>

#define MPORT_NAME_SIZE	30

/**
 * struct mport_mode_info
 * @name: The name of this mode.
 */
struct mport_mode_info {
	char name[MPORT_NAME_SIZE + 1];
};

/**
 * struct mport_device
 * @port_name: Name of the port.
 * @num_modes: The number of valid modes.
 * @mode: The current mode.
 * @mode_info: Array of mode information.
 * @set_mode: Callback to set the sensor mode.
 * @set_device: Callback to load a device attached to this port.
 * @get_status: Callback to get the status string. (optional)
 * @context: Pointer to pass back to callback functions.
 * @dev: The device data structure.
 */
struct mport_device {
	char port_name[MPORT_NAME_SIZE + 1];
	u8 num_modes;
	u8 mode;
	const struct mport_mode_info *mode_info;
	int (*set_mode)(void *context, u8 mode);
	int (*set_device)(void *context, const char *device_name);
	const char *(*get_status)(void *context);
	void *context;
	/* private */
	struct device dev;
};

#define to_mport_device(_dev) container_of(_dev, struct mport_device, dev)

extern int register_mport(struct mport_device *mport, struct device *parent);
extern void unregister_mport(struct mport_device *mport);

extern struct class mport_class;

#endif /* _LINUX_LEGOEV3_MPORT_CLASS_H */
