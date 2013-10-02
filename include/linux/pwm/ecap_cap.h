/*
 * This program is free software; you may redistribute and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#ifndef __LINUX_CAPTURE_H
#define __LINUX_CAPTURE_H

#define EC_RISING				0x0
#define EC_RISING_FALLING			0x1

#define EC_ABS_MODE				0x0
#define EC_DELTA_MODE				0x1

#define EC_ONESHOT				0x1

struct ecap_cap {
	int edge;
	int prescale;
};

int ecap_get_cap_value(int *cap_value, int instance);
int ecap_cap_config(int instance, struct ecap_cap *ecap_cap);

#endif
