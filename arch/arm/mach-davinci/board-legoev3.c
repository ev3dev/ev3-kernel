/*
 * LEGO MINDSTORMS EV3 DA850/OMAP-L138
 *
 * Copyright (C) 2013 Ralph Hempel
 * Copyright (C) 2012 David Lechner <david@lechnology.com>
 *
 * Derived from: arch/arm/mach-davinci/board-da850-evm.c
 * Original Copyrights follow:
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
#include <linux/hwmon/ads79xx.h>
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
#include <linux/i2c-gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/cp_intc.h>
#include <mach/da8xx.h>
#include <mach/legoev3.h>
#include <mach/nand.h>
#include <mach/mux.h>
#include <mach/spi.h>
#include <mach/usb.h>

#include <video/st7586fb.h>

/* LCD configuration:
 *
 * The LCD is connected via SPI1 and uses the st7586fb frame buffer driver.
 */

static const short legoev3_lcd_pins[] = {
	EV3_LCD_DATA_IN, EV3_LCD_RESET, EV3_LCD_A0, EV3_LCD_CS
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
		.chip_select		= 0,
	},
};

static const short legoev3_led_pins[] = {
	EV3_LED_0, EV3_LED_1, EV3_LED_2, EV3_LED_3,
	-1
};

/* LED configuration:
 *
 * The LEDs are connected via GPIOs and use the leds-gpio driver.
 * The default triggers are set so that during boot, the left LED will flash
 * amber to indicate CPU activity and the right LED will flash amber to
 * indicate disk (SD card) activity.
 */

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

/* EV3 button configuration:
 *
 * The buttons are connected via GPIOs and use the gpio-keys driver.
 * The buttons are mapped to the UP, DOWN, LEFT, RIGHT, ENTER and ESC keys.
 */

static const short legoev3_button_pins[] = {
	EV3_BUTTON_0, EV3_BUTTON_1, EV3_BUTTON_2,
	EV3_BUTTON_3, EV3_BUTTON_4, EV3_BUTTON_5,
	-1
};

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#include <linux/gpio_keys.h>
#include<linux/input.h>

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

static const int legoev3_button_gpio[] = {
	EV3_BUTTON_0_PIN,
	EV3_BUTTON_1_PIN,
	EV3_BUTTON_2_PIN,
	EV3_BUTTON_3_PIN,
	EV3_BUTTON_4_PIN,
	EV3_BUTTON_5_PIN
};
#endif

/* what is pm? */

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

#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
#define HAS_MMC 1
#else
#define HAS_MMC 0
#endif

static struct i2c_board_info __initdata da850_evm_i2c_devices[] = {
	{
		I2C_BOARD_INFO("24FC128", 0x50),
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

/* EV3 USB configuration:
 */

static const short legoev3_usb1_pins[] = {
	EV3_USB1_DRV, EV3_USB1_OVC,
	-1
};

static irqreturn_t legoev3_usb_ocic_irq(int, void *);

static struct da8xx_ohci_root_hub legoev3_usb1_pdata = {
	.type	= GPIO_BASED,
	.method	= {
		.gpio_method = {
			.power_control_pin	= EV3_USB1_DRV_PIN,
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

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Fixup the da850_evm_bt_slow_clock_init "
/* Bluetooth Slow clock init using ecap 2 */
static __init void da850_evm_bt_slow_clock_init(void)						// LEGO BT
{												// LEGO BT		
//  int PSC1;											// LEGO BT
												// LEGO BT
// FIXME: These registers need proper mapping to HW
//  PSC1 = __raw_readl(DA8XX_PSC1_VIRT(0x294 * 4));  // Old PSC1 is 32bit -> explains "* 4"	// LEGO BT
//  PSC1 |= 3;											// LEGO BT
//  __raw_writel(PSC1, DA8XX_PSC1_VIRT(0x294 * 4));						// LEGO BT
//												// LEGO BT
//  PSC1 = __raw_readl(DA8XX_PSC1_VIRT(0x48 * 4));						// LEGO BT
//  PSC1 |= 3;											// LEGO BT
//  __raw_writel(PSC1, DA8XX_PSC1_VIRT(0x48 * 4));						// LEGO BT
//
//  PSC1 = __raw_readl(DA8XX_SYSCFG1_VIRT(0x3 * 4));						// LEGO BT
//  PSC1 &= ~0x00000004;										// LEGO BT
//  __raw_writel(PSC1, DA8XX_SYSCFG1_VIRT(0x3 * 4));						// LEGO BT
//
//  __raw_writel(0,      DA8XX_ECAP2_VIRT(0 * 2));     // Old ECAP is 16bit -> explains "* 2"     // LEGO BT
//  __raw_writel(0,      DA8XX_ECAP2_VIRT(2 * 2));     //						// LEGO BT
//  __raw_writew(0x0690, DA8XX_ECAP2_VIRT(0x15 * 2));  // Setup					// LEGO BT
//  __raw_writel(2014,   DA8XX_ECAP2_VIRT(0x06 * 2));  // Duty					// LEGO BT
//  __raw_writel(4028,   DA8XX_ECAP2_VIRT(0x04 * 2));  // Freq					// LEGO BT
}
#endif

#warning "Are these definitions just the dumb way of doing this?"
#define  DA850_ECAP2_OUT		        GPIO_TO_PIN(0, 7)      //LEGO BT
#define  DA850_ECAP2_OUT_ENABLE		GPIO_TO_PIN(0, 12)      //LEGO BT

// static const short da850_bt_slow_clock_pins[] __initconst = {
// 	EV3_ECAP2_OUT, EV3_ECAP2_OUT_ENABLE,
// 	-1
// };

#warning "Are these definitions just the dumb way of doing this?"
#define  DA850_BT_SHUT_DOWN		GPIO_TO_PIN(4, 1)      //LEGO BT
#define  DA850_BT_SHUT_DOWN_EP2		GPIO_TO_PIN(4, 9)      //LEGO BT

// static const short da850_bt_shut_down_pins[] __initconst = {
// 	EV3_GPIO4_1, EV3_GPIO4_9,
// 	-1
// };

static struct davinci_i2c_platform_data lego_i2c_0_pdata = {
	.bus_freq	= 400	/* kHz */,
	.bus_delay	= 0	/* usec */,
};

static struct davinci_uart_config da850_evm_uart_config __initdata = {
	.enabled_uarts = 0x7,
};

static int da850_evm_mmc_get_ro(int index)
{
//	return gpio_get_value(DA850_MMCSD_WP_PIN);
//        return( 0 );
}

static int da850_evm_mmc_get_cd(int index)
{
//	return !gpio_get_value(DA850_MMCSD_CD_PIN);
//          return( 1 );
}
static struct davinci_mmc_config da850_mmc_config = {
//	.get_ro		= da850_evm_mmc_get_ro,
//	.get_cd		= da850_evm_mmc_get_cd,
	.wires		= 4,
	.max_freq	= 50000000,
	.caps		= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.version	= MMC_CTLR_VERSION_2,
};

static const short da850_evm_mmcsd0_pins[] __initconst = {
	DA850_MMCSD0_DAT_0, DA850_MMCSD0_DAT_1, DA850_MMCSD0_DAT_2,
	DA850_MMCSD0_DAT_3, DA850_MMCSD0_CLK, DA850_MMCSD0_CMD,
	DA850_GPIO4_0, DA850_GPIO4_1,
	-1
};

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

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Eliminate this code and eliminate this warning after copying this file to board-legoev3.c"
#warning "This may be needed to handle the BT init???"
#else
#ifdef CONFIG_DA850_WL12XX

static void wl12xx_set_power(int index, bool power_on)
{
	static bool power_state;

	pr_debug("Powering %s wl12xx", power_on ? "on" : "off");

	if (power_on == power_state)
		return;
	power_state = power_on;

	if (power_on) {
		/* Power up sequence required for wl127x devices */
		gpio_set_value_cansleep(DA850_WLAN_EN, 1);
		usleep_range(15000, 15000);
		gpio_set_value_cansleep(DA850_WLAN_EN, 0);
		usleep_range(1000, 1000);
		gpio_set_value_cansleep(DA850_WLAN_EN, 1);
		msleep(70);
	} else {
		gpio_set_value_cansleep(DA850_WLAN_EN, 0);
	}
}

static struct davinci_mmc_config da850_wl12xx_mmc_config = {
	.set_power	= wl12xx_set_power,
	.wires		= 4,
	.max_freq	= 25000000,
	.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_NONREMOVABLE |
			  MMC_CAP_POWER_OFF_CARD,
	.version	= MMC_CTLR_VERSION_2,
};

static const short da850_wl12xx_pins[] __initconst = {
	DA850_MMCSD1_DAT_0, DA850_MMCSD1_DAT_1, DA850_MMCSD1_DAT_2,
	DA850_MMCSD1_DAT_3, DA850_MMCSD1_CLK, DA850_MMCSD1_CMD,
	DA850_GPIO6_9, DA850_GPIO6_10,
	-1
};

static struct wl12xx_platform_data da850_wl12xx_wlan_data __initdata = {
	.irq			= -1,
	.board_ref_clock	= WL12XX_REFCLOCK_38,
	.platform_quirks	= WL12XX_PLATFORM_QUIRK_EDGE_IRQ,
};

static __init int da850_wl12xx_init(void)
{
	int ret;

	ret = davinci_cfg_reg_list(da850_wl12xx_pins);
	if (ret) {
		pr_err("wl12xx/mmc mux setup failed: %d\n", ret);
		goto exit;
	}

	ret = da850_register_mmcsd1(&da850_wl12xx_mmc_config);
	if (ret) {
		pr_err("wl12xx/mmc registration failed: %d\n", ret);
		goto exit;
	}

	ret = gpio_request_one(DA850_WLAN_EN, GPIOF_OUT_INIT_LOW, "wl12xx_en");
	if (ret) {
		pr_err("Could not request wl12xx enable gpio: %d\n", ret);
		goto exit;
	}

	ret = gpio_request_one(DA850_WLAN_IRQ, GPIOF_IN, "wl12xx_irq");
	if (ret) {
		pr_err("Could not request wl12xx irq gpio: %d\n", ret);
		goto free_wlan_en;
	}

	da850_wl12xx_wlan_data.irq = gpio_to_irq(DA850_WLAN_IRQ);

	ret = wl12xx_set_platform_data(&da850_wl12xx_wlan_data);
	if (ret) {
		pr_err("Could not set wl12xx data: %d\n", ret);
		goto free_wlan_irq;
	}

	return 0;

free_wlan_irq:
	gpio_free(DA850_WLAN_IRQ);

free_wlan_en:
	gpio_free(DA850_WLAN_EN);

exit:
	return ret;
}

#else /* CONFIG_DA850_WL12XX */

static __init int da850_wl12xx_init(void)
{
	return 0;
}

#endif /* CONFIG_DA850_WL12XX */
#endif

static struct i2c_gpio_platform_data da850_gpio_i2c_pdata = {
	.sda_pin	= GPIO_TO_PIN(1, 4),
	.scl_pin	= GPIO_TO_PIN(1, 5),
	.udelay		= 2,			/* 250 KHz */
};

static struct platform_device da850_gpio_i2c = {
	.name		= "i2c-gpio",
	.id		= 1,
	.dev		= {
		.platform_data	= &da850_gpio_i2c_pdata,
	},
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

/* EV3 analog/digital converter configuration:
 * The A/D converter is a TI ADS7957. It monitors analog inputs from
 * each of the 4 input ports, motor voltage and current for each of the
 * 4 ouput ports and the battery voltage. The A/D chip is connected to
 * the processor via SPI0. We are using the linux hwmon class to read 
 * the raw voltages in userspace.
 */

static const short legoev3_adc_pins[] = {
	EV3_ADC_DATA_IN, EV3_ADC_DATA_OUT, EV3_ADC_CS, EV3_ADC_CLK, EV3_ADC_ENA,
	-1
};

static struct ads79xx_platform_data legoev3_adc_platform_data = {
	.range		= ADS79XX_RANGE_5V,
	.mode		= ADS79XX_MODE_AUTO,
	.auto_update_ns	= 100000000, /* 100 msec */
};

static struct davinci_spi_config legoev3_spi_analog_cfg = {
	.io_type	= SPI_IO_TYPE_POLL,
	.c2tdelay	= 10,
	.t2cdelay	= 10,
	.t2edelay	= 10,
	.c2edelay	= 10,
};

static struct spi_board_info legoev3_spi0_board_info[] = {
	/* We have to have 4 devices or the spi driver will fail to load because
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
		.modalias		= "ads7957",
		.platform_data		= &legoev3_adc_platform_data,
		.controller_data	= &legoev3_spi_analog_cfg,
		.mode			= SPI_MODE_0,
		.max_speed_hz		= 20000000,
		.bus_num		= 0,
		.chip_select		= 3,
	},
};

/* EV3 sound configuration:
 * The speaker is driven by eHRPWM0B and the amplifier is switched on/off
 * using the EV3_SND_ENA_PIN. The snd-legoev3 driver can be used to play sounds
 * and also provides a standard sound event input for system beep/bell.
 */

static const short legoev3_sound_pins[] = {
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

static __init int da850_set_emif_clk_rate(void)
{
	struct clk *emif_clk;

	emif_clk = clk_get(NULL, "pll0_sysclk3");
	if (WARN(IS_ERR(emif_clk), "Unable to get emif clock\n"))
		return PTR_ERR(emif_clk);

	return clk_set_rate(emif_clk, CONFIG_DA850_FIX_PLL0_SYSCLK3RATE);
}

struct uio_pruss_pdata da8xx_pruss_uio_pdata = {
	.pintc_base	= 0x4000,
};

#define DA850EVM_SATA_REFCLKPN_RATE	(100 * 1000 * 1000)

/*
 * EV3 power configuration:
 */

static const short legoev3_power_pins[] = {
	EV3_SYS_5V_POWER, EV3_BATT_TYPE,
	-1
};

static void legoev3_power_off(void)
{
	if (!gpio_request(EV3_SYS_5V_POWER_PIN, "EV3 system 5V power enable"))
		gpio_direction_output(EV3_SYS_5V_POWER_PIN, 0);
	else
		pr_err("legoev3_init: can not open GPIO %d for power off\n",
			EV3_SYS_5V_POWER_PIN);
}

static struct legoev3_battery_platform_data ev3_battery_data = {
	.spi_dev_name	= "spi0.3",
	.batt_type_gpio	= EV3_BATT_TYPE_PIN,
	.adc_volt_ch	= 4,
	.adc_curr_ch	= 3,
};

static struct platform_device ev3_device_battery = {
	.name	= "legoev3-battery",
	.id	= -1,
	.dev	= {
		.platform_data	= &ev3_battery_data,
	},
};

/*
 * EV3 init:
 */

static __init void legoev3_init(void)
{
	int ret;
	char mask = 0;
	struct davinci_soc_info *soc_info = &davinci_soc_info;

	u8 rmii_en = soc_info->emac_pdata->rmii_en;

	/* Support for EV3 LCD */
	ret = davinci_cfg_reg_list(legoev3_lcd_pins);
	if (ret)
		pr_warning("legoev3_init: LCD pin mux setup failed:"
			" %d\n", ret);
	ret = da8xx_register_spi(1, legoev3_spi1_board_info,
				 ARRAY_SIZE(legoev3_spi1_board_info));
	if (ret)
		pr_warning("legoev3_init: spi 1 registration failed:"
			" %d\n", ret);

	/* Support for EV3 UI LEDs */
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

	/* Support for EV3 UI LEDs */
	ret = davinci_cfg_reg_list(legoev3_button_pins);
	if (ret)
		pr_warning("legoev3_init: Button mux setup failed:"
			" %d\n", ret);

	/*
	 * This is CRITICAL code to making the LEFT button work - it disables
	 * the internal pullup on pin group 25 which is where the GPIO6_6 lives.
	 */
	ret = __raw_readl(DA8XX_SYSCFG1_VIRT(DA8XX_PUPD_SEL_REG));
	ret &= 0xFDFFFFFF;
	__raw_writel(ret, DA8XX_SYSCFG1_VIRT(DA8XX_PUPD_SEL_REG));

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

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Eventually, we'll re-enable these pins!"
#else
	ret = davinci_cfg_reg_list(da850_i2c0_pins);
	if (ret)
		pr_warning("legoev3_init: i2c0 mux setup failed: %d\n",
				ret);

	platform_device_register(&da850_gpio_i2c);
#endif

	ret = da8xx_register_watchdog();
	if (ret)
		pr_warning("da830_evm_init: watchdog registration failed: %d\n",
				ret);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Let's find out which one of these UARTS we really need to update!"
#else
	/* Support for UART 1 */
	ret = davinci_cfg_reg_list(da850_uart1_pins);
	if (ret)
		pr_warning("legoev3_init: UART 1 mux setup failed:"
						" %d\n", ret);
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
	/* Support for UART 2 */
	ret = davinci_cfg_reg_list(da850_uart2_pins);
	if (ret)
		pr_warning("legoev3_init: UART 2 mux setup failed:"
						" %d\n", ret);
#endif

	/* Analog/Digital converter support */
	ret = davinci_cfg_reg_list(legoev3_adc_pins);
	if (ret)
		pr_warning("legoev3_init: A/D converter mux setup failed:"
			" %d\n", ret);

	ret = da8xx_register_spi(0, legoev3_spi0_board_info,
				 ARRAY_SIZE(legoev3_spi0_board_info));
	if (ret)
		pr_warning("legoev3_init: spi 0 registration failed: %d\n",
				ret);

	/* Support for EV3 power */
	ret = davinci_cfg_reg_list(legoev3_power_pins);
	if (ret)
		pr_warning("legoev3_init: power pin mux setup failed:"
			" %d\n", ret);
	pm_power_off = legoev3_power_off;

	ret = platform_device_register(&ev3_device_battery);
	if (ret)
		pr_warning("legoev3_init: battery registration failed:"
			" %d\n", ret);

	/* Sound support */
#if defined(CONFIG_DAVINCI_EHRPWM) || defined(CONFIG_DAVINCI_EHRPWM_MODULE)
	/* eHRPWM0B is used to drive the EV3 speaker */
	ret = davinci_cfg_reg_list(legoev3_sound_pins);
	if (ret)
		pr_warning("legoev3_init: "
			"sound mux setup failed: %d\n", ret);

	ret = gpio_request_one(EV3_SND_ENA_PIN, GPIOF_OUT_INIT_LOW, "snd_ena");
	if (ret)
		pr_warning("legoev3_init:"
			" sound gpio setup failed: %d\n", ret);

	da850_register_ehrpwm(0x02);
#endif	

#if defined(CONFIG_SND_LEGOEV3) || defined(CONFIG_SND_LEGOEV3_MODULE)
	ret = platform_device_register(&snd_legoev3);
	if (ret)
		pr_warning("legoev3_init: "
			"sound device registration failed: %d\n", ret);
#endif


#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Sort out this bluetooth init code! Needed for EV3"
#else
	pr_info("da850_evm_gpio_req_BT_EN\n");	
	
	ret = gpio_request(DA850_BT_EN, "WL1271_BT_EN");
	if (ret)
		pr_warning("legoev3_init: can not open BT GPIO %d\n",
					DA850_BT_EN);
	gpio_direction_output(DA850_BT_EN, 1);
	udelay(1000);
	gpio_direction_output(DA850_BT_EN, 0);
#endif

	if (HAS_MMC) {
		ret = davinci_cfg_reg_list(da850_evm_mmcsd0_pins);
		if (ret)
			pr_warning("legoev3_init: mmcsd0 mux setup failed:"
					" %d\n", ret);

		ret = da8xx_register_mmcsd0(&da850_mmc_config);
		if (ret)
			pr_warning("legoev3_init: mmcsd0 registration failed:"
					" %d\n", ret);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Sort out this bluetooth init code! Needed for EV3???? See WL1271"
#else
		ret = da850_wl12xx_init();
		if (ret)
			pr_warning("legoev3_init: wl12xx initialization"
				   " failed: %d\n", ret);
#endif
	}

	davinci_serial_init(&da850_evm_uart_config);

	i2c_register_board_info(1, da850_evm_i2c_devices,
			ARRAY_SIZE(da850_evm_i2c_devices));

#warning "Is da8xx_register_rtc needed?"
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

	legoev3_usb_init();

	if (gpio_request(DA850_BT_SHUT_DOWN, "bt_en")) {			// LEGO BT
		printk(KERN_ERR "Failed to request gpio DA850_BT_SHUT_DOWN\n");	// LEGO BT
		return;								// LEGO BT
	}									// LEGO BT

	if (gpio_request(DA850_BT_SHUT_DOWN_EP2, "bt_en_EP2")) {		// LEGO BT - EP2
		printk(KERN_ERR "Failed to request gpio DA850_BT_SHUT_DOWN\n");	// LEGO BT - EP2
		return;								// LEGO BT - EP2
	}									// LEGO BT - EP2

	gpio_set_value(DA850_BT_SHUT_DOWN_EP2, 0);				// LEGO BT - EP2
	gpio_direction_output(DA850_BT_SHUT_DOWN_EP2, 0);			// LEGO BT - EP2

	gpio_set_value(DA850_BT_SHUT_DOWN, 0);					// LEGO BT
	gpio_direction_output(DA850_BT_SHUT_DOWN, 0);				// LEGO BT

//	/* Support for Bluetooth shut dw pin */					// LEGO BT
//	pr_info("Support for Bluetooth shut dw pin\n");	
//	ret = davinci_cfg_reg_list(da850_bt_shut_down_pins);			// LEGO BT
//	if (ret)								// LEGO BT
//		pr_warning("legoev3_init: BT shut down mux setup failed:"	// LEGO BT
//						" %d\n", ret);			// LEGO BT

	gpio_set_value(DA850_BT_SHUT_DOWN, 0);					// LEGO BT
	gpio_set_value(DA850_BT_SHUT_DOWN_EP2, 0);				// LEGO BT - EP2

//       /* Support for Bluetooth slow clock */					// LEGO BT
//	ret = davinci_cfg_reg_list(da850_bt_slow_clock_pins);			// LEGO BT
//	if (ret)								// LEGO BT
//		pr_warning("legoev3_init: BT slow clock mux setup failed:"	// LEGO BT
//						" %d\n", ret);			// LEGO BT

	da850_evm_bt_slow_clock_init();						// LEGO BT
	gpio_direction_input(DA850_ECAP2_OUT_ENABLE);				// LEGO BT

	gpio_set_value(DA850_BT_SHUT_DOWN, 1);					// LEGO BT
	gpio_set_value(DA850_BT_SHUT_DOWN_EP2, 1);				// LEGO BT - EP2
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static int __init da850_evm_console_init(void)
{
	return add_preferred_console("ttyS", 1, "115200");
}
console_initcall(da850_evm_console_init);
#endif

static __init void da850_evm_irq_init(void)
{
	struct davinci_soc_info *soc_info = &davinci_soc_info;

	cp_intc_init();
}

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
