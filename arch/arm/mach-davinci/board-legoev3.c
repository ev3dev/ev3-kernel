/*
 * LEGO MINDSTORMS EV3 DA850/OMAP-L138
 *
 * Copyright (C) 2013 Ralph Hempel
 * Copyright (C) 2013-2014 David Lechner <david@lechnology.com>
 *
 * Derived from: arch/arm/mach-davinci/board-da850-evm.c
 * Original Copyrights follows:
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * 2007, 2009 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/console.h>
#include <linux/gpio.h>
#include <linux/legoev3/legoev3_analog.h>
#include <linux/legoev3/legoev3_bluetooth.h>
#include <linux/legoev3/legoev3_ports.h>
#include <linux/power/legoev3_battery.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/regulator/machine.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
#include <linux/wl12xx.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/cp_intc.h>
#include <mach/da8xx.h>
#include <mach/legoev3.h>
#include <mach/legoev3-fiq.h>
#include <mach/nand.h>
#include <mach/mux.h>
#include <mach/spi.h>
#include <mach/time.h>
#include <mach/usb.h>

#include <video/st7586fb.h>

#include "board-legoev3.h"

/*
 * LCD configuration:
 * ==================
 * The LCD is connected via SPI1 and uses the st7586fb frame buffer driver.
 */

static const short legoev3_lcd_pins[] __initconst = {
	EV3_LCD_DATA_OUT, EV3_LCD_RESET, EV3_LCD_A0, EV3_LCD_CS
	-1
};

static const struct st7586fb_platform_data legoev3_st7586fb_data = {
	.rst_gpio	= EV3_LCD_RESET_PIN,
	.a0_gpio	= EV3_LCD_A0_PIN,
	.cs_gpio	= EV3_LCD_CS_PIN,
};

static struct davinci_spi_config legoev3_st7586fb_cfg = {
	.io_type	= SPI_IO_TYPE_DMA,
	.c2tdelay	= 10,
	.t2cdelay	= 10,
};

static struct spi_board_info legoev3_spi1_board_info[] = {
	[0] = {
		.modalias		= "st7586fb-legoev3",
		.platform_data		= &legoev3_st7586fb_data,
		.controller_data	= &legoev3_st7586fb_cfg,
		.mode			= SPI_MODE_3 | SPI_NO_CS,
		.max_speed_hz		= 10000000,
		.bus_num		= 1,
	},
};

/*
 * LED configuration:
 * ==================
 * The LEDs are connected via GPIOs and use the leds-gpio driver.
 * The default triggers are set so that during boot, the left LED will flash
 * amber to indicate CPU activity and the right LED will flash amber to
 * indicate disk (SD card) activity.
 */

static const short legoev3_led_pins[] __initconst = {
	EV3_LED_0, EV3_LED_1, EV3_LED_2, EV3_LED_3,
	-1
};

#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
#include <linux/leds.h>

static struct gpio_led ev3_gpio_leds[] = {
	{
		.name = "ev3:red:right",
		.default_trigger = "mmc0",
		.gpio = EV3_LED_0_PIN,
	},
	{
		.name = "ev3:green:right",
		.default_trigger = "mmc0",
		.gpio = EV3_LED_1_PIN,
	},
	{
		.name = "ev3:red:left",
		.default_trigger = "heartbeat",
		.gpio = EV3_LED_2_PIN,
	},
	{
		.name = "ev3:green:left",
		.default_trigger = "heartbeat",
		.gpio = EV3_LED_3_PIN,
	},
};

static struct gpio_led_platform_data ev3_gpio_led_data = {
	.num_leds	= ARRAY_SIZE(ev3_gpio_leds),
	.leds		= ev3_gpio_leds
};

static struct platform_device ev3_device_gpio_leds = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data  = &ev3_gpio_led_data,
	},
};
#endif

/*
 * EV3 button configuration:
 * =========================
 * The buttons are connected via GPIOs and use the gpio-keys driver.
 * The buttons are mapped to the UP, DOWN, LEFT, RIGHT, ENTER and ESC keys.
 */

static const short legoev3_button_pins[] __initconst = {
	EV3_BUTTON_0, EV3_BUTTON_1, EV3_BUTTON_2,
	EV3_BUTTON_3, EV3_BUTTON_4, EV3_BUTTON_5,
	-1
};

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#include <linux/gpio_keys.h>
#include <linux/input.h>

static struct gpio_keys_button ev3_gpio_keys_table[] = {
	{KEY_UP,    EV3_BUTTON_0_PIN, 1, "ev3:UP",    EV_KEY, 0, 50, 1},
	{KEY_ENTER, EV3_BUTTON_1_PIN, 1, "ev3:ENTER", EV_KEY, 0, 50, 1},
	{KEY_DOWN,  EV3_BUTTON_2_PIN, 1, "ev3:DOWN",  EV_KEY, 0, 50, 1},
	{KEY_RIGHT, EV3_BUTTON_3_PIN, 1, "ev3:RIGHT", EV_KEY, 0, 50, 1},
	{KEY_LEFT,  EV3_BUTTON_4_PIN, 1, "ev3:LEFT",  EV_KEY, 0, 50, 1},
	{KEY_ESC,   EV3_BUTTON_5_PIN, 1, "ev3:ESC",   EV_KEY, 0, 50, 1},
};

static struct gpio_keys_platform_data ev3_gpio_keys_data = {
	.buttons = ev3_gpio_keys_table,
	.nbuttons = ARRAY_SIZE(ev3_gpio_keys_table),
};

static struct platform_device ev3_device_gpiokeys = {
	.name = "gpio-keys",
	.dev = {
		.platform_data = &ev3_gpio_keys_data,
	},
};
#endif

/*
 * Power management configuration:
 * ===============================
 * Not sure about this, but the comments in arch/arm/mach-davinci/include/mach/pm.h
 * imply that a value of 128 indicates that we have an external oscillator.
 */

static struct davinci_pm_config da850_pm_pdata = {
	.sleepcount = 128,
};

static struct platform_device da850_pm_device = {
	.name	= "pm-davinci",
	.dev	= {
		.platform_data	= &da850_pm_pdata,
	},
	.id	= -1,
};

/*
 * EV3 USB configuration:
 * ======================
 */

static const short legoev3_usb1_pins[] __initconst = {
	EV3_USB1_OVC,
	-1
};

static irqreturn_t legoev3_usb_ocic_irq(int, void *);

static struct da8xx_ohci_root_hub legoev3_usb1_pdata = {
	.type	= GPIO_BASED,
	.method	= {
		.gpio_method = {
			.over_current_indicator = EV3_USB1_OVC_PIN,
		},
	},
	.board_ocic_handler	= legoev3_usb_ocic_irq,
};

static irqreturn_t legoev3_usb_ocic_irq(int irq, void *handler)
{
	if (handler != NULL)
		((da8xx_ocic_handler_t)handler)(&legoev3_usb1_pdata, 1);
	return IRQ_HANDLED;
}

static __init void legoev3_usb_init(void)
{
	u32 cfgchip2;
	int ret;

	/*
	 * Set up USB clock/mode in the CFGCHIP2 register.
	 * FYI:  CFGCHIP2 is 0x0000ef00 initially.
	 */
	cfgchip2 = __raw_readl(DA8XX_SYSCFG0_VIRT(DA8XX_CFGCHIP2_REG));

	/* USB2.0 PHY reference clock is 24 MHz */
	cfgchip2 &= ~CFGCHIP2_REFFREQ;
	cfgchip2 |=  CFGCHIP2_REFFREQ_24MHZ;

	/*
	 * Select internal reference clock for USB 2.0 PHY
	 * and use it as a clock source for USB 1.1 PHY
	 * (this is the default setting anyway).
	 */
	cfgchip2 &= ~CFGCHIP2_USB1PHYCLKMUX;
	cfgchip2 |=  CFGCHIP2_USB2PHYCLKMUX;
	/*
	 * We have to override VBUS/ID signals when MUSB is configured into the
	 * host-only mode -- ID pin will float if no cable is connected, so the
	 * controller won't be able to drive VBUS thinking that it's a B-device.
	 * Otherwise, we want to use the OTG mode and enable VBUS comparators.
	 */
	cfgchip2 &= ~CFGCHIP2_OTGMODE;
	cfgchip2 |=  CFGCHIP2_SESENDEN | CFGCHIP2_VBDTCTEN;

	__raw_writel(cfgchip2, DA8XX_SYSCFG0_VIRT(DA8XX_CFGCHIP2_REG));

	/*
	 * TPS2065 switch @ 5V supplies 1 A (sustains 1.5 A),
	 * with the power on to power good time of 3 ms.
	 */
	ret = da8xx_register_usb20(1000, 3);
	if (ret)
		pr_warning("%s: USB 2.0 registration failed: %d\n",
			   __func__, ret);

	/* initilaize usb module */
	da8xx_board_usb_init(legoev3_usb1_pins, &legoev3_usb1_pdata);
}

/*
 * EV3 bluetooth configuration:
 * ============================
 */

static const short legoev3_bt_pins[] __initconst = {
	EV3_BT_ENA, EV3_BT_ENA2, EV3_BT_PIC_ENA, EV3_BT_PIC_RST, EV3_BT_PIC_CTS,
	EV3_BT_CLK, EV3_BT_UART_CTS, EV3_BT_UART_RTS, EV3_BT_UART_RXD,
	EV3_BT_UART_TXD,
	-1
};

static struct legoev3_bluetooth_platform_data legoev3_bt_pdata = {
	.bt_ena_gpio	= EV3_BT_ENA_PIN,
	.bt_ena2_gpio	= EV3_BT_ENA2_PIN,
	.pic_ena_gpio	= EV3_BT_PIC_ENA_PIN,
	.pic_rst_gpio	= EV3_BT_PIC_RST_PIN,
	.pic_cts_gpio	= EV3_BT_PIC_CTS_PIN,
	.clk_pwm_dev	= "ecap.2",
};

static struct platform_device legoev3_bt_device = {
	.name	= "legoev3-bluetooth",
	.dev	= {
		.platform_data	= &legoev3_bt_pdata,
	},
	.id	= -1,
};

/*
 * SD card reader configuration:
 * =============================
 * LEGO did not hook up the write protect pin? And does not use card detect?
 */

static const short legoev3_sd_pins[] __initconst = {
	EV3_SD_DAT_0, EV3_SD_DAT_1, EV3_SD_DAT_2,
	EV3_SD_DAT_3, EV3_SD_CLK, EV3_SD_CMD,
	-1
};
/*
static int legoev3_mmc_get_ro(int index)
{
	return gpio_get_value(EV3_SD_WP_PIN);
}

static int legoev3_mmc_get_cd(int index)
{
	return !gpio_get_value(EV3_SD_CD_PIN);
}
*/
static struct davinci_mmc_config legoev3_sd_config = {
/*
	.get_ro		= legoev3_mmc_get_ro,
	.get_cd		= legoev3_mmc_get_cd,
*/
	.wires		= 4,
	.max_freq	= 50000000,
	.caps		= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.version	= MMC_CTLR_VERSION_2,
};

/*
 * EV3 CPU frequency scaling configuration:
 * ========================================
 */

#ifdef CONFIG_CPU_FREQ
static __init int legoev3_init_cpufreq(void)
{
	switch (system_rev & 0xF) {
	case 3:
		da850_max_speed = 456000;
		break;
	case 2:
		da850_max_speed = 408000;
		break;
	case 1:
		da850_max_speed = 372000;
		break;
	}

	return da850_register_cpufreq("pll0_sysclk3");
}
#else
static __init int legoev3_init_cpufreq(void) { return 0; }
#endif

/*
 * EV3 I2C board configuration:
 * ============================
 * This is the I2C that communicates with other devices on the main board,
 * not the input ports. (Using I2C0)
 * Devices are:
 * - EEPROM (24c128) to get the hardware version and the bluetooth MAC
 *	address for the EV3.
 * - The bluetooth module (PIC_*) for lms2012 Mode 2 communications,
 *	whatever that is (see c_i2c.c in lms2012).
 */

static const short legoev3_i2c_board_pins[] __initconst = {
	EV3_I2C_BOARD_SDA, EV3_I2C_BOARD_SCL,
	-1
};

static struct i2c_board_info __initdata legoev3_i2c_board_devices[] = {
	{
		I2C_BOARD_INFO("24c128", 0x50),
	},
	{
		I2C_BOARD_INFO("PIC_CodedDataTo", 0x54),
	},
	{
		I2C_BOARD_INFO("PIC_ReadStatus", 0x55),
	},
	{
		I2C_BOARD_INFO("PIC_RawDataTo", 0x56),
	},
	{
		I2C_BOARD_INFO("PIC_ReadDataFrom", 0x57),
	},
};

static struct davinci_i2c_platform_data legoev3_i2c_board_pdata = {
	.bus_freq	= 400	/* kHz */,
	.bus_delay	= 0	/* usec */,
};

/*
 * EV3 UART configuration:
 * =======================
 * UART0 is used for Input Port 1. It is also the debug terminal.
 * UART1 is used for Input Port 2.
 * UART2 is used to communicate to the bluetooth chip.
 * Input Ports 3 and 4 get software UARTs via the PRU.
 */

static struct davinci_uart_config legoev3_uart_config = {
	.enabled_uarts = 0x7,
};

#warning "Fix up the pru_suart code here so it works"
static int __init da850_evm_config_pru_suart(void)
{
	int ret;

	pr_info("da850_evm_config_pru_suart configuration\n");
	if (!machine_is_davinci_da850_evm())
		return 0;

// FIXME: Add pru suart setup
//    ret = da8xx_pinmux_setup(da850_pru_suart_pins);
//    if (ret)
//        pr_warning("legoev3_init: da850_pru_suart_pins mux setup failed: %d\n",
//                ret);
//
//    ret = da8xx_register_pru_suart();
//    if (ret)
//        pr_warning("legoev3_init: pru suart registration failed: %d\n", ret);
      return ret;
}
device_initcall(da850_evm_config_pru_suart);

#warning "Are these EDMA initializers really needed?"
/*
 * The following EDMA channels/slots are not being used by drivers (for
 * example: Timer, GPIO, UART events etc) on da850/omap-l138 EVM, hence
 * they are being reserved for codecs on the DSP side.
 */
static const s16 da850_dma0_rsv_chans[][2] = {
	/* (offset, number) */
	{ 8,  6},
	{24,  4},
	{30,  2},
	{-1, -1}
};

static const s16 da850_dma0_rsv_slots[][2] = {
	/* (offset, number) */
	{ 8,  6},
	{24,  4},
	{30, 50},
	{-1, -1}
};

static const s16 da850_dma1_rsv_chans[][2] = {
	/* (offset, number) */
	{ 0, 28},
	{30,  2},
	{-1, -1}
};

static const s16 da850_dma1_rsv_slots[][2] = {
	/* (offset, number) */
	{ 0, 28},
	{30, 90},
	{-1, -1}
};

static struct edma_rsv_info da850_edma_cc0_rsv = {
	.rsv_chans      = da850_dma0_rsv_chans,
	.rsv_slots      = da850_dma0_rsv_slots,
};

static struct edma_rsv_info da850_edma_cc1_rsv = {
	.rsv_chans      = da850_dma1_rsv_chans,
	.rsv_slots      = da850_dma1_rsv_slots,
};

static struct edma_rsv_info *da850_edma_rsv[2] = {
	&da850_edma_cc0_rsv,
	&da850_edma_cc1_rsv,
};

/*
 * EV3 analog/digital converter configuration:
 * ===========================================
 * The A/D converter is a TI ADS7957. It monitors analog inputs from
 * each of the 4 input ports, motor voltage and current for each of the
 * 4 ouput ports and the voltage. The A/D chip is connected to
 * the processor via SPI0. We are using the linux hwmon class to read
 * the raw voltages in userspace.
 */

static const short legoev3_adc_pins[] __initconst = {
	EV3_ADC_DATA_IN, EV3_ADC_DATA_OUT, EV3_ADC_CS, EV3_ADC_CLK, EV3_ADC_ENA,
	-1
};

static struct legoev3_analog_platform_data legoev3_adc_platform_data = {
	.in_pin1_ch	= { 6, 8, 10, 12 },
	.in_pin6_ch	= { 5, 7, 9, 11 },
	.out_pin5_ch	= { 1, 0, 13, 14 },
	.batt_volt_ch	= 4,
	.batt_curr_ch	= 3,
};

static struct davinci_spi_config legoev3_spi_adc_cfg = {
	.io_type	= SPI_IO_TYPE_POLL,
	.c2tdelay	= 10,
	.t2cdelay	= 10,
	.t2edelay	= 10,
	.c2edelay	= 10,
};

static struct spi_board_info legoev3_spi0_board_info[] = {
	/*
	 * We have to have 4 devices or the spi driver will fail to load because
	 * chip_select >= ARRAY_SIZE(legoev3_spi0_board_info). 0 - 2 are not
	 * actually used.
	 */
	[0] = {
		.chip_select		= 0,
	},
	[1] = {
		.chip_select		= 1,
	},
	[2] = {
		.chip_select		= 2,
	},
	[3] = {
		.modalias		= "legoev3-analog",
		.platform_data		= &legoev3_adc_platform_data,
		.controller_data	= &legoev3_spi_adc_cfg,
		.mode			= SPI_MODE_0,
		.max_speed_hz		= 20000000,
		.bus_num		= 0,
		.chip_select		= 3,
	},
};

/*
 * EV3 input and output port configuration:
 * ========================================
 * These are the input and output ports on the EV3 brick that connect to
 * sensors and motors.
 *
 * Note: The i2c clock and uart transmit functions of the input port share
 * the same physical pin on the chip, so instead of including them in the
 * board pin mux, they are passed to input port driver which performs the
 * mux as necessary when devices are added and removed. I2C device ids start
 * at 3 because the SoC I2Cs use 1 and 2.
 */

static const short legoev3_in_out_pins[] __initconst = {
	EV3_IN1_PIN1, EV3_IN1_PIN2, EV3_IN1_PIN5, EV3_IN1_PIN6, EV3_IN1_BUF_ENA,
	EV3_IN1_UART_TXD, EV3_IN1_UART_RXD,
	EV3_IN2_PIN1, EV3_IN2_PIN2, EV3_IN2_PIN5, EV3_IN2_PIN6, EV3_IN2_BUF_ENA,
	EV3_IN2_UART_TXD, EV3_IN2_UART_RXD,
	EV3_IN3_PIN1, EV3_IN3_PIN2, EV3_IN3_PIN5, EV3_IN3_PIN6, EV3_IN3_BUF_ENA,
	EV3_IN4_PIN1, EV3_IN4_PIN2, EV3_IN4_PIN5, EV3_IN4_PIN6, EV3_IN4_BUF_ENA,
	EV3_OUT1_PIN1, EV3_OUT1_PIN2, EV3_OUT1_PIN5, EV3_OUT1_PIN5_INT, EV3_OUT1_PIN6_DIR,
	EV3_OUT2_PIN1, EV3_OUT2_PIN2, EV3_OUT2_PIN5, EV3_OUT2_PIN5_INT, EV3_OUT2_PIN6_DIR,
	EV3_OUT3_PIN1, EV3_OUT3_PIN2, EV3_OUT3_PIN5, EV3_OUT3_PIN5_INT, EV3_OUT3_PIN6_DIR,
	EV3_OUT4_PIN1, EV3_OUT4_PIN2, EV3_OUT4_PIN5, EV3_OUT4_PIN5_INT, EV3_OUT4_PIN6_DIR,
	EV3_OUT1_PWM, EV3_OUT2_PWM, EV3_OUT3_PWM, EV3_OUT4_PWM, EV3_FIQ_STAT,
	-1
};

static struct legoev3_ports_platform_data legoev3_ports_data = {
	.input_port_data = {
		{
			.id			= EV3_PORT_IN1,
			.pin1_gpio		= EV3_IN1_PIN1_PIN,
			.pin2_gpio		= EV3_IN1_PIN2_PIN,
			.pin5_gpio		= EV3_IN1_PIN5_PIN,
			.pin6_gpio		= EV3_IN1_PIN6_PIN,
			.buf_ena_gpio		= EV3_IN1_BUF_ENA_PIN,
			.i2c_clk_gpio		= EV3_IN1_I2C_CLK_PIN,
			.i2c_dev_id		= 3,
			.i2c_pin_mux		= EV3_IN1_I2C_CLK,
			.uart_pin_mux		= EV3_IN1_UART_TXD,
		},
		{
			.id			= EV3_PORT_IN2,
			.pin1_gpio		= EV3_IN2_PIN1_PIN,
			.pin2_gpio		= EV3_IN2_PIN2_PIN,
			.pin5_gpio		= EV3_IN2_PIN5_PIN,
			.pin6_gpio		= EV3_IN2_PIN6_PIN,
			.buf_ena_gpio		= EV3_IN2_BUF_ENA_PIN,
			.i2c_clk_gpio		= EV3_IN2_I2C_CLK_PIN,
			.i2c_dev_id		= 4,
			.i2c_pin_mux		= EV3_IN2_I2C_CLK,
			.uart_pin_mux		= EV3_IN2_UART_TXD,
		},
		{
			.id			= EV3_PORT_IN3,
			.pin1_gpio		= EV3_IN3_PIN1_PIN,
			.pin2_gpio		= EV3_IN3_PIN2_PIN,
			.pin5_gpio		= EV3_IN3_PIN5_PIN,
			.pin6_gpio		= EV3_IN3_PIN6_PIN,
			.buf_ena_gpio		= EV3_IN3_BUF_ENA_PIN,
			.i2c_clk_gpio		= EV3_IN3_I2C_CLK_PIN,
			.i2c_dev_id		= 5,
			.i2c_pin_mux		= EV3_IN3_I2C_CLK,
			.uart_pin_mux		= EV3_IN3_UART_TXD,
		},
		{
			.id			= EV3_PORT_IN4,
			.pin1_gpio		= EV3_IN4_PIN1_PIN,
			.pin2_gpio		= EV3_IN4_PIN2_PIN,
			.pin5_gpio		= EV3_IN4_PIN5_PIN,
			.pin6_gpio		= EV3_IN4_PIN6_PIN,
			.buf_ena_gpio		= EV3_IN4_BUF_ENA_PIN,
			.i2c_clk_gpio		= EV3_IN4_I2C_CLK_PIN,
			.i2c_dev_id		= 6,
			.i2c_pin_mux		= EV3_IN4_I2C_CLK,
			.uart_pin_mux		= EV3_IN4_UART_TXD,
		},
	},
	.output_port_data = {
		{
			.id			= EV3_PORT_OUT1,
			.pin1_gpio		= EV3_OUT1_PIN1_PIN,
			.pin2_gpio		= EV3_OUT1_PIN2_PIN,
			.pin5_gpio		= EV3_OUT1_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT1_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT1_PIN6_DIR_PIN,
			.pwm_gpio		= EV3_OUT1_PWM,
			.pwm_dev_name		= "ehrpwm.1:1",
		},
		{
			.id			= EV3_PORT_OUT2,
			.pin1_gpio		= EV3_OUT2_PIN1_PIN,
			.pin2_gpio		= EV3_OUT2_PIN2_PIN,
			.pin5_gpio		= EV3_OUT2_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT2_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT2_PIN6_DIR_PIN,
			.pwm_gpio		= EV3_OUT2_PWM,
			.pwm_dev_name		= "ehrpwm.1:0",
		},
		{
			.id			= EV3_PORT_OUT3,
			.pin1_gpio		= EV3_OUT3_PIN1_PIN,
			.pin2_gpio		= EV3_OUT3_PIN2_PIN,
			.pin5_gpio		= EV3_OUT3_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT3_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT3_PIN6_DIR_PIN,
			.pwm_gpio		= EV3_OUT3_PWM,
			.pwm_dev_name		= "ecap.0",
		},
		{
			.id			= EV3_PORT_OUT4,
			.pin1_gpio		= EV3_OUT4_PIN1_PIN,
			.pin2_gpio		= EV3_OUT4_PIN2_PIN,
			.pin5_gpio		= EV3_OUT4_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT4_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT4_PIN6_DIR_PIN,
			.pwm_gpio		= EV3_OUT4_PWM,
			.pwm_dev_name		= "ecap.1",
		},
	},
};

static struct platform_device legoev3_ports_device = {
	.name	= "legoev3-ports",
	.id	= -1,
	.dev	= {
		.platform_data	= &legoev3_ports_data,
	},
};

/*
 * EV3 FIQ (fast interrupt) configuration:
 * =======================================
 * The EV3 uses FIQs to get near-realtime performance for software emulated
 * devices.
 *
 * The input ports each can communicate via I2C. The AM1808 processor only has
 * two I2C ports and the NXT ultrasonic sensor requires a non-standard hack
 * to make it work, so we implement our own I2C using GPIO. In order to get
 * the required performance, we use a system timer and an FIQ interrupt.
 *
 * PCM playback on the EV3 speaker suffers from aliasing due to latency in
 * handling interrups.
 *
 * We can't use the platform device resource infrastructure here because we
 * are sneaking behind the back of the kernel with the FIQs.
 */

static struct legoev3_fiq_platform_data legoev3_in_port_i2c_platform_data = {
	.intc_mem_base		= DA8XX_CP_INTC_BASE,
	.intc_mem_size		= 0x608,
	.gpio_mem_base		= DA8XX_GPIO_BASE,
	.gpio_mem_size		= 0xD8,
	.ehrpwm_mem_base	= DA8XX_EHRPWM0_BASE,
	.ehrpwm_mem_size	= 0x1FFF,
	.timer_irq		= IRQ_DA8XX_TINT34_1,
	.ehrpwm_irq		= IRQ_DA8XX_EHRPWM0,
	.status_gpio		= EV3_FIQ_STAT_PIN,
};

static struct platform_device legoev3_in_port_i2c_fiq = {
	.name		= "legoev3-fiq",
	.id		= -1,
	.dev		= {
		.platform_data	= &legoev3_in_port_i2c_platform_data,
	},
};

/*
 * EV3 sound configuration:
 * ========================
 * The speaker is driven by eHRPWM0B and the amplifier is switched on/off
 * using the EV3_SND_ENA_PIN. The snd-legoev3 driver can be used to play sounds
 * and also provides a standard sound event input for system beep/bell.
 */

static const short legoev3_sound_pins[] __initconst = {
	EV3_SND_PWM, EV3_SND_ENA,
	-1
};

#if defined(CONFIG_SND_LEGOEV3) || defined(CONFIG_SND_LEGOEV3_MODULE)
#include <sound/legoev3.h>

static struct snd_legoev3_platform_data ev3_snd_data = {
	.pwm_dev_name	= "ehrpwm.0:1",
	.amp_gpio	= EV3_SND_ENA_PIN,
};

static struct platform_device snd_legoev3 =
{
	.name	= "snd-legoev3",
	.id	= -1,
	.dev	= {
		.platform_data = &ev3_snd_data,
	},
};
#endif

/*
 * EV3 PLL clock configuration:
 * ============================
 */

static __init int da850_set_emif_clk_rate(void)
{
	struct clk *emif_clk;

	emif_clk = clk_get(NULL, "pll0_sysclk3");
	if (WARN(IS_ERR(emif_clk), "Unable to get emif clock\n"))
		return PTR_ERR(emif_clk);

	return clk_set_rate(emif_clk, CONFIG_DA850_FIX_PLL0_SYSCLK3RATE);
}

/*
 * EV3 power configuration:
 * ========================
 * System power is switched off via gpio.
 * The battery voltage and current are monitored and there is a switch to
 * indicate if a rechargable battery pack is being used.
 */

static const short legoev3_power_pins[] __initconst = {
	EV3_SYS_POWER_ENA, EV3_SYS_5V_POWER, EV3_BATT_TYPE,
	-1
};

static struct gpio legoev3_sys_power_gpio[] = {
	{
		.gpio	= EV3_SYS_POWER_ENA_PIN,
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "System power enable",
	},
	{
		.gpio	= EV3_SYS_5V_POWER_PIN,
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "System 5V power enable",
	},
};

static void legoev3_power_off(void)
{
	int ret;

	ret = gpio_direction_output(EV3_SYS_5V_POWER_PIN, 0);
	if (ret)
		pr_err("legoev3_init: can not set GPIO %d for power off\n",
			EV3_SYS_5V_POWER_PIN);
}

static struct legoev3_battery_platform_data ev3_battery_data = {
	.batt_type_gpio		= EV3_BATT_TYPE_PIN,
};

static struct platform_device legoev3_battery_device = {
	.name	= "legoev3-battery",
	.id	= -1,
	.dev	= {
		.platform_data	= &ev3_battery_data,
	},
};

/*
 * EV3 init:
 * =========
 * Initalization items generally follow this pattern:
 *	- comment describing the device(s) we are about to configure
 *	- perform pin mux and any other hardware configuration
 *	- if the driver is enabled, the register the device that uses the driver
 */

static __init void legoev3_init(void)
{
	int ret;
	u8 ehrpwm_mask = 0;

	/* Disable all internal pullup/pulldown resistors */
	ret = __raw_readl(DA8XX_SYSCFG1_VIRT(DA8XX_PUPD_ENA_REG));
	ret &= ~0xFFFFFFFF;
	__raw_writel(ret, DA8XX_SYSCFG1_VIRT(DA8XX_PUPD_ENA_REG));

	/* Support for EV3 LCD */
	ret = davinci_cfg_reg_list(legoev3_lcd_pins);
	if (ret)
		pr_warning("legoev3_init: LCD pin mux setup failed:"
			" %d\n", ret);
#if defined(CONFIG_FB_ST7586) || defined(CONFIG_FB_ST7586_MODULE)
	ret = da8xx_register_spi(1, legoev3_spi1_board_info,
				 ARRAY_SIZE(legoev3_spi1_board_info));
	if (ret)
		pr_warning("legoev3_init: spi1/framebuffer registration failed:"
			" %d\n", ret);
#endif

	/* Support for EV3 LEDs */
	ret = davinci_cfg_reg_list(legoev3_led_pins);
	if (ret)
		pr_warning("legoev3_init: LED mux setup failed:"
			" %d\n", ret);
#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
	ret = platform_device_register(&ev3_device_gpio_leds);
	if (ret)
		pr_warning("legoev3_init: LED registration failed:"
			" %d\n", ret);
#endif

	/* Support for EV3 buttons */
	ret = davinci_cfg_reg_list(legoev3_button_pins);
	if (ret)
		pr_warning("legoev3_init: Button mux setup failed:"
			" %d\n", ret);

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
	ret = platform_device_register(&ev3_device_gpiokeys);
	if (ret)
	pr_warning("legoev3_init: button registration failed:"
		" %d\n", ret);
#endif

	/*
	 * Though bootloader takes care to set emif clock at allowed
	 * possible rate. Kernel needs to reconfigure this rate to
	 * support platforms requiring fixed emif clock rate.
	 */
	ret = da850_set_emif_clk_rate();
	if (ret)
		pr_warning("legoev3_init: Failed to set rate of pll0_sysclk3/emif clock: %d\n",
				ret);

	ret = da850_register_edma(da850_edma_rsv);
	if (ret)
		pr_warning("legoev3_init: edma registration failed: %d\n",
				ret);

	ret = da8xx_register_watchdog();
	if (ret)
		pr_warning("da830_evm_init: watchdog registration failed: %d\n",
				ret);

	/* support for board-level I2C */
	ret = davinci_cfg_reg_list(legoev3_i2c_board_pins);
	if (ret)
		pr_warning("legoev3_init: board i2c mux setup failed: %d\n",
				ret);
	i2c_register_board_info(1, legoev3_i2c_board_devices,
				ARRAY_SIZE(legoev3_i2c_board_devices));
	da8xx_register_i2c(0, &legoev3_i2c_board_pdata);


	/* Analog/Digital converter support */
	ret = davinci_cfg_reg_list(legoev3_adc_pins);
	if (ret)
		pr_warning("legoev3_init: A/D converter mux setup failed:"
			" %d\n", ret);
#if defined(CONFIG_LEGOEV3_ANALOG) || defined(CONFIG_LEGOEV3_ANALOG_MODULE)
	ret = da8xx_register_spi(0, legoev3_spi0_board_info,
				 ARRAY_SIZE(legoev3_spi0_board_info));
	if (ret)
		pr_warning("legoev3_init: spi0/analog registration failed: %d\n",
				ret);
#endif

	/* Support for EV3 power */
	ret = davinci_cfg_reg_list(legoev3_power_pins);
	if (ret)
		pr_warning("legoev3_init: power pin mux setup failed:"
			" %d\n", ret);
	pm_power_off = legoev3_power_off;
	/* request the power gpios so that they cannot be accidentally used */
	ret = gpio_request_array(legoev3_sys_power_gpio,
				 ARRAY_SIZE(legoev3_sys_power_gpio));
	if (ret)
		pr_warning("legoev3_init: requesting power pins failed:"
			" %d\n", ret);
#if defined(CONFIG_BATTERY_LEGOEV3) || defined(CONFIG_BATTERY_LEGOEV3_MODULE)
	ret = platform_device_register(&legoev3_battery_device);
	if (ret)
		pr_warning("legoev3_init: battery registration failed:"
			" %d\n", ret);
#endif

	/* Input/Output port support */
	ret = davinci_cfg_reg_list(legoev3_in_out_pins);
	if (ret)
		pr_warning("legoev3_init: "
			"input port pin mux failed: %d\n", ret);

#if defined(CONFIG_LEGOEV3_DEV_PORTS) || defined(CONFIG_LEGOEV3_DEV_PORTS_MODULE)
	ret = platform_device_register(&legoev3_ports_device);
	if (ret)
		pr_warning("legoev3_init: "
			"input/output port registration failed: %d\n", ret);

	legoev3_hires_timer_init();
#endif

#if defined(CONFIG_LEGOEV3_FIQ)
	ret = platform_device_register(&legoev3_in_port_i2c_fiq);
	if (ret)
		pr_warning("legoev3_init: "
			"FIQ I2C backend registration failed: %d\n", ret);
#endif

	/* Sound support */
	ret = davinci_cfg_reg_list(legoev3_sound_pins);
	if (ret)
		pr_warning("legoev3_init: "
			"sound mux setup failed: %d\n", ret);

#if defined(CONFIG_SND_LEGOEV3) || defined(CONFIG_SND_LEGOEV3_MODULE)
	/*
	 * TODO:
	 * We should probably give the sound driver ownership of this gpio
	 * instead of requesting it here.
	 */
	ret = gpio_request_one(EV3_SND_ENA_PIN, GPIOF_OUT_INIT_LOW, "snd_ena");
	if (ret)
		pr_warning("legoev3_init:"
			" sound gpio setup failed: %d\n", ret);

	ret = platform_device_register(&snd_legoev3);
	if (ret)
		pr_warning("legoev3_init: "
			"sound device registration failed: %d\n", ret);

	/* eHRPWM0B is used to drive the EV3 speaker */
	ehrpwm_mask |= 0x2;
#endif

	/* eHRPWM support */
	/* pin mux is set in output port and sound support */
#if defined(CONFIG_DAVINCI_EHRPWM) || defined(CONFIG_DAVINCI_EHRPWM_MODULE)
	da850_register_ehrpwm(ehrpwm_mask);
#endif

	/* SD card support */
	ret = davinci_cfg_reg_list(legoev3_sd_pins);
	if (ret)
		pr_warning("legoev3_init: mmcsd0 mux setup failed:"
				" %d\n", ret);
#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
	ret = da8xx_register_mmcsd0(&legoev3_sd_config);
	if (ret)
		pr_warning("legoev3_init: mmcsd0 registration failed:"
					" %d\n", ret);
#endif

	/* real-time clock support */
	ret = da8xx_register_rtc();
	if (ret)
		pr_warning("legoev3_init: rtc setup failed: %d\n", ret);

#warning "Is legoev3_init_cpufreq needed?"
	ret = legoev3_init_cpufreq();
	if (ret)
		pr_warning("legoev3_init: cpufreq registration failed: %d\n",
				ret);

#warning "Is da8xx_register_cpuidle needed?"
	ret = da8xx_register_cpuidle();
	if (ret)
		pr_warning("legoev3_init: cpuidle registration failed: %d\n",
				ret);

#warning "Is da850_pm_device needed?"
	ret = da850_register_pm(&da850_pm_device);
	if (ret)
		pr_warning("legoev3_init: suspend registration failed: %d\n",
				ret);

	/* USB support */
	legoev3_usb_init();

	/* Bluetooth support */
	ret = davinci_cfg_reg_list(legoev3_bt_pins);
	if (ret)
		pr_warning("legoev3_init: bluetooth pin mux setup failed:"
			   " %d\n", ret);
	ret = da850_register_ecap(2);
	if (ret)
		pr_warning("legoev3_init: registering bluetooth clock ecap device failed:"
			   " %d\n", ret);
	ret = platform_device_register(&legoev3_bt_device);
	if (ret)
		pr_warning("legoev3_init: registering on-board bluetooth failed:"
			   " %d\n", ret);

	/* UART support - used by input ports and bluetooth */
	davinci_serial_init(&legoev3_uart_config);

	/* ecap and hrpwm for output port support */
	ret = da850_register_ecap(0);
	if (ret)
		pr_warning("legoev3_init: registering ecap0 failed:"
			   " %d\n", ret);
	ret = da850_register_ecap(1);
	if (ret)
		pr_warning("legoev3_init: registering ecap1 failed:"
			   " %d\n", ret);

#if defined(CONFIG_DAVINCI_EHRPWM) || defined(CONFIG_DAVINCI_EHRPWM_MODULE)
	ehrpwm_mask = 0xC;
	da850_register_ehrpwm(ehrpwm_mask);
#endif
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static int __init da850_evm_console_init(void)
{
	return add_preferred_console("ttyS", 1, "115200");
}
console_initcall(da850_evm_console_init);
#endif

static void __init legoev3_map_io(void)
{
	da850_init();
}

#warning "Make sure DA850_LEGOEV3 gets added to the ARM Build Database!!!"
// FIXME: Figure out the right dma_zone_size
//MACHINE_START(DA850_LEGOEV3, "LEGO MINDSTORMS EV3 Programmable Brick")
MACHINE_START(DAVINCI_DA850_EVM, "DaVinci DA850/OMAP-L138/AM18x EVM")
	.atag_offset	= 0x100,
	.map_io		= legoev3_map_io,
	.init_irq	= cp_intc_init,
	.timer		= &davinci_timer,
	.init_machine	= legoev3_init,
	.dma_zone_size	= SZ_64M,
	.restart	= da8xx_restart,
MACHINE_END
