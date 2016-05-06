/*
 * GPIO PWM driver
 *
 * Copyright (C) 2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_PLATFORM_DATA_PWM_GPIO_H__
#define __LINUX_PLATFORM_DATA_PWM_GPIO_H__

struct pwm_gpio {
	const char* name;
	unsigned gpio;
};

struct pwm_gpio_platform_data {
	unsigned num_pwms;
	struct pwm_gpio *pwms;
};

#endif /* __LINUX_PLATFORM_DATA_PWM_GPIO_H__ */
