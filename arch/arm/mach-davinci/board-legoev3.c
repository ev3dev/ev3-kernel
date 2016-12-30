/*
 * LEGO MINDSTORMS EV3 - TI DA850/OMAP-L138
 *
 * Copyright (C) 2013 Ralph Hempel
 * Copyright (C) 2013-2015 David Lechner <david@lechnology.com>
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
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
#include <linux/leds_pwm.h>
#include <linux/memory.h>
#include <linux/platform_data/at24.h>
#include <linux/platform_data/legoev3.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/platform_data/pwm-gpio.h>
#include <linux/regulator/machine.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/vt_kern.h>
#include <linux/wl12xx.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/system_info.h>

#include "cp_intc.h"
#include <mach/da8xx.h>
#include <mach/legoev3.h>
#include <mach/legoev3-fiq.h>
#include <mach/mux.h>
#include <mach/time.h>

#include <sound/legoev3.h>

#include <video/st7586fb.h>
#include "../../../drivers/staging/fbtft/fbtft.h"

#include "board-legoev3.h"

/*
 * LCD configuration:
 * ==================
 * The LCD is connected via SPI1. The stock LCD uses the st7586fb frame buffer
 * driver. We also support the Adafruit 1.8" TFT which uses the fb_st7735r
 * frame buffer driver. The installed display is detected by looking at
 * EV3_LCD_CS_PIN. It is connected to the CS input on the stock LCD, which pulls
 * high. On the Adafruit, it is used to drive the backlight PWM, so it is just
 * floating.
 */

static const short legoev3_lcd_pins[] __initconst = {
	EV3_LCD_DATA_OUT, EV3_LCD_RESET, EV3_LCD_A0, EV3_LCD_CS
	-1
};

/* Stock LCD */

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

static struct spi_board_info legoev3_st7586fb_spi1_board_info[] = {
	{
		.modalias		= "st7586fb-legoev3",
		.platform_data		= &legoev3_st7586fb_data,
		.controller_data	= &legoev3_st7586fb_cfg,
		.mode			= SPI_MODE_3 | SPI_NO_CS,
		.max_speed_hz		= 10000000,
		.bus_num		= 1,
	},
};

/* Adafruit 1.8" TFT */

static const struct fbtft_platform_data legoev3_st7735r_data = {
	.display = {
		.buswidth = 8,
		.backlight = 1,
	},
	.rotate = 270,
	.gpios = (const struct fbtft_gpio []) {
		{ "reset", EV3_LCD_RESET_PIN },
		{ "dc", EV3_LCD_A0_PIN },
		{ "led", EV3_LCD_CS_PIN },
		{},
	},
};

static struct spi_board_info legoev3_st7735r_spi1_board_info[] = {
	{
		.modalias		= "fb_st7735r",
		.platform_data		= &legoev3_st7735r_data,
		.mode			= SPI_MODE_0 | SPI_NO_CS,
		.max_speed_hz		= 32000000,
		.bus_num		= 1,
	},
};

/*
 * LED configuration:
 * ==================
 * The LEDs are connected via GPIOs. We are using pwm-gpio so the LEDs can be
 * dimmed. The default triggers are set so that during boot, the left LED will
 * flash amber to indicate CPU activity and the right LED will flash amber to
 * indicate disk (SD card) activity.
 */

static const short legoev3_led_pins[] __initconst = {
	EV3_LED_0, EV3_LED_1, EV3_LED_2, EV3_LED_3,
	-1
};

static struct pwm_gpio ev3_led_pwms[] = {
	{
		.name = "ev3-left0-led", /* red */
		.gpio = EV3_LED_0_PIN,
	},
	{
		.name = "ev3-left1-led", /* green */
		.gpio = EV3_LED_1_PIN,
	},
	{
		.name = "ev3-right1-led", /* green */
		.gpio = EV3_LED_2_PIN,
	},
	{
		.name = "ev3-right0-led", /* red */
		.gpio = EV3_LED_3_PIN,
	},
};

static struct pwm_gpio_platform_data ev3_led_pwms_data = {
	.num_pwms = ARRAY_SIZE(ev3_led_pwms),
	.pwms = ev3_led_pwms,
};

static struct platform_device ev3_led_pwms_device = {
	.name	= "pwm-gpio",
	.id	= -1,
	.dev	= {
		.platform_data  = &ev3_led_pwms_data,
	},
};

static struct led_pwm ev3_leds[] = {
	{
		.name = "ev3:left:red:ev3dev",
		.default_trigger = "mmc0",
		.max_brightness = LED_FULL,
		.pwm_period_ns = NSEC_PER_SEC / 100,
	},
	{
		.name = "ev3:left:green:ev3dev",
		.default_trigger = "mmc0",
		.max_brightness = LED_FULL,
		.pwm_period_ns = NSEC_PER_SEC / 100,
	},
	{
		.name = "ev3:right:green:ev3dev",
		.default_trigger = "mmc0",
		.max_brightness = LED_FULL,
		.pwm_period_ns = NSEC_PER_SEC / 100,
	},
	{
		.name = "ev3:right:red:ev3dev",
		.default_trigger = "mmc0",
		.max_brightness = LED_FULL,
		.pwm_period_ns = NSEC_PER_SEC / 100,
	},
};

static struct led_pwm_platform_data ev3_leds_data = {
	.num_leds	= ARRAY_SIZE(ev3_leds),
	.leds		= ev3_leds
};

static struct platform_device ev3_leds_device = {
	.name	= "leds_pwm",
	.id	= -1,
	.dev	= {
		.platform_data  = &ev3_leds_data,
	},
};

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

static struct gpio_keys_button ev3_gpio_keys_table[] = {
	{KEY_UP,        EV3_BUTTON_0_PIN, 1, "ev3:UP",    EV_KEY, 0, 50, 1},
	{KEY_ENTER,     EV3_BUTTON_1_PIN, 1, "ev3:ENTER", EV_KEY, 0, 50, 1},
	{KEY_DOWN,      EV3_BUTTON_2_PIN, 1, "ev3:DOWN",  EV_KEY, 0, 50, 1},
	{KEY_RIGHT,     EV3_BUTTON_3_PIN, 1, "ev3:RIGHT", EV_KEY, 0, 50, 1},
	{KEY_LEFT,      EV3_BUTTON_4_PIN, 1, "ev3:LEFT",  EV_KEY, 0, 50, 1},
	{KEY_BACKSPACE, EV3_BUTTON_5_PIN, 1, "ev3:BACK",   EV_KEY, 0, 50, 1},
};

static struct gpio_keys_platform_data ev3_gpio_keys_data = {
	.buttons = ev3_gpio_keys_table,
	.nbuttons = ARRAY_SIZE(ev3_gpio_keys_table),
	.name = "EV3 buttons",
};

static struct platform_device ev3_device_gpiokeys = {
	.name = "gpio-keys",
	.dev = {
		.platform_data = &ev3_gpio_keys_data,
	},
};

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
 * usb1/USB1 is the USB 1.1 host port.
 * usb20 is the USB 2.0 OTG (On-The-Go) port.
 *
 * The over-current GPIO (EV3_USB1_OVC) is actually over-current for the
 * entire brick, not just the USB port.
 */

static const short legoev3_usb11_pins[] __initconst = {
	EV3_USB11_OVC,
	-1
};

static da8xx_ocic_handler_t legoev3_usb11_ocic_handler;

static int legoev3_usb11_get_oci(unsigned port)
{
	return !gpio_get_value(EV3_USB11_OVC_PIN);
}

static irqreturn_t legoev3_usb11_ocic_irq(int, void *);

static int legoev3_usb11_ocic_notify(da8xx_ocic_handler_t handler)
{
	int irq 	= gpio_to_irq(EV3_USB11_OVC_PIN);
	int error	= 0;

	if (handler != NULL) {
		legoev3_usb11_ocic_handler = handler;

		error = request_irq(irq, legoev3_usb11_ocic_irq,
				    IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				    "OHCI over-current indicator", NULL);
		if (error)
			printk(KERN_ERR "%s: could not request IRQ to watch "
			       "over-current indicator changes\n", __func__);
	} else
		free_irq(irq, NULL);

	return error;
}

static struct da8xx_ohci_root_hub legoev3_usb11_pdata = {
	.get_oci	= legoev3_usb11_get_oci,
	.ocic_notify	= legoev3_usb11_ocic_notify,
	.potpgt		= (3 + 1) / 2,	/* 3 ms max */
};

static irqreturn_t legoev3_usb11_ocic_irq(int irq, void *dev_id)
{
	legoev3_usb11_ocic_handler(&legoev3_usb11_pdata, 1);
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
		pr_warn("%s: USB 2.0 registration failed: %d\n", __func__, ret);

	/* initilaize usb module */
	ret = da8xx_register_usb11(&legoev3_usb11_pdata);
	if (ret)
		pr_warn("%s: USB 1.1 registration failed: %d\n", __func__, ret);
}

/*
 * EV3 bluetooth configuration:
 * ============================
 */

static const short legoev3_bt_pins[] __initconst = {
	EV3_BT_ENA, EV3_BT_CLK_ENA, EV3_BT_CLK, EV3_BT_UART_CTS,
	EV3_BT_UART_RTS, EV3_BT_UART_RXD, EV3_BT_UART_TXD,
	-1
};

static struct legoev3_bluetooth_platform_data legoev3_bt_pdata = {
	.bt_ena_gpio		= EV3_BT_ENA_PIN,
	.bt_clk_ena_gpio	= EV3_BT_CLK_ENA_PIN,
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
 * MicroSD does not have write protect.
 */

static const short legoev3_sd_pins[] __initconst = {
	EV3_SD_DAT_0, EV3_SD_DAT_1, EV3_SD_DAT_2,
	EV3_SD_DAT_3, EV3_SD_CLK, EV3_SD_CMD, EV3_SD_CD,
	-1
};

static int legoev3_mmc_get_ro(int index)
{
	/* MicroSD does not have r/w pin */
	return 0;
}

static int legoev3_mmc_get_cd(int index)
{
	return !gpio_get_value(EV3_SD_CD_PIN);
}

static struct davinci_mmc_config legoev3_sd_config = {
	.get_ro		= legoev3_mmc_get_ro,
	.get_cd		= legoev3_mmc_get_cd,
	.wires		= 4,
	.max_freq	= 50000000,
	.caps		= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
};

/*
 * EV3 CPU frequency scaling configuration:
 * ========================================
 */

#ifdef CONFIG_CPU_FREQ
static int legoev3_init_cpufreq(void)
{
	return da850_register_cpufreq("pll0_sysclk3");
}
#else
static int legoev3_init_cpufreq(void) { return 0; }
#endif

/*
 * EV3 EEPROM
 * ==========
 * Used get the hardware version and the bluetooth MAC address for the EV3.
 * The MAC address is also used as the serial number.
 */

static void legoev3_eeprom_setup(struct memory_accessor *mem_acc, void *context)
{
	u8 data[8];

	/*
	 * U-boot in the official LEGO firmware does not set ATAGS, so if
	 * system_rev != 0 here, we know we are using a newer version of u-boot
	 * that actually set system_rev for us already. If system_rev is not
	 * set already at this point, then we go ahead and read it (and the
	 * serial/Bluetooth MAC) from the eeprom now.
	 */
	if (system_rev)
		return;

	/*
	 * The first byte read has corrupt data for some reason. It seems to be
	 * a hardware problem with the EEPROM combined with the way the at24
	 * driver forms its i2c messages. So, we are reading a couple of extra
	 * bytes before the actual data. data[2] is the hardware revision and
	 * data[3] is checksum. Hardware rev 3 has the mac address at 0x3f00
	 * instead of hw id.
	 */
	if (mem_acc->read(mem_acc, data, 0x3efe, 8) == 8)
		system_rev = (data[2] ^ data[3]) == 0xff ? data[2] : 3;

	/* revs > 3 have mac address at 0x3f06 */
	if (system_rev == 3 || mem_acc->read(mem_acc, data, 0x3f04, 8) == 8) {
		system_serial_high = be16_to_cpu(*(u16 *)(data + 2));
		system_serial_low = be32_to_cpu(*(u32 *)(data + 4));
	}
}

static struct at24_platform_data legoev3_eeprom_info = {
	.byte_len	= 128 * 1024 / 8,
	.page_size	= 64,
	.flags		= AT24_FLAG_ADDR16 | AT24_FLAG_READONLY | AT24_FLAG_IRUGO,
	.setup		= legoev3_eeprom_setup,
};

/*
 * EV3 I2C board configuration:
 * ============================
 * This is the I2C that communicates with other devices on the main board,
 * not the input ports. (Using I2C0)
 * Devices are:
 * - EEPROM (24c128) see EEPROM section above.
 */

static const short legoev3_i2c_board_pins[] __initconst = {
	EV3_I2C_BOARD_SDA, EV3_I2C_BOARD_SCL,
	-1
};

static struct i2c_board_info __initdata legoev3_i2c_board_devices[] = {
	{
		I2C_BOARD_INFO("24c128", 0x50),
		.platform_data  = &legoev3_eeprom_info,
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
 * PRU_SUART1 is used for Input Port 4.
 * PRU_SUART2 is used for Input Port 3.
 *
 * Pin muxes are defined in the I/O port and bluetooth sections.
 *
 * The actual PRU Soft-UART board configuration is in the following files:
 * drivers/tty/serial/omapl_uart/pru/hal/uart/include/omapl_suart_board.h
 * drivers/tty/serial/omapl_uart/pru/hal/uart/include/suart_api.h
 */

/*
 * EV3 EDMA configuration:
 * =======================
 * NOTE: These comments from from the the evm board file and may not be accurate.
 *
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
	.rsv_chans	= da850_dma0_rsv_chans,
	.rsv_slots	= da850_dma0_rsv_slots,
};

static struct edma_rsv_info da850_edma_cc1_rsv = {
	.rsv_chans	= da850_dma1_rsv_chans,
	.rsv_slots	= da850_dma1_rsv_slots,
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
	EV3_ADC_DATA_IN, EV3_ADC_DATA_OUT, EV3_ADC_CS, EV3_ADC_CLK,
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
	 * actually used. The unused items still need controller data, or we
	 * will get a NULL pointer dereference error when loading.
	 */
	{
		.controller_data	= &legoev3_spi_adc_cfg,
		.chip_select		= 0,
	},
	{
		.controller_data	= &legoev3_spi_adc_cfg,
		.chip_select		= 1,
	},
	{
		.controller_data	= &legoev3_spi_adc_cfg,
		.chip_select		= 2,
	},
	{
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
	EV3_IN2_PIN1, EV3_IN2_PIN2, EV3_IN2_PIN5, EV3_IN2_PIN6, EV3_IN2_BUF_ENA,
	EV3_IN3_PIN1, EV3_IN3_PIN2, EV3_IN3_PIN5, EV3_IN3_PIN6, EV3_IN3_BUF_ENA,
	EV3_IN4_PIN1, EV3_IN4_PIN2, EV3_IN4_PIN5, EV3_IN4_PIN6, EV3_IN4_BUF_ENA,
	EV3_IN1_UART_RXD, EV3_IN2_UART_RXD, EV3_IN3_UART_RXD, EV3_IN4_UART_RXD,
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
			.uart_tty		= "ttyS1",
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
			.uart_tty		= "ttyS0",
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
			.uart_tty		= "ttySU1",
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
			.uart_tty		= "ttySU0",
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
		},
		{
			.id			= EV3_PORT_OUT2,
			.pin1_gpio		= EV3_OUT2_PIN1_PIN,
			.pin2_gpio		= EV3_OUT2_PIN2_PIN,
			.pin5_gpio		= EV3_OUT2_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT2_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT2_PIN6_DIR_PIN,
		},
		{
			.id			= EV3_PORT_OUT3,
			.pin1_gpio		= EV3_OUT3_PIN1_PIN,
			.pin2_gpio		= EV3_OUT3_PIN2_PIN,
			.pin5_gpio		= EV3_OUT3_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT3_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT3_PIN6_DIR_PIN,
		},
		{
			.id			= EV3_PORT_OUT4,
			.pin1_gpio		= EV3_OUT4_PIN1_PIN,
			.pin2_gpio		= EV3_OUT4_PIN2_PIN,
			.pin5_gpio		= EV3_OUT4_PIN5_PIN,
			.pin5_int_gpio		= EV3_OUT4_PIN5_INT_PIN,
			.pin6_dir_gpio		= EV3_OUT4_PIN6_DIR_PIN,
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

static struct snd_legoev3_platform_data ev3_snd_data = {
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
	EV3_SYS_POWER_ENA, EV3_SYS_5V_POWER, EV3_BATT_ADC, EV3_BATT_TYPE,
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
	.batt_adc_gpio	= EV3_BATT_ADC_PIN,
	.batt_type_gpio	= EV3_BATT_TYPE_PIN,
};

static struct platform_device legoev3_battery_device = {
	.name	= "legoev3-battery",
	.id	= -1,
	.dev	= {
		.platform_data	= &ev3_battery_data,
	},
};

/*
 * EV3 PWM configuration:
 * ========================
 * PWM outputs are used by LEDs, sound, bluetooth and motors. This just provides
 * a lookup table so that the respective drivers can find the right pwm devices.
 * The period is always set by the driver, but it is important to set correct
 * polarity here.
 */

static struct pwm_lookup legoev3_pwm_lookup[] = {
	PWM_LOOKUP("pwm-gpio", 0, NULL, "ev3:left:red:ev3dev",    0, PWM_POLARITY_NORMAL),
	PWM_LOOKUP("pwm-gpio", 1, NULL, "ev3:left:green:ev3dev",  0, PWM_POLARITY_NORMAL),
	PWM_LOOKUP("pwm-gpio", 2, NULL, "ev3:right:green:ev3dev", 0, PWM_POLARITY_NORMAL),
	PWM_LOOKUP("pwm-gpio", 3, NULL, "ev3:right:red:ev3dev",   0, PWM_POLARITY_NORMAL),
	PWM_LOOKUP("ecap.0",   0, NULL, "outC",              0, PWM_POLARITY_INVERSED),
	PWM_LOOKUP("ecap.1",   0, NULL, "outD",              0, PWM_POLARITY_INVERSED),
	PWM_LOOKUP("ecap.2",   0, "legoev3-bluetooth", NULL, 0, PWM_POLARITY_INVERSED),
	PWM_LOOKUP("ehrpwm.0", 1, "snd-legoev3",       NULL, 0, PWM_POLARITY_INVERSED),
	PWM_LOOKUP("ehrpwm.1", 0, NULL, "outB",              0, PWM_POLARITY_INVERSED),
	PWM_LOOKUP("ehrpwm.1", 1, NULL, "outA",              0, PWM_POLARITY_INVERSED),
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
	bool stock_lcd;
	u8 ehrpwm_mask = 0;

	/* Disable all internal pullup/pulldown resistors */
	ret = __raw_readl(DA8XX_SYSCFG1_VIRT(DA8XX_PUPD_ENA_REG));
	ret &= ~0xFFFFFFFF;
	__raw_writel(ret, DA8XX_SYSCFG1_VIRT(DA8XX_PUPD_ENA_REG));

	/* Support for GPIOs - must be first! */
	ret = da850_register_gpio();
	if (ret)
		pr_warn("%s: GPIO init failed: %d\n", __func__, ret);

	/* Support for EV3 LCD */
	ret = davinci_cfg_reg_list(legoev3_lcd_pins);
	if (ret)
		pr_warn("legoev3_init: LCD pin mux setup failed:"
			" %d\n", ret);
	/* detect LCD - stock LCD pulls pin high */
	ret = gpio_request_one(EV3_LCD_CS_PIN, GPIOF_INIT_LOW, NULL);
	if (ret)
		pr_warn("legoev3_init: Could not request EV3_LCD_CS_PIN:"
			" %d\n", ret);
	gpio_direction_input(EV3_LCD_CS_PIN);
	stock_lcd = gpio_get_value(EV3_LCD_CS_PIN);
	gpio_free(EV3_LCD_CS_PIN);
	printk("%s: global_cursor_default: %d, default_screen_mode: %d",
		__func__, global_cursor_default, default_screen_mode);
	global_cursor_default = 1;
	if (default_screen_mode == 0 && stock_lcd)
		default_screen_mode = 1;
	if (stock_lcd)
		ret = spi_register_board_info(legoev3_st7586fb_spi1_board_info,
				ARRAY_SIZE(legoev3_st7586fb_spi1_board_info));
	else
		ret = spi_register_board_info(legoev3_st7735r_spi1_board_info,
				ARRAY_SIZE(legoev3_st7735r_spi1_board_info));
	if (ret)
		pr_warn("%s: spi1/frambuffer registration failed: %d\n",
			__func__, ret);
	if (stock_lcd)
		ret = da8xx_register_spi_bus(1,
				ARRAY_SIZE(legoev3_st7586fb_spi1_board_info));
	else
		ret = da8xx_register_spi_bus(1,
				ARRAY_SIZE(legoev3_st7735r_spi1_board_info));
	if (ret)
		pr_warn("%s: SPI 1 registration failed: %d\n", __func__, ret);

	/* Support for EV3 LEDs */
	ret = davinci_cfg_reg_list(legoev3_led_pins);
	if (ret)
		pr_warn("legoev3_init: LED mux setup failed:"
			" %d\n", ret);
	ret = platform_device_register(&ev3_led_pwms_device);
	if (ret)
		pr_warn("legoev3_init: LED gpio-pwm registration failed:"
			" %d\n", ret);
	ret = platform_device_register(&ev3_leds_device);
	if (ret)
		pr_warn("legoev3_init: LED registration failed:"
			" %d\n", ret);

	/* Support for EV3 buttons */
	ret = davinci_cfg_reg_list(legoev3_button_pins);
	if (ret)
		pr_warn("legoev3_init: Button mux setup failed:"
			" %d\n", ret);

	ret = platform_device_register(&ev3_device_gpiokeys);
	if (ret)
	pr_warn("legoev3_init: button registration failed:"
		" %d\n", ret);

	/*
	 * Though bootloader takes care to set emif clock at allowed
	 * possible rate. Kernel needs to reconfigure this rate to
	 * support platforms requiring fixed emif clock rate.
	 */
	ret = da850_set_emif_clk_rate();
	if (ret)
		pr_warn("legoev3_init: Failed to set rate of pll0_sysclk3/emif clock: %d\n",
				ret);

	ret = da850_register_edma(da850_edma_rsv);
	if (ret)
		pr_warn("legoev3_init: edma registration failed: %d\n",
				ret);

	ret = da8xx_register_watchdog();
	if (ret)
		pr_warn("da830_evm_init: watchdog registration failed: %d\n",
				ret);

	/* support for board-level I2C */
	ret = davinci_cfg_reg_list(legoev3_i2c_board_pins);
	if (ret)
		pr_warn("legoev3_init: board i2c mux setup failed: %d\n",
				ret);
	i2c_register_board_info(1, legoev3_i2c_board_devices,
				ARRAY_SIZE(legoev3_i2c_board_devices));
	da8xx_register_i2c(0, &legoev3_i2c_board_pdata);


	/* Analog/Digital converter support */
	ret = davinci_cfg_reg_list(legoev3_adc_pins);
	if (ret)
		pr_warn("legoev3_init: A/D converter mux setup failed:"
			" %d\n", ret);
	ret = spi_register_board_info(legoev3_spi0_board_info,
				      ARRAY_SIZE(legoev3_spi0_board_info));
	if (ret)
		pr_warn("%s: spi0/analog registration failed: %d\n",
			__func__, ret);
	ret = da8xx_register_spi_bus(0, ARRAY_SIZE(legoev3_spi0_board_info));
	if (ret)
		pr_warn("%s: SPI 1 registration failed: %d\n", __func__, ret);

	/* Support for EV3 power */
	ret = davinci_cfg_reg_list(legoev3_power_pins);
	if (ret)
		pr_warn("legoev3_init: power pin mux setup failed:"
			" %d\n", ret);
	pm_power_off = legoev3_power_off;
	/* request the power gpios so that they cannot be accidentally used */
	ret = gpio_request_array(legoev3_sys_power_gpio,
				 ARRAY_SIZE(legoev3_sys_power_gpio));
	if (ret)
		pr_warn("legoev3_init: requesting power pins failed:"
			" %d\n", ret);
	ret = platform_device_register(&legoev3_battery_device);
	if (ret)
		pr_warn("legoev3_init: battery registration failed:"
			" %d\n", ret);

	/* Input/Output port support */
	ret = davinci_cfg_reg_list(legoev3_in_out_pins);
	if (ret)
		pr_warn("legoev3_init: "
			"input port pin mux failed: %d\n", ret);

	ret = platform_device_register(&legoev3_ports_device);
	if (ret)
		pr_warn("legoev3_init: "
			"input/output port registration failed: %d\n", ret);

	ret = platform_device_register(&legoev3_in_port_i2c_fiq);
	if (ret)
		pr_warn("legoev3_init: "
			"FIQ I2C backend registration failed: %d\n", ret);

	/* Sound support */
	ret = davinci_cfg_reg_list(legoev3_sound_pins);
	if (ret)
		pr_warn("legoev3_init: "
			"sound mux setup failed: %d\n", ret);

	ret = platform_device_register(&snd_legoev3);
	if (ret)
		pr_warn("legoev3_init: "
			"sound device registration failed: %d\n", ret);

	/* eHRPWM0B is used to drive the EV3 speaker */
	ehrpwm_mask |= 0x2;

	/* eHRPWM support */
	/* pin mux is set in output port and sound support */
	da850_register_ehrpwm(ehrpwm_mask);

	/* SD card support */
	ret = davinci_cfg_reg_list(legoev3_sd_pins);
	if (ret)
		pr_warn("legoev3_init: mmcsd0 mux setup failed:"
				" %d\n", ret);
	ret = gpio_request(EV3_SD_CD_PIN, "SD Card CD\n");
	if (ret)
		pr_warn("legoev3_init: can not open GPIO %d\n", EV3_SD_CD_PIN);
	gpio_direction_input(EV3_SD_CD_PIN);

	ret = da8xx_register_mmcsd0(&legoev3_sd_config);
	if (ret)
		pr_warn("legoev3_init: mmcsd0 registration failed:"
					" %d\n", ret);

	/* real-time clock support */
	ret = da8xx_register_rtc();
	if (ret)
		pr_warn("legoev3_init: rtc setup failed: %d\n", ret);

	ret = legoev3_init_cpufreq();
	if (ret)
		pr_warn("legoev3_init: cpufreq registration failed: %d\n",
				ret);

	ret = da8xx_register_cpuidle();
	if (ret)
		pr_warn("legoev3_init: cpuidle registration failed: %d\n",
				ret);

	ret = da850_register_pm(&da850_pm_device);
	if (ret)
		pr_warn("legoev3_init: suspend registration failed: %d\n",
				ret);

	/* USB support */
	ret = davinci_cfg_reg_list(legoev3_usb11_pins);
	if (ret)
		pr_warn("legoev3_init: USB mux setup failed: %d\n", ret);

	legoev3_usb_init();

	/* Bluetooth support */
	ret = davinci_cfg_reg_list(legoev3_bt_pins);
	if (ret)
		pr_warn("legoev3_init: bluetooth pin mux setup failed:"
			   " %d\n", ret);
	ret = da850_register_ecap(2);
	if (ret)
		pr_warn("legoev3_init: registering bluetooth clock ecap device failed:"
			   " %d\n", ret);
	ret = platform_device_register(&legoev3_bt_device);
	if (ret)
		pr_warn("legoev3_init: registering on-board bluetooth failed:"
			   " %d\n", ret);

	/* UART support - used by input ports and bluetooth */
	davinci_serial_init(da8xx_serial_device);
	ret = da8xx_register_pru_suart();
	if (ret)
		pr_warn("legoev3_init: pru suart registration failed: %d\n", ret);

	/* ecap and ehrpwm for output port support */
	ret = da850_register_ecap(0);
	if (ret)
		pr_warn("legoev3_init: registering ecap0 failed:"
			   " %d\n", ret);
	ret = da850_register_ecap(1);
	if (ret)
		pr_warn("legoev3_init: registering ecap1 failed:"
			   " %d\n", ret);
	ehrpwm_mask = 0xC;
	da850_register_ehrpwm(ehrpwm_mask);

	pwm_add_table(legoev3_pwm_lookup, ARRAY_SIZE(legoev3_pwm_lookup));
}

static void __init legoev3_map_io(void)
{
	da850_init();
}

/* FIXME: Figure out the right dma_zone_size */
MACHINE_START(DAVINCI_DA850_EVM, "LEGO MINDSTORMS EV3 Programmable Brick")
	.atag_offset	= 0x100,
	.map_io		= legoev3_map_io,
	.init_irq	= cp_intc_init,
	.init_time	= davinci_timer_init,
	.init_machine	= legoev3_init,
	.init_late	= davinci_init_late,
	.dma_zone_size	= SZ_64M,
	.restart	= da8xx_restart,
	.reserve	= da8xx_rproc_reserve_cma,
MACHINE_END
