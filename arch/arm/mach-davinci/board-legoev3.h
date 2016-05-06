/*
 * Board specific defines for LEGO MINDSTORMS EV3
 *
 * Copyright (C) 2013 Ralph Hempel
 * Copyright (C) 2013-2014 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_MACH_DAVINCI_BOARD_LEGOEV3_H
#define __ARCH_ARM_MACH_DAVINCI_BOARD_LEGOEV3_H

#include <mach/mux.h>
#include <linux/platform_data/gpio-davinci.h>

#define PIN(ev3_name, da850_name) \
EV3_##ev3_name		= DA850_##da850_name,

#define GPIO_PIN(ev3_name, group, pin)			\
EV3_##ev3_name		= DA850_GPIO##group##_##pin,	\
EV3_##ev3_name##_PIN	= GPIO_TO_PIN(group, pin),

enum legoev3_pin_map {
	/* LCD pins */
	PIN(LCD_DATA_OUT, SPI1_SIMO)
	GPIO_PIN(LCD_RESET, 5, 0)
	GPIO_PIN(LCD_A0, 2, 11)
	GPIO_PIN(LCD_CS, 2, 12)

	/* LED pins */
	GPIO_PIN(LED_0, 6, 13)
	GPIO_PIN(LED_1, 6, 7)
	GPIO_PIN(LED_2, 6, 14)
	GPIO_PIN(LED_3, 6, 12)

	/* Button pins */
	GPIO_PIN(BUTTON_0, 7, 15)
	GPIO_PIN(BUTTON_1, 1, 13)
	GPIO_PIN(BUTTON_2, 7, 14)
	GPIO_PIN(BUTTON_3, 7, 12)
	GPIO_PIN(BUTTON_4, 6, 6)
	GPIO_PIN(BUTTON_5, 6, 10)

	/* sound pins */
	PIN(SND_PWM, EHRPWM0_B)
	GPIO_PIN(SND_ENA, 6, 15)

	/* power pins */
	GPIO_PIN(SYS_POWER_ENA, 6, 5)
	GPIO_PIN(SYS_5V_POWER, 6, 11)
	GPIO_PIN(BATT_TYPE, 8, 8)
	GPIO_PIN(BATT_ADC, 0, 6)

	/* I2C board pins */
	PIN(I2C_BOARD_SDA, I2C0_SDA)
	PIN(I2C_BOARD_SCL, I2C0_SCL)

	/* Bluetooth pins */
	GPIO_PIN(BT_ENA, 4, 9)
	GPIO_PIN(BT_CLK_ENA, 0, 5)
	PIN(BT_CLK, ECAP2_APWM2)
	PIN(BT_UART_CTS, NUART2_CTS)
	PIN(BT_UART_RTS, NUART2_RTS)
	PIN(BT_UART_RXD, UART2_RXD)
	PIN(BT_UART_TXD, UART2_TXD)

	/* analog/digital converter pins */
	PIN(ADC_DATA_IN, SPI0_SOMI)
	PIN(ADC_DATA_OUT, SPI0_SOMI)
	PIN(ADC_CS, SPI0_SCS_3)
	PIN(ADC_CLK, SPI0_CLK)

	/* USB1 VBUS pins */
	GPIO_PIN(USB11_OVC, 6, 3)

	/* SD card reader pins */
	PIN(SD_DAT_0, MMCSD0_DAT_0)
	PIN(SD_DAT_1, MMCSD0_DAT_1)
	PIN(SD_DAT_2, MMCSD0_DAT_2)
	PIN(SD_DAT_3, MMCSD0_DAT_3)
	PIN(SD_CLK, MMCSD0_CLK)
	PIN(SD_CMD, MMCSD0_CMD)
	GPIO_PIN(SD_CD, 5, 14)

	/* Input and output port pins */
	GPIO_PIN(IN1_PIN1, 8, 10)
	GPIO_PIN(IN1_PIN2, 2, 2)
	GPIO_PIN(IN1_PIN5, 0, 2)
	GPIO_PIN(IN1_PIN6, 0, 15)
	GPIO_PIN(IN1_BUF_ENA, 8, 11)
	GPIO_PIN(IN1_I2C_CLK, 1, 0) /* physical pin is shared with IN1_UART_TXD */
	PIN(IN1_UART_TXD, UART1_TXD) /* physical pin is shared with IN1_I2C_CLK */
	PIN(IN1_UART_RXD, UART1_RXD)
	GPIO_PIN(IN2_PIN1, 8, 12)
	GPIO_PIN(IN2_PIN2, 8, 15)
	GPIO_PIN(IN2_PIN5, 0, 14)
	GPIO_PIN(IN2_PIN6, 0, 13)
	GPIO_PIN(IN2_BUF_ENA, 8, 14)
	GPIO_PIN(IN2_I2C_CLK, 8, 3) /* physical pin is shared with IN2_UART_TXD */
	PIN(IN2_UART_TXD, UART0_TXD) /* physical pin is shared with IN2_I2C_CLK */
	PIN(IN2_UART_RXD, UART0_RXD)
	GPIO_PIN(IN3_PIN1, 8, 9)
	GPIO_PIN(IN3_PIN2, 7, 11)
	GPIO_PIN(IN3_PIN5, 0, 12)
	GPIO_PIN(IN3_PIN6, 1, 14)
	GPIO_PIN(IN3_BUF_ENA, 7, 9)
	GPIO_PIN(IN3_I2C_CLK, 1, 12) /* physical pin is shared with IN3_UART */
	PIN(IN3_UART_TXD, AXR_4) /* physical pin is shared with IN3_I2C_CLK */
	PIN(IN3_UART_RXD, AXR_2)
	GPIO_PIN(IN4_PIN1, 6, 4)
	GPIO_PIN(IN4_PIN2, 7, 8)
	GPIO_PIN(IN4_PIN5, 0, 1)
	GPIO_PIN(IN4_PIN6, 1, 15)
	GPIO_PIN(IN4_BUF_ENA, 7, 10)
	GPIO_PIN(IN4_I2C_CLK, 1, 11) /* physical pin is shared with IN4_UART */
	PIN(IN4_UART_TXD, AXR_3) /* physical pin is shared with IN4_I2C_CLK */
	PIN(IN4_UART_RXD, AXR_1)
	GPIO_PIN(OUT1_PIN1, 3, 15)
	GPIO_PIN(OUT1_PIN2, 3, 6)
	GPIO_PIN(OUT1_PIN5, 5, 4)
	GPIO_PIN(OUT1_PIN5_INT, 5, 11)
	GPIO_PIN(OUT1_PIN6_DIR, 0, 4)
	PIN(OUT1_PWM, EHRPWM1_B)
	GPIO_PIN(OUT2_PIN1, 2, 1)
	GPIO_PIN(OUT2_PIN2, 0, 3)
	GPIO_PIN(OUT2_PIN5, 2, 5)
	GPIO_PIN(OUT2_PIN5_INT, 5, 8)
	GPIO_PIN(OUT2_PIN6_DIR, 2, 9)
	PIN(OUT2_PWM, EHRPWM1_A)
	GPIO_PIN(OUT3_PIN1, 6, 8)
	GPIO_PIN(OUT3_PIN2, 5, 9)
	GPIO_PIN(OUT3_PIN5, 3, 8)
	GPIO_PIN(OUT3_PIN5_INT, 5, 13)
	GPIO_PIN(OUT3_PIN6_DIR, 3, 14)
	PIN(OUT3_PWM, ECAP0_APWM0)
	GPIO_PIN(OUT4_PIN1, 5, 3)
	GPIO_PIN(OUT4_PIN2, 5, 10)
	GPIO_PIN(OUT4_PIN5, 5, 15)
	GPIO_PIN(OUT4_PIN5_INT, 6, 9)
	GPIO_PIN(OUT4_PIN6_DIR, 2, 8)
	PIN(OUT4_PWM, ECAP1_APWM1)
	GPIO_PIN(FIQ_STAT, 2, 7) /* used by FIQ handler routine to notify
					the OS when status has changed.
					See arch/arm/mach-davinci/legoev3-fiq.c.
					This pin is TP4 in the lms2012 source
					code, so it should be safe to use. */
};

#endif /* __ARCH_ARM_MACH_DAVINCI_BOARD_LEGOEV3_H */
