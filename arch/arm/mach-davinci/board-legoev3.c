/*
 * LEGO MINDSTORMS EV3 DA850/OMAP-L138
 *
 * Copyright (C) 2013 Ralph Hempel
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

#define DA850_EVM_PHY_ID		"davinci_mdio-0:00"
#define DA850_LCD_PWR_PIN		GPIO_TO_PIN(2, 8)
#define DA850_LCD_BL_PIN		GPIO_TO_PIN(2, 15)

#define DA850_MMCSD_CD_PIN		GPIO_TO_PIN(4, 2) //Lego
#define DA850_MMCSD_WP_PIN		GPIO_TO_PIN(4, 1)

#define DA850_WLAN_EN			GPIO_TO_PIN(6, 9)
#define DA850_WLAN_IRQ			GPIO_TO_PIN(6, 10)

#define DA850_MII_MDIO_CLKEN_PIN	GPIO_TO_PIN(2, 6)

#define DA850_SD_ENABLE_PIN		GPIO_TO_PIN(0, 11)

#define DA850_BT_EN			GPIO_TO_PIN(0, 15)

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#define DAVINCI_BACKLIGHT_MAX_BRIGHTNESS	250
#define DAVINVI_BACKLIGHT_DEFAULT_BRIGHTNESS	250
#define DAVINCI_PWM_PERIOD_NANO_SECONDS		10000000


static struct platform_pwm_backlight_data da850evm_backlight_data = {
	.max_brightness	= DAVINCI_BACKLIGHT_MAX_BRIGHTNESS,
	.dft_brightness	= DAVINVI_BACKLIGHT_DEFAULT_BRIGHTNESS,
	.pwm_period_ns	= DAVINCI_PWM_PERIOD_NANO_SECONDS,
};

static struct platform_device da850evm_backlight = {
	.name		= "pwm-backlight",
	.id		= -1,
};
#endif

static struct mtd_partition da850evm_spiflash_part[] = {
	[0] = {
		.name = "U-Boot",
		.offset = 0,
		.size = SZ_256K,
		.mask_flags = MTD_WRITEABLE,
	},
	[1] = {
		.name = "U-Boot Env",
		.offset = MTDPART_OFS_APPEND,
		.size = SZ_64K,
		.mask_flags = MTD_WRITEABLE,
	},
	[2] = {
		.name = "Kernel",
		.offset = MTDPART_OFS_NXTBLK,
		.size = SZ_2M,
		.mask_flags = 0,
	},
	[3] = {
		.name = "Filesystem",
		.offset = MTDPART_OFS_NXTBLK,
		.size = SZ_8M + SZ_2M + SZ_256K + SZ_128K,
		.mask_flags = 0,
	},
	[4] = {
		.name = "Storage",
		.offset = MTDPART_OFS_NXTBLK,
		.size = SZ_2M + SZ_1M + SZ_256K + SZ_64K,
		.mask_flags = 0,
	},
};

static struct flash_platform_data da850evm_spiflash_data = {
	.name		= "m25p80",
	.parts		= da850evm_spiflash_part,
	.nr_parts	= ARRAY_SIZE(da850evm_spiflash_part),
//	.type		= "m25p64",
	.type           = "s25sl12801",

};

static struct davinci_spi_config da850evm_spiflash_cfg = {
	.io_type	= SPI_IO_TYPE_DMA,
	.c2tdelay	= 8,
	.t2cdelay	= 8,
};

static const struct st7586fb_platform_data lms2012_st7586fb_data = {
	.rst_gpio	= GPIO_TO_PIN(5, 0),
	.a0_gpio	= GPIO_TO_PIN(2, 11),
	.cs_gpio	= GPIO_TO_PIN(2, 12),
};

static struct davinci_spi_config lms2012_st7586fb_cfg = {
        .io_type	= SPI_IO_TYPE_DMA,
        .c2tdelay	= 10,
        .t2cdelay	= 10,
 };

static struct spi_board_info da850evm_spiflash_info[] = {
	[0] = { // SPI0
		.modalias		= "m25p80",
		.platform_data		= &da850evm_spiflash_data,
		.controller_data	= &da850evm_spiflash_cfg,
		.mode			= SPI_MODE_0,
//      	.max_speed_hz		= 30000000,
		.max_speed_hz		= 50000000,
		.bus_num		= 0,
		.chip_select		= 0,
	},
};

static struct spi_board_info da850evm_spifb_info[] = {
	[0] = { // SPI1
//		.modalias = "st7586fb",
		.modalias		= "lms2012_lcd",
		.platform_data		= &lms2012_st7586fb_data,
		.controller_data	= &lms2012_st7586fb_cfg,
		.mode			= SPI_MODE_3 | SPI_NO_CS,
		.max_speed_hz		= 10000000,
 		.bus_num		= 1,
 		.chip_select		= 0,
    },
};

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#define TVP5147_CH0		"tvp514x-0"
#define TVP5147_CH1		"tvp514x-1"

#define VPIF_STATUS		0x002c
#define VPIF_STATUS_CLR		0x0030

#ifdef CONFIG_MTD
static void da850_evm_m25p80_notify_add(struct mtd_info *mtd)
{
	char *mac_addr = davinci_soc_info.emac_pdata->mac_addr;
	size_t retlen;

	if (!strcmp(mtd->name, "MAC-Address")) {
		mtd_read(mtd, 0, ETH_ALEN, &retlen, mac_addr);
		if (retlen == ETH_ALEN)
			pr_info("Read MAC addr from SPI Flash: %pM\n",
				mac_addr);
	}
}

static struct mtd_notifier da850evm_spi_notifier = {
	.add	= da850_evm_m25p80_notify_add,
};

static void da850_evm_setup_mac_addr(void)
{
	register_mtd_user(&da850evm_spi_notifier);
}
#else
static void da850_evm_setup_mac_addr(void) { }
#endif

static struct mtd_partition da850_evm_norflash_partition[] = {
	{
		.name           = "bootloaders + env",
		.offset         = 0,
		.size           = SZ_512K,
		.mask_flags     = MTD_WRITEABLE,
	},
	{
		.name           = "kernel",
		.offset         = MTDPART_OFS_APPEND,
		.size           = SZ_4M,
		.mask_flags     = 0,
	},
	{
		.name           = "filesystem",
		.offset         = MTDPART_OFS_APPEND,
		.size           = MTDPART_SIZ_FULL,
		.mask_flags     = 0,
	},
};

static struct davinci_aemif_timing da850_evm_norflash_timing = {
	.wsetup		= 10,
	.wstrobe	= 60,
	.whold		= 10,
	.rsetup		= 10,
	.rstrobe	= 110,
	.rhold		= 10,
	.ta		= 30,
};

static struct physmap_flash_data da850_evm_norflash_data = {
	.width		= 2,
	.parts		= da850_evm_norflash_partition,
	.nr_parts	= ARRAY_SIZE(da850_evm_norflash_partition),
	.timing		= &da850_evm_norflash_timing,
};

static struct resource da850_evm_norflash_resource[] = {
	{
		.start	= DA8XX_AEMIF_CS2_BASE,
		.end	= DA8XX_AEMIF_CS2_BASE + SZ_32M - 1,
		.flags	= IORESOURCE_MEM,
	},
};
#endif

static struct davinci_pm_config da850_pm_pdata = {
	.sleepcount = 128,
};

static struct platform_device da850_pm_device = {
	.name           = "pm-davinci",
	.dev = {
		.platform_data	= &da850_pm_pdata,
	},
	.id             = -1,
};

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
/* DA850/OMAP-L138 EVM includes a 512 MByte large-page NAND flash
 * (128K blocks). It may be used instead of the (default) SPI flash
 * to boot, using TI's tools to install the secondary boot loader
 * (UBL) and U-Boot.
 */
static struct mtd_partition da850_evm_nandflash_partition[] = {
	{
		.name		= "u-boot env",
		.offset		= 0,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,
	 },
	{
		.name		= "UBL",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,
	},
	{
		.name		= "u-boot",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 4 * SZ_128K,
		.mask_flags	= MTD_WRITEABLE,
	},
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_4M,
		.mask_flags	= 0,
	},
	{
		.name		= "filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	},
};

static struct davinci_aemif_timing da850_evm_nandflash_timing = {
	.wsetup		= 24,
	.wstrobe	= 21,
	.whold		= 14,
	.rsetup		= 19,
	.rstrobe	= 50,
	.rhold		= 0,
	.ta		= 20,
};

static struct davinci_nand_pdata da850_evm_nandflash_data = {
	.parts		= da850_evm_nandflash_partition,
	.nr_parts	= ARRAY_SIZE(da850_evm_nandflash_partition),
	.ecc_mode	= NAND_ECC_HW,
	.ecc_bits	= 4,
	.bbt_options	= NAND_BBT_USE_FLASH,
	.timing		= &da850_evm_nandflash_timing,
};

static struct resource da850_evm_nandflash_resource[] = {
	{
		.start	= DA8XX_AEMIF_CS3_BASE,
		.end	= DA8XX_AEMIF_CS3_BASE + SZ_512K + 2 * SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= DA8XX_AEMIF_CTL_BASE,
		.end	= DA8XX_AEMIF_CTL_BASE + SZ_32K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device da850_evm_devices[] = {
	{
		.name		= "davinci_nand",
		.id		= 1,
		.dev		= {
			.platform_data	= &da850_evm_nandflash_data,
		},
		.num_resources	= ARRAY_SIZE(da850_evm_nandflash_resource),
		.resource	= da850_evm_nandflash_resource,
	},
#if !defined(CONFIG_MMC_DAVINCI) || \
    !defined(CONFIG_MMC_DAVINCI_MODULE)
	{
		.name		= "physmap-flash",
		.id		= 0,
		.dev		= {
			.platform_data  = &da850_evm_norflash_data,
		},
		.num_resources	= 1,
		.resource	= da850_evm_norflash_resource,

	},
#endif
};
static struct davinci_aemif_devices da850_emif_devices = {
	.devices	= da850_evm_devices,
	.num_devices	= ARRAY_SIZE(da850_evm_devices),
};

static struct platform_device davinci_emif_device = {
	.name	= "davinci_aemif",
	.id	= -1,
	.dev	= {
		.platform_data	= &da850_emif_devices,
	},
};

#define DA8XX_AEMIF_CE2CFG_OFFSET	0x10
#define DA8XX_AEMIF_ASIZE_16BIT		0x1

static void __init da850_evm_init_nor(void)
{
	void __iomem *aemif_addr;

	aemif_addr = ioremap(DA8XX_AEMIF_CTL_BASE, SZ_32K);

	/* Configure data bus width of CS2 to 16 bit */
	writel(readl(aemif_addr + DA8XX_AEMIF_CE2CFG_OFFSET) |
		DA8XX_AEMIF_ASIZE_16BIT,
		aemif_addr + DA8XX_AEMIF_CE2CFG_OFFSET);

	iounmap(aemif_addr);
}

static const short da850_evm_nand_pins[] = {
	DA850_EMA_D_0, DA850_EMA_D_1, DA850_EMA_D_2, DA850_EMA_D_3,
	DA850_EMA_D_4, DA850_EMA_D_5, DA850_EMA_D_6, DA850_EMA_D_7,
	DA850_EMA_A_1, DA850_EMA_A_2, DA850_NEMA_CS_3, DA850_NEMA_CS_4,
	DA850_NEMA_WE, DA850_NEMA_OE,
	-1
};

static const short da850_evm_nor_pins[] = {
	DA850_EMA_BA_1, DA850_EMA_CLK, DA850_EMA_WAIT_1, DA850_NEMA_CS_2,
	DA850_NEMA_WE, DA850_NEMA_OE, DA850_EMA_D_0, DA850_EMA_D_1,
	DA850_EMA_D_2, DA850_EMA_D_3, DA850_EMA_D_4, DA850_EMA_D_5,
	DA850_EMA_D_6, DA850_EMA_D_7, DA850_EMA_D_8, DA850_EMA_D_9,
	DA850_EMA_D_10, DA850_EMA_D_11, DA850_EMA_D_12, DA850_EMA_D_13,
	DA850_EMA_D_14, DA850_EMA_D_15, DA850_EMA_A_0, DA850_EMA_A_1,
	DA850_EMA_A_2, DA850_EMA_A_3, DA850_EMA_A_4, DA850_EMA_A_5,
	DA850_EMA_A_6, DA850_EMA_A_7, DA850_EMA_A_8, DA850_EMA_A_9,
	DA850_EMA_A_10, DA850_EMA_A_11, DA850_EMA_A_12, DA850_EMA_A_13,
	DA850_EMA_A_14, DA850_EMA_A_15, DA850_EMA_A_16, DA850_EMA_A_17,
	DA850_EMA_A_18, DA850_EMA_A_19, DA850_EMA_A_20, DA850_EMA_A_21,
	DA850_EMA_A_22, DA850_EMA_A_23,
	-1
};
#endif

#if defined(CONFIG_MMC_DAVINCI) || \
    defined(CONFIG_MMC_DAVINCI_MODULE)
#define HAS_MMC 1
#else
#define HAS_MMC 0
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#if defined(CONFIG_SPI_DAVINCI)
#define HAS_SPI 1
#else
#define HAS_SPI 0
#endif

#if defined(CONFIG_FB_DA8XX)
#define HAS_LCD	1
#else
#define HAS_LCD	0
#endif

#if defined(CONFIG_SND_DA850_SOC_EVM) || \
	defined(CONFIG_SND_DA850_SOC_EVM_MODULE)
#define HAS_MCASP 1
#else
#define HAS_MCASP 0
#endif

#if defined(CONFIG_DAVINCI_EHRPWM) || defined(CONFIG_DAVINCI_EHRPWM_MODULE)
#define HAS_EHRPWM 1
#else
#define HAS_EHRPWM 0
#endif

#if defined(CONFIG_ECAP_PWM) || \
	defined(CONFIG_ECAP_PWM_MODULE)
#define HAS_ECAP_PWM 1
#else
#define HAS_ECAP_PWM 0
#endif

#if defined(CONFIG_BACKLIGHT_PWM) || defined(CONFIG_BACKLIGHT_PWM_MODULE)
#define HAS_BACKLIGHT 1
#else
#define HAS_BACKLIGHT 0
#endif

#if defined(CONFIG_ECAP_CAP) || defined(CONFIG_ECAP_CAP_MODULE)
#define HAS_ECAP_CAP 1
#else
#define HAS_ECAP_CAP 0
#endif

/* have_imager() - Check if we have support for imager interface */
static inline int have_imager(void)
{
#if defined(CONFIG_DA850_UI_CAMERA)
	return 1;
#else
	return 0;
#endif
}

static inline void da850_evm_setup_nor_nand(void)
{
	int ret = 0;

	ret = davinci_cfg_reg_list(da850_evm_nand_pins);
	if (ret)
		pr_warning("da850_evm_init: nand mux setup failed: "
				"%d\n", ret);

	if (!HAS_MMC) {
		ret = davinci_cfg_reg(DA850_GPIO0_11);
		if (ret)
			pr_warning("da850_evm_init:GPIO(0,11) mux setup "
					"failed\n");

		ret = gpio_request(DA850_SD_ENABLE_PIN, "mmc_sd_en");
		if (ret)
			pr_warning("Cannot open GPIO %d\n",
					DA850_SD_ENABLE_PIN);

		/* Driver GP0[11] low for NOR to work */
		gpio_direction_output(DA850_SD_ENABLE_PIN, 0);

		ret = davinci_cfg_reg_list(da850_evm_nor_pins);
		if (ret)
			pr_warning("da850_evm_init: nor mux setup failed: %d\n",
				ret);

		da850_evm_init_nor();
	} else {
		/*
		 * On Logic PD Rev.3 EVMs GP0[11] pin needs to be configured
		 * for MMC and NOR to work. When GP0[11] is low, the SD0
		 * interface will not work, but NOR flash will. When GP0[11]
		 * is high, SD0 will work but NOR flash will not. By default
		 * we are assuming that GP0[11] pin is driven high, when UI
		 * card is not connected. Hence we are not configuring the
		 * GP0[11] pin when MMC/SD is enabled and UI card is not
		 * connected. Not configuring the GPIO pin will enable the
		 * bluetooth to work on AM18x as it requires the GP0[11]
		 * pin for UART flow control.
		 */
		ret = davinci_cfg_reg(DA850_GPIO0_11);
		if (ret)
			pr_warning("da850_evm_init:GPIO(0,11) mux setup "
					"failed\n");

		ret = gpio_request(DA850_SD_ENABLE_PIN, "mmc_sd_en");
		if (ret)
			pr_warning("Cannot open GPIO %d\n",
					DA850_SD_ENABLE_PIN);

		/* Driver GP0[11] high for SD to work */
		gpio_direction_output(DA850_SD_ENABLE_PIN, 1);
	}

	platform_device_register(&davinci_emif_device);
}

#ifdef CONFIG_DA850_UI_RMII
static inline void da850_evm_setup_emac_rmii(int rmii_sel)
{
	struct davinci_soc_info *soc_info = &davinci_soc_info;

	soc_info->emac_pdata->rmii_en = 1;
	gpio_set_value_cansleep(rmii_sel, 0);
}
#else
static inline void da850_evm_setup_emac_rmii(int rmii_sel) { }
#endif


#define DA850_KEYS_DEBOUNCE_MS	10
/*
 * At 200ms polling interval it is possible to miss an
 * event by tapping very lightly on the push button but most
 * pushes do result in an event; longer intervals require the
 * user to hold the button whereas shorter intervals require
 * more CPU time for polling.
 */
#define DA850_GPIO_KEYS_POLL_MS	200

enum da850_evm_ui_exp_pins {
	DA850_EVM_UI_EXP_SEL_C = 5,
	DA850_EVM_UI_EXP_SEL_B,
	DA850_EVM_UI_EXP_SEL_A,
	DA850_EVM_UI_EXP_PB8,
	DA850_EVM_UI_EXP_PB7,
	DA850_EVM_UI_EXP_PB6,
	DA850_EVM_UI_EXP_PB5,
	DA850_EVM_UI_EXP_PB4,
	DA850_EVM_UI_EXP_PB3,
	DA850_EVM_UI_EXP_PB2,
	DA850_EVM_UI_EXP_PB1,
};

static const char const *da850_evm_ui_exp[] = {
	[DA850_EVM_UI_EXP_SEL_C]        = "sel_c",
	[DA850_EVM_UI_EXP_SEL_B]        = "sel_b",
	[DA850_EVM_UI_EXP_SEL_A]        = "sel_a",
	[DA850_EVM_UI_EXP_PB8]          = "pb8",
	[DA850_EVM_UI_EXP_PB7]          = "pb7",
	[DA850_EVM_UI_EXP_PB6]          = "pb6",
	[DA850_EVM_UI_EXP_PB5]          = "pb5",
	[DA850_EVM_UI_EXP_PB4]          = "pb4",
	[DA850_EVM_UI_EXP_PB3]          = "pb3",
	[DA850_EVM_UI_EXP_PB2]          = "pb2",
	[DA850_EVM_UI_EXP_PB1]          = "pb1",
};

#define DA850_N_UI_PB		8

static struct gpio_keys_button da850_evm_ui_keys[] = {
	[0 ... DA850_N_UI_PB - 1] = {
		.type			= EV_KEY,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= DA850_KEYS_DEBOUNCE_MS,
		.code			= -1, /* assigned at runtime */
		.gpio			= -1, /* assigned at runtime */
		.desc			= NULL, /* assigned at runtime */
	},
};

static struct gpio_keys_platform_data da850_evm_ui_keys_pdata = {
	.buttons = da850_evm_ui_keys,
	.nbuttons = ARRAY_SIZE(da850_evm_ui_keys),
	.poll_interval = DA850_GPIO_KEYS_POLL_MS,
};

static struct platform_device da850_evm_ui_keys_device = {
	.name = "gpio-keys-polled",
	.id = 0,
	.dev = {
		.platform_data = &da850_evm_ui_keys_pdata
	},
};

static void da850_evm_ui_keys_init(unsigned gpio)
{
	int i;
	struct gpio_keys_button *button;

	for (i = 0; i < DA850_N_UI_PB; i++) {
		button = &da850_evm_ui_keys[i];
		button->code = KEY_F8 - i;
		button->desc = (char *)
				da850_evm_ui_exp[DA850_EVM_UI_EXP_PB8 + i];
		button->gpio = gpio + DA850_EVM_UI_EXP_PB8 + i;
	}
}

#ifdef CONFIG_DA850_UI_CLCD
static inline void da850_evm_setup_char_lcd(int a, int b, int c)
{
	gpio_set_value_cansleep(a, 0);
	gpio_set_value_cansleep(b, 0);
	gpio_set_value_cansleep(c, 0);
}
#else
static inline void da850_evm_setup_char_lcd(int a, int b, int c) { }
#endif

#ifdef CONFIG_DA850_UI_SD_VIDEO_PORT
static inline void da850_evm_setup_video_port(int video_sel)
{
	gpio_set_value_cansleep(video_sel, 0);
}
#else
static inline void da850_evm_setup_video_port(int video_sel) { }
#endif

#ifdef CONFIG_DA850_UI_CAMERA
static inline void da850_evm_setup_camera(int camera_sel)
{
	gpio_set_value_cansleep(camera_sel, 0);
}
#else
static inline void da850_evm_setup_camera(int camera_sel) { }
#endif

static int da850_evm_ui_expander_setup(struct i2c_client *client, unsigned gpio,
						unsigned ngpio, void *c)
{
	int sel_a, sel_b, sel_c, ret;

	sel_a = gpio + DA850_EVM_UI_EXP_SEL_A;
	sel_b = gpio + DA850_EVM_UI_EXP_SEL_B;
	sel_c = gpio + DA850_EVM_UI_EXP_SEL_C;

	ret = gpio_request(sel_a, da850_evm_ui_exp[DA850_EVM_UI_EXP_SEL_A]);
	if (ret) {
		pr_warning("Cannot open UI expander pin %d\n", sel_a);
		goto exp_setup_sela_fail;
	}

	ret = gpio_request(sel_b, da850_evm_ui_exp[DA850_EVM_UI_EXP_SEL_B]);
	if (ret) {
		pr_warning("Cannot open UI expander pin %d\n", sel_b);
		goto exp_setup_selb_fail;
	}

	ret = gpio_request(sel_c, da850_evm_ui_exp[DA850_EVM_UI_EXP_SEL_C]);
	if (ret) {
		pr_warning("Cannot open UI expander pin %d\n", sel_c);
		goto exp_setup_selc_fail;
	}

	/* deselect all functionalities */
	gpio_direction_output(sel_a, 1);
	gpio_direction_output(sel_b, 1);
	gpio_direction_output(sel_c, 1);

	da850_evm_ui_keys_init(gpio);
	ret = platform_device_register(&da850_evm_ui_keys_device);
	if (ret) {
		pr_warning("Could not register UI GPIO expander push-buttons");
		goto exp_setup_keys_fail;
	}

	pr_info("DA850/OMAP-L138 EVM UI card detected\n");

	da850_evm_setup_nor_nand();

	da850_evm_setup_emac_rmii(sel_a);

	da850_evm_setup_char_lcd(sel_a, sel_b, sel_c);

	da850_evm_setup_video_port(sel_c);

	da850_evm_setup_camera(sel_b);

	return 0;

exp_setup_keys_fail:
	gpio_free(sel_c);
exp_setup_selc_fail:
	gpio_free(sel_b);
exp_setup_selb_fail:
	gpio_free(sel_a);
exp_setup_sela_fail:
	return ret;
}

static int da850_evm_ui_expander_teardown(struct i2c_client *client,
					unsigned gpio, unsigned ngpio, void *c)
{
	platform_device_unregister(&da850_evm_ui_keys_device);

	/* deselect all functionalities */
	gpio_set_value_cansleep(gpio + DA850_EVM_UI_EXP_SEL_C, 1);
	gpio_set_value_cansleep(gpio + DA850_EVM_UI_EXP_SEL_B, 1);
	gpio_set_value_cansleep(gpio + DA850_EVM_UI_EXP_SEL_A, 1);

	gpio_free(gpio + DA850_EVM_UI_EXP_SEL_C);
	gpio_free(gpio + DA850_EVM_UI_EXP_SEL_B);
	gpio_free(gpio + DA850_EVM_UI_EXP_SEL_A);

	return 0;
}

/* assign the baseboard expander's GPIOs after the UI board's */
#define DA850_UI_EXPANDER_N_GPIOS ARRAY_SIZE(da850_evm_ui_exp)
#define DA850_BB_EXPANDER_GPIO_BASE (DAVINCI_N_GPIO + DA850_UI_EXPANDER_N_GPIOS)

enum da850_evm_bb_exp_pins {
	DA850_EVM_BB_EXP_DEEP_SLEEP_EN = 0,
	DA850_EVM_BB_EXP_SW_RST,
	DA850_EVM_BB_EXP_TP_23,
	DA850_EVM_BB_EXP_TP_22,
	DA850_EVM_BB_EXP_TP_21,
	DA850_EVM_BB_EXP_USER_PB1,
	DA850_EVM_BB_EXP_USER_LED2,
	DA850_EVM_BB_EXP_USER_LED1,
	DA850_EVM_BB_EXP_USER_SW1,
	DA850_EVM_BB_EXP_USER_SW2,
	DA850_EVM_BB_EXP_USER_SW3,
	DA850_EVM_BB_EXP_USER_SW4,
	DA850_EVM_BB_EXP_USER_SW5,
	DA850_EVM_BB_EXP_USER_SW6,
	DA850_EVM_BB_EXP_USER_SW7,
	DA850_EVM_BB_EXP_USER_SW8
};

static const char const *da850_evm_bb_exp[] = {
	[DA850_EVM_BB_EXP_DEEP_SLEEP_EN]	= "deep_sleep_en",
	[DA850_EVM_BB_EXP_SW_RST]		= "sw_rst",
	[DA850_EVM_BB_EXP_TP_23]		= "tp_23",
	[DA850_EVM_BB_EXP_TP_22]		= "tp_22",
	[DA850_EVM_BB_EXP_TP_21]		= "tp_21",
	[DA850_EVM_BB_EXP_USER_PB1]		= "user_pb1",
	[DA850_EVM_BB_EXP_USER_LED2]		= "user_led2",
	[DA850_EVM_BB_EXP_USER_LED1]		= "user_led1",
	[DA850_EVM_BB_EXP_USER_SW1]		= "user_sw1",
	[DA850_EVM_BB_EXP_USER_SW2]		= "user_sw2",
	[DA850_EVM_BB_EXP_USER_SW3]		= "user_sw3",
	[DA850_EVM_BB_EXP_USER_SW4]		= "user_sw4",
	[DA850_EVM_BB_EXP_USER_SW5]		= "user_sw5",
	[DA850_EVM_BB_EXP_USER_SW6]		= "user_sw6",
	[DA850_EVM_BB_EXP_USER_SW7]		= "user_sw7",
	[DA850_EVM_BB_EXP_USER_SW8]		= "user_sw8",
};

#define DA850_N_BB_USER_SW	8

static struct gpio_keys_button da850_evm_bb_keys[] = {
	[0] = {
		.type			= EV_KEY,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= DA850_KEYS_DEBOUNCE_MS,
		.code			= KEY_PROG1,
		.desc			= NULL, /* assigned at runtime */
		.gpio			= -1, /* assigned at runtime */
	},
	[1 ... DA850_N_BB_USER_SW] = {
		.type			= EV_SW,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= DA850_KEYS_DEBOUNCE_MS,
		.code			= -1, /* assigned at runtime */
		.desc			= NULL, /* assigned at runtime */
		.gpio			= -1, /* assigned at runtime */
	},
};

static struct gpio_keys_platform_data da850_evm_bb_keys_pdata = {
	.buttons = da850_evm_bb_keys,
	.nbuttons = ARRAY_SIZE(da850_evm_bb_keys),
	.poll_interval = DA850_GPIO_KEYS_POLL_MS,
};

static struct platform_device da850_evm_bb_keys_device = {
	.name = "gpio-keys-polled",
	.id = 1,
	.dev = {
		.platform_data = &da850_evm_bb_keys_pdata
	},
};

static void da850_evm_bb_keys_init(unsigned gpio)
{
	int i;
	struct gpio_keys_button *button;

	button = &da850_evm_bb_keys[0];
	button->desc = (char *)
		da850_evm_bb_exp[DA850_EVM_BB_EXP_USER_PB1];
	button->gpio = gpio + DA850_EVM_BB_EXP_USER_PB1;

	for (i = 0; i < DA850_N_BB_USER_SW; i++) {
		button = &da850_evm_bb_keys[i + 1];
		button->code = SW_LID + i;
		button->desc = (char *)
				da850_evm_bb_exp[DA850_EVM_BB_EXP_USER_SW1 + i];
		button->gpio = gpio + DA850_EVM_BB_EXP_USER_SW1 + i;
	}
}

#define DA850_N_BB_USER_LED	2

static struct gpio_led da850_evm_bb_leds[] = {
	[0 ... DA850_N_BB_USER_LED - 1] = {
		.active_low = 1,
		.gpio = -1, /* assigned at runtime */
		.name = NULL, /* assigned at runtime */
	},
};

static struct gpio_led_platform_data da850_evm_bb_leds_pdata = {
	.leds = da850_evm_bb_leds,
	.num_leds = ARRAY_SIZE(da850_evm_bb_leds),
};

static struct platform_device da850_evm_bb_leds_device = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev = {
		.platform_data = &da850_evm_bb_leds_pdata
	}
};

static void da850_evm_bb_leds_init(unsigned gpio)
{
	int i;
	struct gpio_led *led;

	for (i = 0; i < DA850_N_BB_USER_LED; i++) {
		led = &da850_evm_bb_leds[i];

		led->gpio = gpio + DA850_EVM_BB_EXP_USER_LED2 + i;
		led->name =
			da850_evm_bb_exp[DA850_EVM_BB_EXP_USER_LED2 + i];
	}
}

static int da850_evm_bb_expander_setup(struct i2c_client *client,
						unsigned gpio, unsigned ngpio,
						void *c)
{
	int ret;

	/*
	 * Register the switches and pushbutton on the baseboard as a gpio-keys
	 * device.
	 */
	da850_evm_bb_keys_init(gpio);
	ret = platform_device_register(&da850_evm_bb_keys_device);
	if (ret) {
		pr_warning("Could not register baseboard GPIO expander keys");
		goto io_exp_setup_sw_fail;
	}

	da850_evm_bb_leds_init(gpio);
	ret = platform_device_register(&da850_evm_bb_leds_device);
	if (ret) {
		pr_warning("Could not register baseboard GPIO expander LEDS");
		goto io_exp_setup_leds_fail;
	}

	return 0;

io_exp_setup_leds_fail:
	platform_device_unregister(&da850_evm_bb_keys_device);
io_exp_setup_sw_fail:
	return ret;
}

static int da850_evm_bb_expander_teardown(struct i2c_client *client,
					unsigned gpio, unsigned ngpio, void *c)
{
	platform_device_unregister(&da850_evm_bb_leds_device);
	platform_device_unregister(&da850_evm_bb_keys_device);

	return 0;
}

static struct pca953x_platform_data da850_evm_ui_expander_info = {
	.gpio_base	= DAVINCI_N_GPIO,
	.setup		= da850_evm_ui_expander_setup,
	.teardown	= da850_evm_ui_expander_teardown,
	.names		= da850_evm_ui_exp,
};

static struct pca953x_platform_data da850_evm_bb_expander_info = {
	.gpio_base	= DA850_BB_EXPANDER_GPIO_BASE,
	.setup		= da850_evm_bb_expander_setup,
	.teardown	= da850_evm_bb_expander_teardown,
	.names		= da850_evm_bb_exp,
};
#endif

static struct i2c_board_info __initdata da850_evm_i2c_devices[] = {
#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
	{
		I2C_BOARD_INFO("tlv320aic3x", 0x18),
	},
	{
		I2C_BOARD_INFO("tca6416", 0x20),
		.platform_data = &da850_evm_ui_expander_info,
	},
	{
		I2C_BOARD_INFO("tca6416", 0x21),
		.platform_data = &da850_evm_bb_expander_info,
	},
	{
		I2C_BOARD_INFO("cdce913", 0x65),
	},
	{
		I2C_BOARD_INFO("PCA9543A", 0x73),
	},
#endif
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

/*
 * USB1 VBUS is controlled by GPIO2[4], over-current is reported on GPIO6[13].
 */
#define ON_BD_USB_DRV	GPIO_TO_PIN(2, 4)
#define ON_BD_USB_OVC	GPIO_TO_PIN(6, 13)

static const short da850_evm_usb11_pins[] = {
	DA850_GPIO2_4, DA850_GPIO6_13,
	-1
};

static irqreturn_t da850_evm_usb_ocic_irq(int, void *);

static struct da8xx_ohci_root_hub da850_evm_usb11_pdata = {
	.type			= GPIO_BASED,
	.method	= {
		.gpio_method = {
			.power_control_pin	= ON_BD_USB_DRV,
			.over_current_indicator = ON_BD_USB_OVC,
		},
	},
	.board_ocic_handler	= da850_evm_usb_ocic_irq,
};

static irqreturn_t da850_evm_usb_ocic_irq(int irq, void *handler)
{
	if (handler != NULL)
		((da8xx_ocic_handler_t)handler)(&da850_evm_usb11_pdata, 1);
	return IRQ_HANDLED;
}
#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Fixup the da850_evm_bt_slow_clock_init "
/* Bluetooth Slow clock init using ecap 2 */
static __init void da850_evm_bt_slow_clock_init(void)						// LEGO BT
{												// LEGO BT		
  int PSC1;											// LEGO BT
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

static __init void da850_evm_usb_init(void)
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
	da8xx_board_usb_init(da850_evm_usb11_pins, &da850_evm_usb11_pdata);
}

static const short da850_bt_slow_clock_pins[] __initconst = {
	DA850_ECAP2_OUT, DA850_ECAP2_OUT_ENABLE,
	-1
};

#warning "Are these definitions just the dumb way of doing this?"
#define  DA850_BT_SHUT_DOWN		GPIO_TO_PIN(4, 1)      //LEGO BT
#define  DA850_BT_SHUT_DOWN_EP2		GPIO_TO_PIN(4, 9)      //LEGO BT

static const short da850_bt_shut_down_pins[] __initconst = {
	DA850_GPIO4_1, DA850_GPIO4_9,
	-1
};

static struct davinci_i2c_platform_data lego_i2c_0_pdata = {
	.bus_freq	= 400	/* kHz */,
	.bus_delay	= 0	/* usec */,
};

static struct davinci_uart_config da850_evm_uart_config __initdata = {
	.enabled_uarts = 0x7,
};

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
/* davinci da850 evm audio machine driver */
static u8 da850_iis_serializer_direction[] = {
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	TX_MODE,
	RX_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
};

static struct snd_platform_data da850_evm_snd_data = {
	.tx_dma_offset	= 0x2000,
	.rx_dma_offset	= 0x2000,
	.op_mode	= DAVINCI_MCASP_IIS_MODE,
	.num_serializer	= ARRAY_SIZE(da850_iis_serializer_direction),
	.tdm_slots	= 2,
	.serial_dir	= da850_iis_serializer_direction,
	.asp_chan_q	= EVENTQ_0,
	.version	= MCASP_VERSION_2,
	.txnumevt	= 1,
	.rxnumevt	= 1,
};

static const short da850_evm_mcasp_pins[] __initconst = {
	DA850_AHCLKX, DA850_ACLKX, DA850_AFSX,
	DA850_ACLKR, DA850_AFSR, DA850_AMUTE,
	DA850_AXR_11, DA850_AXR_12,
	-1
};
#endif

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

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Eliminate this code and eliminate this warning after copying this file to board-legoev3.c"
#warning "Now check if anything here is needed in this LCD and Touchpanel Panel code"
#else
static void da850_panel_power_ctrl(int val)
{
	/* lcd power */
	gpio_set_value_cansleep(DA850_LCD_PWR_PIN, val);

	mdelay(200);

	/* lcd backlight */
	gpio_set_value_cansleep(DA850_LCD_BL_PIN, val);
}

static int da850_lcd_hw_init(void)
{
	int status;

	status = gpio_request(DA850_LCD_BL_PIN, "lcd bl\n");
	if (status < 0)
		return status;

	status = gpio_request(DA850_LCD_PWR_PIN, "lcd pwr\n");
	if (status < 0) {
		gpio_free(DA850_LCD_BL_PIN);
		return status;
	}

	gpio_direction_output(DA850_LCD_BL_PIN, 0);
	gpio_direction_output(DA850_LCD_PWR_PIN, 0);

	return 0;
}

/* TPS65070 voltage regulator support */

/* 3.3V */
static struct regulator_consumer_supply tps65070_dcdc1_consumers[] = {
	{
		.supply = "usb0_vdda33",
	},
	{
		.supply = "usb1_vdda33",
	},
};

/* 3.3V or 1.8V */
static struct regulator_consumer_supply tps65070_dcdc2_consumers[] = {
	{
		.supply = "dvdd3318_a",
	},
	{
		.supply = "dvdd3318_b",
	},
	{
		.supply = "dvdd3318_c",
	},
};

/* 1.2V */
static struct regulator_consumer_supply tps65070_dcdc3_consumers[] = {
	{
		.supply = "cvdd",
	},
};

/* 1.8V LDO */
static struct regulator_consumer_supply tps65070_ldo1_consumers[] = {
	{
		.supply = "sata_vddr",
	},
	{
		.supply = "usb0_vdda18",
	},
	{
		.supply = "usb1_vdda18",
	},
	{
		.supply = "ddr_dvdd18",
	},
};

/* 1.2V LDO */
static struct regulator_consumer_supply tps65070_ldo2_consumers[] = {
	{
		.supply = "sata_vdd",
	},
	{
		.supply = "pll0_vdda",
	},
	{
		.supply = "pll1_vdda",
	},
	{
		.supply = "usbs_cvdd",
	},
	{
		.supply = "vddarnwa1",
	},
};

/* We take advantage of the fact that both defdcdc{2,3} are tied high */
static struct tps6507x_reg_platform_data tps6507x_platform_data = {
	.defdcdc_default = true,
};

static struct regulator_init_data tps65070_regulator_data[] = {
	/* dcdc1 */
	{
		.constraints = {
			.min_uV = 3150000,
			.max_uV = 3450000,
			.valid_ops_mask = (REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS),
			.boot_on = 1,
		},
		.num_consumer_supplies = ARRAY_SIZE(tps65070_dcdc1_consumers),
		.consumer_supplies = tps65070_dcdc1_consumers,
	},

	/* dcdc2 */
	{
		.constraints = {
			.min_uV = 1710000,
			.max_uV = 3450000,
			.valid_ops_mask = (REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS),
			.boot_on = 1,
		},
		.num_consumer_supplies = ARRAY_SIZE(tps65070_dcdc2_consumers),
		.consumer_supplies = tps65070_dcdc2_consumers,
		.driver_data = &tps6507x_platform_data,
	},

	/* dcdc3 */
	{
		.constraints = {
			.min_uV = 950000,
			.max_uV = 1350000,
			.valid_ops_mask = (REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS),
			.boot_on = 1,
		},
		.num_consumer_supplies = ARRAY_SIZE(tps65070_dcdc3_consumers),
		.consumer_supplies = tps65070_dcdc3_consumers,
		.driver_data = &tps6507x_platform_data,
	},

	/* ldo1 */
	{
		.constraints = {
			.min_uV = 1710000,
			.max_uV = 1890000,
			.valid_ops_mask = (REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS),
			.boot_on = 1,
		},
		.num_consumer_supplies = ARRAY_SIZE(tps65070_ldo1_consumers),
		.consumer_supplies = tps65070_ldo1_consumers,
	},

	/* ldo2 */
	{
		.constraints = {
			.min_uV = 1140000,
			.max_uV = 1320000,
			.valid_ops_mask = (REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS),
			.boot_on = 1,
		},
		.num_consumer_supplies = ARRAY_SIZE(tps65070_ldo2_consumers),
		.consumer_supplies = tps65070_ldo2_consumers,
	},
};

static struct touchscreen_init_data tps6507x_touchscreen_data = {
	.poll_period =  30,	/* ms between touch samples */
	.min_pressure = 0x30,	/* minimum pressure to trigger touch */
	.vref = 0,		/* turn off vref when not using A/D */
	.vendor = 0,		/* /sys/class/input/input?/id/vendor */
	.product = 65070,	/* /sys/class/input/input?/id/product */
	.version = 0x100,	/* /sys/class/input/input?/id/version */
};

static struct tps6507x_board tps_board = {
	.tps6507x_pmic_init_data = &tps65070_regulator_data[0],
	.tps6507x_ts_init_data = &tps6507x_touchscreen_data,
};

static struct i2c_board_info __initdata da850_evm_tps65070_info[] = {
	{
		I2C_BOARD_INFO("tps6507x", 0x48),
		.platform_data = &tps_board,
	},
};

static int __init pmic_tps65070_init(void)
{
	return i2c_register_board_info(1, da850_evm_tps65070_info,
					ARRAY_SIZE(da850_evm_tps65070_info));
}

static const short da850_evm_lcdc_pins[] = {
	DA850_GPIO2_8, DA850_GPIO2_15,
	-1
};

#endif


#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Eliminate this code and eliminate this warning after copying this file to board-legoev3.c"
#warning "Now check if anything here is needed in this I2C code"
#else
static struct i2c_client *pca9543a;

static int pca9543a_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	pr_info("pca9543a_probe");
	pca9543a = client;
	return 0;
}

static int pca9543a_remove(struct i2c_client *client)
{
	pca9543a = NULL;
	return 0;
}

static const struct i2c_device_id pca9543a_ids[] = {
	{ "PCA9543A", 0, },
	{ /* end of list */ },
};

/* This is for i2c driver for the MT9T031 header i2c switch */
static struct i2c_driver pca9543a_driver = {
	.driver.name	= "PCA9543A",
	.id_table	= pca9543a_ids,
	.probe		= pca9543a_probe,
	.remove		= pca9543a_remove,
};

/**
 * da850_enable_pca9543a() - Enable/Disable I2C switch PCA9543A for sensor
 * @en: enable/disable flag
 */
static int da850_enable_pca9543a(int en)
{
	static char val = 1;
	int status;
	struct i2c_msg msg = {
			.flags = 0,
			.len = 1,
			.buf = &val,
		};

	pr_info("da850evm_enable_pca9543a\n");
	if (!en)
		val = 0;

	if (!pca9543a)
		return -ENXIO;

	msg.addr = pca9543a->addr;
	/* turn i2c switch, pca9543a, on/off */
	status = i2c_transfer(pca9543a->adapter, &msg, 1);
	pr_info("da850evm_enable_pca9543a, status = %d\n", status);
	return status;
}

static const short da850_evm_mii_pins[] = {
	DA850_MII_TXEN, DA850_MII_TXCLK, DA850_MII_COL, DA850_MII_TXD_3,
	DA850_MII_TXD_2, DA850_MII_TXD_1, DA850_MII_TXD_0, DA850_MII_RXER,
	DA850_MII_CRS, DA850_MII_RXCLK, DA850_MII_RXDV, DA850_MII_RXD_3,
	DA850_MII_RXD_2, DA850_MII_RXD_1, DA850_MII_RXD_0, DA850_MDIO_CLK,
	DA850_MDIO_D,
	-1
};

static const short da850_evm_rmii_pins[] = {
	DA850_RMII_TXD_0, DA850_RMII_TXD_1, DA850_RMII_TXEN,
	DA850_RMII_CRS_DV, DA850_RMII_RXD_0, DA850_RMII_RXD_1,
	DA850_RMII_RXER, DA850_RMII_MHZ_50_CLK, DA850_MDIO_CLK,
	DA850_MDIO_D,
	-1
};
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Eliminate this code and eliminate this warning after copying this file to board-legoev3.c"
#warning "Is this needed at all????"
#else
static int __init da850_evm_config_emac(void)
{
	void __iomem *cfg_chip3_base;
	int ret;
	u32 val;
	struct davinci_soc_info *soc_info = &davinci_soc_info;
	u8 rmii_en = soc_info->emac_pdata->rmii_en;

	if (!machine_is_davinci_da850_evm())
		return 0;

	cfg_chip3_base = DA8XX_SYSCFG0_VIRT(DA8XX_CFGCHIP3_REG);

	val = __raw_readl(cfg_chip3_base);

#warning "Lego diables all the code from here..."
	if (rmii_en) {
		val |= BIT(8);
		ret = davinci_cfg_reg_list(da850_evm_rmii_pins);
		pr_info("EMAC: RMII PHY configured, MII PHY will not be"
							" functional\n");
	} else {
		val &= ~BIT(8);
		ret = davinci_cfg_reg_list(da850_evm_mii_pins);
		pr_info("EMAC: MII PHY configured, RMII PHY will not be"
							" functional\n");
	}

	if (ret)
		pr_warning("da850_evm_init: cpgmac/rmii mux setup failed: %d\n",
				ret);

	/* configure the CFGCHIP3 register for RMII or MII */
	__raw_writel(val, cfg_chip3_base);

	ret = davinci_cfg_reg(DA850_GPIO2_6);
	if (ret)
		pr_warning("da850_evm_init:GPIO(2,6) mux setup "
							"failed\n");

	ret = gpio_request(DA850_MII_MDIO_CLKEN_PIN, "mdio_clk_en");
	if (ret) {
		pr_warning("Cannot open GPIO %d\n",
					DA850_MII_MDIO_CLKEN_PIN);
		return ret;
	}

	/* Enable/Disable MII MDIO clock */
	gpio_direction_output(DA850_MII_MDIO_CLKEN_PIN, rmii_en);

	soc_info->emac_pdata->phy_id = DA850_EVM_PHY_ID;

	ret = da8xx_register_emac();
	if (ret)
		pr_warning("da850_evm_init: emac registration failed: %d\n",
				ret);
#warning "...to here"
	return 0;
}
device_initcall(da850_evm_config_emac);

static const struct vpif_input da850_ch2_inputs[] = {
		{
		.input = {
			.index = 0,
			.name = "Camera",
			.type = V4L2_INPUT_TYPE_CAMERA,
			.std = V4L2_STD_BAYER_ALL
		},
		.subdev_name = "mt9t031",
	},
};

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
#endif

#ifdef CONFIG_CPU_FREQ
static __init int da850_evm_init_cpufreq(void)
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
static __init int da850_evm_init_cpufreq(void) { return 0; }
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Eliminate this code and eliminate this warning after copying this file to board-legoev3.c"
#warning "Now check if anything here is needed in this VPIF code"
#else
#if defined(CONFIG_DAVINCI_UART1_AFE)
#define HAS_UART1_AFE 1
#else
#define HAS_UART1_AFE 0
#endif

/* Retaining these APIs, since the VPIF drivers do not check NULL handlers */
static int da850_set_vpif_clock(int mux_mode, int hd)
{
	return 0;
}

static int da850_setup_vpif_input_channel_mode(int mux_mode)
{
	return 0;
}

int da850_vpif_setup_input_path(int ch, const char *name)
{
	int ret = 1;

	 if (!strcmp(name, "mt9t031") && have_imager())
		ret = da850_enable_pca9543a(1);

	return ret;
}

static int da850_vpif_intr_status(void __iomem *vpif_base, int channel)
{
	int status = 0;
	int mask;

	if (channel < 0 || channel > 3)
		return 0;

	mask = 1 << channel;
	status = __raw_readl((vpif_base + VPIF_STATUS)) & mask;
	__raw_writel(status, (vpif_base + VPIF_STATUS_CLR));

	return status;
}

#if defined(CONFIG_DA850_UI_SD_VIDEO_PORT)
/* VPIF capture configuration */
static struct tvp514x_platform_data tvp5146_pdata = {
	.clk_polarity = 0,
	.hs_polarity = 1,
	.vs_polarity = 1
};
#endif

#define TVP514X_STD_ALL (V4L2_STD_NTSC | V4L2_STD_PAL)

static struct vpif_subdev_info da850_vpif_capture_sdev_info[] = {
#if defined(CONFIG_DA850_UI_CAMERA)
	{
		.name	= "mt9t031",
		.board_info = {
			I2C_BOARD_INFO("mt9t031", 0x5d),
			.platform_data = (void *)1,
		},
		.vpif_if = {
			.if_type = VPIF_IF_RAW_BAYER,
			.hd_pol = 0,
			.vd_pol = 0,
			.fid_pol = 0,
		},
	},
#elif defined(CONFIG_DA850_UI_SD_VIDEO_PORT)
	{
		.name	= TVP5147_CH0,
		.board_info = {
			I2C_BOARD_INFO("tvp5146", 0x5d),
			.platform_data = &tvp5146_pdata,
		},
		.input = INPUT_CVBS_VI2B,
		.output = OUTPUT_10BIT_422_EMBEDDED_SYNC,
		.can_route = 1,
		.vpif_if = {
			.if_type = VPIF_IF_BT656,
			.hd_pol = 1,
			.vd_pol = 1,
			.fid_pol = 0,
		},
	},
	{
		.name	= TVP5147_CH1,
		.board_info = {
			I2C_BOARD_INFO("tvp5146", 0x5c),
			.platform_data = &tvp5146_pdata,
		},
		.input = INPUT_SVIDEO_VI2C_VI1C,
		.output = OUTPUT_10BIT_422_EMBEDDED_SYNC,
		.can_route = 1,
		.vpif_if = {
			.if_type = VPIF_IF_BT656,
			.hd_pol = 1,
			.vd_pol = 1,
			.fid_pol = 0,
		},
	},
#endif
};

static const struct vpif_input da850_ch0_inputs[] = {
	{
		.input = {
			.index = 0,
			.name = "Composite",
			.type = V4L2_INPUT_TYPE_CAMERA,
			.std = TVP514X_STD_ALL,
		},
		.subdev_name = TVP5147_CH0,
	},
};

static const struct vpif_input da850_ch1_inputs[] = {
	{
		.input = {
			.index = 0,
			.name = "S-Video",
			.type = V4L2_INPUT_TYPE_CAMERA,
			.std = TVP514X_STD_ALL,
		},
		.subdev_name = TVP5147_CH1,
	},
};

static struct vpif_capture_config da850_vpif_capture_config = {
	.setup_input_channel_mode = da850_setup_vpif_input_channel_mode,
	.setup_input_path = da850_vpif_setup_input_path,
	.intr_status = da850_vpif_intr_status,
	.subdev_info = da850_vpif_capture_sdev_info,
	.subdev_count = ARRAY_SIZE(da850_vpif_capture_sdev_info),
#if defined(CONFIG_DA850_UI_SD_VIDEO_PORT)
	.chan_config[0] = {
		.inputs = da850_ch0_inputs,
		.input_count = ARRAY_SIZE(da850_ch0_inputs),
	},
	.chan_config[1] = {
		.inputs = da850_ch1_inputs,
		.input_count = ARRAY_SIZE(da850_ch1_inputs),
	},
#elif defined(CONFIG_DA850_UI_CAMERA)
	.chan_config[0] = {
		.inputs = da850_ch2_inputs,
		.input_count = ARRAY_SIZE(da850_ch2_inputs),
	},
#endif
	.card_name      = "DA850/OMAP-L138 Video Capture",
};

/* VPIF display configuration */
static struct vpif_subdev_info da850_vpif_subdev[] = {
	{
		.name	= "adv7343",
		.board_info = {
			I2C_BOARD_INFO("adv7343", 0x2a),
		},
	},
};

static const char *vpif_output[] = {
	"Composite",
	"Component",
	"S-Video",
};

static struct vpif_display_config da850_vpif_display_config = {
	.set_clock	= da850_set_vpif_clock,
	.intr_status	= da850_vpif_intr_status,
	.subdevinfo	= da850_vpif_subdev,
	.subdev_count	= ARRAY_SIZE(da850_vpif_subdev),
	.output		= vpif_output,
	.output_count	= ARRAY_SIZE(vpif_output),
	.card_name	= "DA850/OMAP-L138 Video Display",
};

#if defined(CONFIG_VIDEO_DAVINCI_VPIF_DISPLAY) ||\
		defined(CONFIG_VIDEO_DAVINCI_VPIF_DISPLAY_MODULE)
#define HAS_VPIF_DISPLAY 1
#else
#define HAS_VPIF_DISPLAY 0
#endif

#if defined(CONFIG_VIDEO_DAVINCI_VPIF_CAPTURE) ||\
		defined(CONFIG_VIDEO_DAVINCI_VPIF_CAPTURE_MODULE)
#define HAS_VPIF_CAPTURE 1
#else
#define HAS_VPIF_CAPTURE 0
#endif
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

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
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
//        pr_warning("da850_evm_init: da850_pru_suart_pins mux setup failed: %d\n",
//                ret);
//
//    ret = da8xx_register_pru_suart();
//    if (ret)
//        pr_warning("da850_evm_init: pru suart registration failed: %d\n", ret);
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
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
static const short da850_lms2012_lcd_pins[] = {
	DA850_GPIO2_11, DA850_GPIO2_12, DA850_GPIO5_0,
	-1
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

static __init void da850_legoev3_init(void)
{
	int ret;
	char mask = 0;
	struct davinci_soc_info *soc_info = &davinci_soc_info;

	u8 rmii_en = soc_info->emac_pdata->rmii_en;

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
	/* Support for EV3 LCD */
	ret = davinci_cfg_reg_list(da850_lms2012_lcd_pins);
	if (ret)
		pr_warning("da850_evm_init: LMS2012 LCD mux setup failed:"
						" %d\n", ret);
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
        // FIXME: Looks like LEGO does not use this PMIC
	ret = pmic_tps65070_init();
	if (ret)
		pr_warning("da850_evm_init: TPS65070 PMIC init failed: %d\n",
				ret);
#endif

	/*
	 * Though bootloader takes care to set emif clock at allowed
	 * possible rate. Kernel needs to reconfigure this rate to
	 * support platforms requiring fixed emif clock rate.
	 */
	ret = da850_set_emif_clk_rate();
	if (ret)
		pr_warning("da850_evm_init: Failed to set rate of pll0_sysclk3/emif clock: %d\n",
				ret);

	ret = da850_register_edma(da850_edma_rsv);
	if (ret)
		pr_warning("da850_evm_init: edma registration failed: %d\n",
				ret);

	ret = davinci_cfg_reg_list(da850_i2c0_pins);
	if (ret)
		pr_warning("da850_evm_init: i2c0 mux setup failed: %d\n",
				ret);

	platform_device_register(&da850_gpio_i2c);

	ret = da8xx_register_watchdog();
	if (ret)
		pr_warning("da830_evm_init: watchdog registration failed: %d\n",
				ret);

	/* Support for UART 1 */
	ret = davinci_cfg_reg_list(da850_uart1_pins);
	if (ret)
		pr_warning("da850_evm_init: UART 1 mux setup failed:"
						" %d\n", ret);
#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
	/* Support for UART 2 */
	ret = davinci_cfg_reg_list(da850_uart2_pins);
	if (ret)
		pr_warning("da850_evm_init: UART 2 mux setup failed:"
						" %d\n", ret);
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Sort out this bluetooth init code! Needed for EV3"
#else
	pr_info("da850_evm_gpio_req_BT_EN\n");	
	
	ret = gpio_request(DA850_BT_EN, "WL1271_BT_EN");
	if (ret)
		pr_warning("da850_evm_init: can not open BT GPIO %d\n",
					DA850_BT_EN);
	gpio_direction_output(DA850_BT_EN, 1);
	udelay(1000);
	gpio_direction_output(DA850_BT_EN, 0);
#endif

	if (HAS_MMC) {
		ret = davinci_cfg_reg_list(da850_evm_mmcsd0_pins);
		if (ret)
			pr_warning("da850_evm_init: mmcsd0 mux setup failed:"
					" %d\n", ret);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "LEGO commented out operations around CD and WP - why?"
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
 		ret = gpio_request(DA850_MMCSD_CD_PIN, "MMC CD\n");
		if (ret)
			pr_warning("da850_evm_init: can not open GPIO %d\n",
					DA850_MMCSD_CD_PIN);
		gpio_direction_input(DA850_MMCSD_CD_PIN);

		ret = gpio_request(DA850_MMCSD_WP_PIN, "MMC WP\n");
		if (ret)
			pr_warning("da850_evm_init: can not open GPIO %d\n",
					DA850_MMCSD_WP_PIN);
		gpio_direction_input(DA850_MMCSD_WP_PIN);
#endif

		ret = da8xx_register_mmcsd0(&da850_mmc_config);
		if (ret)
			pr_warning("da850_evm_init: mmcsd0 registration failed:"
					" %d\n", ret);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Sort out this bluetooth init code! Needed for EV3???? See WL1271"
#else
		ret = da850_wl12xx_init();
		if (ret)
			pr_warning("da850_evm_init: wl12xx initialization"
				   " failed: %d\n", ret);
#endif
	}

	davinci_serial_init(&da850_evm_uart_config);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
	if (have_imager())
		i2c_add_driver(&pca9543a_driver);
#endif

	i2c_register_board_info(1, da850_evm_i2c_devices,
			ARRAY_SIZE(da850_evm_i2c_devices));

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
	/*
	 * shut down uart 0 and 1; they are not used on the board and
	 * accessing them causes endless "too much work in irq53" messages
	 * with arago fs
	 */
	__raw_writel(0, IO_ADDRESS(DA8XX_UART0_BASE) + 0x30);

	if (HAS_MCASP) {
		if (HAS_UART1_AFE)
			pr_warning("WARNING: both McASP and UART1_AFE are "
				"enabled, but they share pins.\n"
					"\tDisable one of them.\n");

		ret = davinci_cfg_reg_list(da850_evm_mcasp_pins);
		if (ret)
			pr_warning("da850_evm_init: mcasp mux setup failed: %d\n",
					ret);

		da8xx_register_mcasp(0, &da850_evm_snd_data);
	}

	ret = da8xx_register_pruss_uio(&da8xx_pruss_uio_pdata);
	if (ret)
		pr_warning("%s: pruss_uio initialization failed: %d\n",
				__func__, ret);	
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Is da850_lcdcntl_pins needed?"
	ret = davinci_cfg_reg_list(da850_lcdcntl_pins);
	if (ret)
		pr_warning("da850_evm_init: lcdcntl mux setup failed: %d\n",
				ret);

#warning "Is da850_evm_lcdc_pins needed?"
	/* Handle board specific muxing for LCD here */
	ret = davinci_cfg_reg_list(da850_evm_lcdc_pins);
	if (ret)
		pr_warning("da850_evm_init: evm specific lcd mux setup "
				"failed: %d\n",	ret);

#warning "Is da850_lcd_hw_init needed?"
	ret = da850_lcd_hw_init();
	if (ret)
		pr_warning("da850_evm_init: lcd initialization failed: %d\n",
				ret);

#warning "Is da8xx_register_lcdc needed?"
	sharp_lk043t1dg01_pdata.panel_power_ctrl = da850_panel_power_ctrl,
	ret = da8xx_register_lcdc(&sharp_lk043t1dg01_pdata);
	if (ret)
		pr_warning("da850_evm_init: lcdc registration failed: %d\n",
				ret);
#endif

#warning "Is da8xx_register_rtc needed?"
	ret = da8xx_register_rtc();
	if (ret)
		pr_warning("da850_evm_init: rtc setup failed: %d\n", ret);

#warning "Is da850_evm_init_cpufreq needed?"
	ret = da850_evm_init_cpufreq();
	if (ret)
		pr_warning("da850_evm_init: cpufreq registration failed: %d\n",
				ret);

#warning "Is da8xx_register_cpuidle needed?"
	ret = da8xx_register_cpuidle();
	if (ret)
		pr_warning("da850_evm_init: cpuidle registration failed: %d\n",
				ret);

#warning "Is da850_pm_device needed?"
	ret = da850_register_pm(&da850_pm_device);
	if (ret)
		pr_warning("da850_evm_init: suspend registration failed: %d\n",
				ret);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
	if (HAS_VPIF_DISPLAY || HAS_VPIF_CAPTURE) {
		ret = da850_register_vpif();
		if (ret)
			pr_warning("da850_evm_init: VPIF setup failed: %d\n",
				   ret);
	}

	if (HAS_VPIF_CAPTURE) {
		ret = davinci_cfg_reg_list(da850_vpif_capture_pins);
		if (ret)
			pr_warning("da850_evm_init: VPIF capture mux failed:"
					"%d\n", ret);

		ret = da850_register_vpif_capture(&da850_vpif_capture_config);
		if (ret)
			pr_warning("da850_evm_init: VPIF capture setup failed:"
					"%d\n", ret);
	}

	if (HAS_VPIF_DISPLAY) {
		ret = davinci_cfg_reg_list(da850_vpif_display_pins);
		if (ret)
			pr_warning("da850_evm_init : VPIF capture mux failed :"
					"%d\n", ret);

		ret = da850_register_vpif_display(&da850_vpif_display_config);
		if (ret)
			pr_warning("da850_evm_init: VPIF display setup failed:"
					"%d\n", ret);
	}
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
#else
#endif
	ret = da8xx_register_spi(0, da850evm_spiflash_info,
				 ARRAY_SIZE(da850evm_spiflash_info));
	if (ret)
		pr_warning("da850_evm_init: spi 0 registration failed: %d\n",
				ret);
	ret = da8xx_register_spi(1, da850evm_spifb_info,
				 ARRAY_SIZE(da850evm_spifb_info));
	if (ret)
		pr_warning("da850_evm_init: spi 1 registration failed: %d\n",
				ret);

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
#else
	ret = da850_register_sata(DA850EVM_SATA_REFCLKPN_RATE);
	if (ret)
		pr_warning("da850_evm_init: sata registration failed: %d\n",
				ret);

	da850_evm_setup_mac_addr();
#endif

	da850_evm_usb_init();


#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#else
#warning "Delete this code and eliminate this warning after copying this file to board-legoev3.c"
	if (HAS_EHRPWM) {
		if (rmii_en) {
			ret = davinci_cfg_reg_list(da850_ehrpwm0_pins);
			if (ret)
				pr_warning("da850_evm_init:"
				" ehrpwm0 mux setup failed: %d\n", ret);
			else
				mask = BIT(0) | BIT(1);
		} else {
			pr_warning("da850_evm_init:"
			" eHRPWM module 0 cannot be used"
			" since it is being used by MII interface\n");
			mask = 0;
		}

		if (!HAS_LCD) {
			ret = davinci_cfg_reg_list(da850_ehrpwm1_pins);
			if (ret)
				pr_warning("da850_evm_init:"
				" eHRPWM module1 output A mux"
				" setup failed %d\n", ret);
			else
				mask = mask | BIT(2);
		} else {
			pr_warning("da850_evm_init:"
				" eHRPWM module1 outputA cannot be"
				" used since it is being used by LCD\n");
		}

		if (!HAS_SPI) {
			ret = davinci_cfg_reg(DA850_EHRPWM1_B);
			if (ret)
				pr_warning("da850_evm_init:"
					" eHRPWM module1 outputB mux"
					" setup failed %d\n", ret);
		else
			mask =  mask  | BIT(3);
		} else {
			pr_warning("da850_evm_init:"
				" eHRPWM module1 outputB cannot be"
				" used since it is being used by spi1\n");
		}

		da850_register_ehrpwm(mask);
	}

	if (HAS_ECAP_PWM) {
		ret = davinci_cfg_reg(DA850_ECAP2_APWM2);
		if (ret)
			pr_warning("da850_evm_init:ecap mux failed:"
					" %d\n", ret);
		ret = da850_register_ecap(2);
		if (ret)
			pr_warning("da850_evm_init:"
				" eCAP registration failed: %d\n", ret);
	}

	if (HAS_BACKLIGHT) {
		ret = da850_register_backlight(&da850evm_backlight,
				&da850evm_backlight_data);
		if (ret)
			pr_warning("da850_evm_init:"
				" backlight device registration"
				" failed: %d\n", ret);
	}

	if (HAS_ECAP_CAP) {
		if (HAS_MCASP)
			pr_warning("da850_evm_init:"
				"ecap module 1 cannot be used "
				"since it shares pins with McASP\n");
		else {
			ret = davinci_cfg_reg(DA850_ECAP1_APWM1);
			if (ret)
				pr_warning("da850_evm_init:ecap mux failed:%d\n"
						, ret);
			else {
				ret = da850_register_ecap_cap(1);
				if (ret)
					pr_warning("da850_evm_init"
					"eCAP registration failed: %d\n", ret);
			}
		}
	}
#endif

#ifdef CONFIG_MACH_DAVINCI_LEGOEV3
#warning "Keep this code and eliminate this warning after copying this file to board-legoev3.c"
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
//		pr_warning("da850_evm_init: BT shut down mux setup failed:"	// LEGO BT
//						" %d\n", ret);			// LEGO BT

	gpio_set_value(DA850_BT_SHUT_DOWN, 0);					// LEGO BT
	gpio_set_value(DA850_BT_SHUT_DOWN_EP2, 0);				// LEGO BT - EP2

//       /* Support for Bluetooth slow clock */					// LEGO BT
//	ret = davinci_cfg_reg_list(da850_bt_slow_clock_pins);			// LEGO BT
//	if (ret)								// LEGO BT
//		pr_warning("da850_evm_init: BT slow clock mux setup failed:"	// LEGO BT
//						" %d\n", ret);			// LEGO BT

        da850_evm_bt_slow_clock_init();						// LEGO BT
        gpio_direction_input(DA850_ECAP2_OUT_ENABLE);				// LEGO BT

	gpio_set_value(DA850_BT_SHUT_DOWN, 1);					// LEGO BT
	gpio_set_value(DA850_BT_SHUT_DOWN_EP2, 1);				// LEGO BT - EP2
#endif
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static int __init da850_evm_console_init(void)
{
//	if (!machine_is_davinci_da850_evm())
//		return 0;
//
//	return add_preferred_console("ttyS", 2, "115200");
	return add_preferred_console("ttyS", 1, "115200"); //Nico
}
console_initcall(da850_evm_console_init);
#endif

static __init void da850_evm_irq_init(void)
{
	struct davinci_soc_info *soc_info = &davinci_soc_info;

	cp_intc_init();
}

static void __init da850_evm_map_io(void)
{
	da850_init();
}

#warning "Make sure DA850_LEGOEV3 gets added to the ARM Build Database!!!"
// FIXME: Figure out the right dma_zone_size
//MACHINE_START(DA850_LEGOEV3, "LEGO MINDSTORMS EV3 Programmable Brick")
MACHINE_START(DAVINCI_DA850_EVM, "DaVinci DA850/OMAP-L138/AM18x EVM")
	.atag_offset	= 0x100,
	.map_io		= da850_evm_map_io,
	.init_irq	= cp_intc_init,
	.timer		= &davinci_timer,
	.init_machine	= da850_legoev3_init,
	.dma_zone_size	= SZ_64M,
	.restart	= da8xx_restart,
MACHINE_END
