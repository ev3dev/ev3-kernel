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
#include <sound/pcm.h>

/**
 * struct legoev3_fiq_platform_data - platform specific data
 * @intc_mem_base: Base memory address of interrupt controller.
 * @intc_mem_size: Size of interrupt memory.
 * @gpio_mem_base: Base memory address of GPIO controller.
 * @gpio_mem_size: Size of GPIO memory.
 * @ehrpwm_mem_base: Base memory address of EHRPWM controller.
 * @ehrowm_mem_size: Size of EHROWM memory.
 * @timer_irq: Timer interrupt for I2C callback.
 * @ehrpwm_irq: EHRPWM interrupt for sound callback.
 * @status_gpio: GPIO that is not physically connected that can be used to
 *	to generate interrupts so that the FIQ can notify external code
 *	that its status has changed.
 */
struct legoev3_fiq_platform_data {
	unsigned intc_mem_base;
	unsigned intc_mem_size;
	unsigned gpio_mem_base;
	unsigned gpio_mem_size;
	unsigned ehrpwm_mem_base;
	unsigned ehrpwm_mem_size;
	unsigned timer_irq;
	unsigned ehrpwm_irq;
	int status_gpio;
};

extern int legoev3_fiq_request_port(enum legoev3_input_port_id port_id,
				    int sda_pin, int scl_pin);
extern void legoev3_fiq_release_port(enum legoev3_input_port_id port_id);
extern int legoev3_fiq_start_xfer(enum legoev3_input_port_id port_id,
				  struct i2c_msg msgs[], int num_msg,
				  void (*complete)(int, void *), void *context);
extern void legoev3_fiq_cancel_xfer(enum legoev3_input_port_id port_id);
extern int legoev3_fiq_ehrpwm_request(void);
extern void legoev3_fiq_ehrpwm_release(void);
extern int legoev3_fiq_ehrpwm_prepare(struct snd_pcm_substream *substream,
				      int volume, unsigned char int_period,
				      void (*period_elapsed)(void *),
				      void *context);
extern void legoev3_fiq_ehrpwm_ramp(struct snd_pcm_substream *substream,
				    int direction, unsigned ramp_ms);
extern unsigned legoev3_fiq_ehrpwm_get_playback_ptr(void);
extern void legoev3_fiq_ehrpwm_set_volume(int volume);
extern int legoev3_fiq_ehrpwm_int_enable(void);
extern int legoev3_fiq_ehrpwm_int_disable(void);

#endif /* __MACH_LEGOEV3_FIQ_H */
