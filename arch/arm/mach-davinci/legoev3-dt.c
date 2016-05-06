/*
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Modified from mach-omap/omap2/board-generic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/irqdomain.h>
#include <linux/platform_data/legoev3.h>
#include <linux/platform_data/legoev3_i2c.h>

#include <asm/mach/arch.h>

#include <mach/common.h>
#include <mach/da8xx.h>
#include <mach/legoev3-fiq.h>

#include <sound/legoev3.h>

#include "board-legoev3.h"
#include "cp_intc.h"

static struct of_dev_auxdata da850_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("ti,davinci-i2c", 0x01c22000, "i2c_davinci.1", NULL),
	OF_DEV_AUXDATA("ti,davinci-wdt", 0x01c21000, "davinci-wdt", NULL),
	OF_DEV_AUXDATA("ti,da830-mmc", 0x01c40000, "da830-mmc.0", NULL),
	OF_DEV_AUXDATA("ti,da850-ehrpwm", 0x01f00000, "ehrpwm.0", NULL),
	OF_DEV_AUXDATA("ti,da850-ehrpwm", 0x01f02000, "ehrpwm.1", NULL),
	OF_DEV_AUXDATA("ti,da850-ecap", 0x01f06000, "ecap.0", NULL),
	OF_DEV_AUXDATA("ti,da850-ecap", 0x01f07000, "ecap.1", NULL),
	OF_DEV_AUXDATA("ti,da850-ecap", 0x01f08000, "ecap.2", NULL),
	OF_DEV_AUXDATA("ti,da830-spi", 0x01c41000, "spi_davinci.0", NULL),
	OF_DEV_AUXDATA("ti,da830-spi", 0x01f0e000, "spi_davinci.1", NULL),
	OF_DEV_AUXDATA("ns16550a", 0x01c42000, "serial8250.0", NULL),
	OF_DEV_AUXDATA("ns16550a", 0x01d0c000, "serial8250.1", NULL),
	OF_DEV_AUXDATA("ns16550a", 0x01d0d000, "serial8250.2", NULL),
	OF_DEV_AUXDATA("ti,da850-aemif", 0x68000000, "ti-aemif", NULL),
	OF_DEV_AUXDATA("ti,da850-tilcdc", 0x01e13000, "da8xx_lcdc.0", NULL),
	OF_DEV_AUXDATA("ti,da830-ohci", 0x01e25000, "ohci-da8xx", NULL),
	OF_DEV_AUXDATA("ti,da830-musb", 0x01e00000, "musb-da8xx", NULL),
	OF_DEV_AUXDATA("ti,da830-usb-phy", 0x01c1417c, "da8xx-usb-phy", NULL),
	{}
};

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

static struct i2c_legoev3_platform_data legoev3_i2c3_data = {
	.sda_pin	= EV3_IN1_PIN5_PIN,
	.scl_pin	= EV3_IN1_I2C_CLK_PIN,
	.port_id	= EV3_PORT_IN1,
};

static struct platform_device legoev3_i2c3_device = {
	.name	= "i2c-legoev3",
	.dev	= {
		.platform_data	= &legoev3_i2c3_data,
	},
	.id	= 3,
};

static struct i2c_legoev3_platform_data legoev3_i2c4_data = {
	.sda_pin	= EV3_IN2_PIN5_PIN,
	.scl_pin	= EV3_IN2_I2C_CLK_PIN,
	.port_id	= EV3_PORT_IN2,
};

static struct platform_device legoev3_i2c4_device = {
	.name	= "i2c-legoev3",
	.dev	= {
		.platform_data	= &legoev3_i2c4_data,
	},
	.id	= 4,
};

static struct i2c_legoev3_platform_data legoev3_i2c5_data = {
	.sda_pin	= EV3_IN3_PIN5_PIN,
	.scl_pin	= EV3_IN3_I2C_CLK_PIN,
	.port_id	= EV3_PORT_IN3,
};

static struct platform_device legoev3_i2c5_device = {
	.name	= "i2c-legoev3",
	.dev	= {
		.platform_data	= &legoev3_i2c5_data,
	},
	.id	= 5,
};

static struct i2c_legoev3_platform_data legoev3_i2c6_data = {
	.sda_pin	= EV3_IN4_PIN5_PIN,
	.scl_pin	= EV3_IN4_I2C_CLK_PIN,
	.port_id	= EV3_PORT_IN4,
};

static struct platform_device legoev3_i2c6_device = {
	.name	= "i2c-legoev3",
	.dev	= {
		.platform_data	= &legoev3_i2c6_data,
	},
	.id	= 6,
};

static struct legoev3_bluetooth_platform_data legoev3_bt_pdata = {
	.pic_ena_gpio		= EV3_PIC_ENA_PIN,
	.pic_rst_gpio		= EV3_PIC_RST_PIN,
	.pic_cts_gpio		= EV3_PIC_CTS_PIN,
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

static struct pwm_lookup legoev3_pwm_lookup[] = {
	PWM_LOOKUP("ecap.2",   0, "legoev3-bluetooth", NULL, 0, PWM_POLARITY_INVERSED),
	PWM_LOOKUP("ehrpwm.0", 1, "snd-legoev3",       NULL, 0, PWM_POLARITY_INVERSED),
};

static void __init legoev3_init_machine(void)
{
	int ret;

	ret = da8xx_register_usb20_phy_clk(false);
	if (ret)
		pr_warn("%s: registering USB 2.0 PHY clock failed: %d",
			__func__, ret);
	ret = da8xx_register_usb11_phy_clk(false);
	if (ret)
		pr_warn("%s: registering USB 1.1 PHY clock failed: %d",
			__func__, ret);

	of_platform_default_populate(NULL, da850_auxdata_lookup, NULL);
	davinci_pm_init();

	pwm_add_table(legoev3_pwm_lookup, ARRAY_SIZE(legoev3_pwm_lookup));

	ret = platform_device_register(&legoev3_in_port_i2c_fiq);
	if (ret)
		pr_warn("%s: FIQ I2C backend registration failed: %d\n", 
			__func__, ret);

	ret = platform_device_register(&legoev3_i2c3_device);
	if (ret)
		pr_warn("%s: Input port 1 I2C registration failed: %d\n", 
			__func__, ret);

	ret = platform_device_register(&legoev3_i2c4_device);
	if (ret)
		pr_warn("%s: Input port 2 I2C registration failed: %d\n", 
			__func__, ret);

	ret = platform_device_register(&legoev3_i2c5_device);
	if (ret)
		pr_warn("%s: Input port 3 I2C registration failed: %d\n", 
			__func__, ret);

	ret = platform_device_register(&legoev3_i2c6_device);
	if (ret)
		pr_warn("%s: Input port 4 I2C registration failed: %d\n", 
			__func__, ret);

	ret = da8xx_register_pru_suart();
	if (ret)
		pr_warn("%s: pru suart registration failed: %d\n",
			__func__, ret);

	ret = platform_device_register(&legoev3_bt_device);
	if (ret)
		pr_warn("%s: registering on-board bluetooth failed: %d\n",
			__func__, ret);

	ret = platform_device_register(&snd_legoev3);
	if (ret)
		pr_warn("%s: sound device registration failed: %d\n",
			__func__, ret);
}

static const char *const legoev3_dt_compat[] __initconst = {
	"lego,ev3",
	NULL,
};

DT_MACHINE_START(DA850_DT, "LEGO MINDSTORMS EV3")
	.map_io		= da850_init,
	.init_time	= davinci_timer_init,
	.init_machine	= legoev3_init_machine,
	.dt_compat	= legoev3_dt_compat,
	.init_late	= davinci_init_late,
	.restart	= da8xx_restart,
MACHINE_END
