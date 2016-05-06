/*
 * Platform data for LEGO MINDSTORMS EV3 drivers
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

#ifndef _PLATFORM_DATA_LEGOEV3_H
#define _PLATFORM_DATA_LEGOEV3_H

#include <mach/legoev3.h>

struct legoev3_analog_platform_data {
	u8 in_pin1_ch[NUM_EV3_PORT_IN];
	u8 in_pin6_ch[NUM_EV3_PORT_IN];
	u8 out_pin5_ch[NUM_EV3_PORT_OUT];
	u8 batt_volt_ch;
	u8 batt_curr_ch;
};

/**
 * struct legoev3_battery_platform_data
 * @bat_adc_gpio: GPIO that switches the battery voltage to the analog/digital
 *	converter on or off.
 * @bat_type_gpio: GPIO connected to the battery type indicator switch in the
 *	battery compartment of the EV3.
 */
struct legoev3_battery_platform_data {
	unsigned int batt_adc_gpio;
	unsigned int batt_type_gpio;
};

/**
 * struct legoev3_bluetooth_platform_data
 * @pic_ena_gpio: GPIO that is connected to PIC Vdd
 * @pic_rst_gpio: GPIO that is connected to PIC nMCLR
 * @pic_cts_gpio: GPIO that is connected to BT UART CTS
 * @bt_ena_gpio: GPIO that is connected to the nSHUTD pin of the bluetooth chip
 * @bt_ena_clk_gpio: GPIO that can pull down the "slow clock" PWM output
 */
struct legoev3_bluetooth_platform_data {
	int pic_ena_gpio;
	int pic_rst_gpio;
	int pic_cts_gpio;
	int bt_ena_gpio;
	int bt_clk_ena_gpio;
};

struct ev3_input_port_platform_data {
	enum legoev3_input_port_id id;
	unsigned pin1_gpio;
	unsigned pin2_gpio;
	unsigned pin5_gpio;
	unsigned pin6_gpio;
	unsigned buf_ena_gpio;
	unsigned i2c_clk_gpio;
	unsigned i2c_dev_id;
	unsigned i2c_pin_mux;
	unsigned uart_pin_mux;
	const char *uart_tty;
};

struct ev3_output_port_platform_data {
	enum legoev3_output_port_id id;
	unsigned pin1_gpio;
	unsigned pin2_gpio;
	unsigned pin5_gpio;
	unsigned pin5_int_gpio;
	unsigned pin6_dir_gpio;
};

struct legoev3_ports_platform_data {
	struct ev3_input_port_platform_data  input_port_data[NUM_EV3_PORT_IN];
	struct ev3_output_port_platform_data output_port_data[NUM_EV3_PORT_OUT];
};

#endif /* _PLATFORM_DATA_LEGOEV3_H */
