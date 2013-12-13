/*
 * Chip specific defines for LEGO MINDSTORMS EV3
 *
 * Author: Ralph Hempel
 *
 * Based on changes made by LEGO 
 * 2007, 2009-2010 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_ARCH_DAVINCI_LEGOEV3_H
#define __ASM_ARCH_DAVINCI_LEGOEV3_H

#include <mach/mux.h>

extern void __iomem *da8xx_psc1_base; 		// LEGO BT slow clock
extern void __iomem *da8xx_ecap2_base; 		// LEGO BT slow clock

#define DA8XX_MSTPRI2_REG	0x118

#define DA8XX_PSC1_VIRT(x)	(da8xx_psc1_base + (x))       // LEGO BT Slow clock
#define DA8XX_ECAP2_VIRT(x)	(da8xx_ecap2_base + (x))      // LEGO BT Slow clock

#define DA8XX_LCD_CNTRL_BASE	0x01e13000
#define DA8XX_PLL1_BASE		0x01e1a000
#define DA8XX_MMCSD0_BASE	0x01c40000

#define DA8XX_DDR2_CTL_BASE	0xb0000000

#define DA8XX_USB0_BASE		0x01e00000
#define DA850_SATA_BASE		0x01E18000
#define DA850_SATA_CLK_PWRDN	0x01E2C018
#define DA8XX_ECAP2_BASE        0x01F08000	// LEGO BT Slow clock

#define PINMUX0			0x00
#define PINMUX1			0x04
#define PINMUX2			0x08
#define PINMUX3			0x0c
#define PINMUX4			0x10
#define PINMUX5			0x14
#define PINMUX6			0x18
#define PINMUX7			0x1c
#define PINMUX8			0x20
#define PINMUX9			0x24
#define PINMUX10		0x28
#define PINMUX11		0x2c
#define PINMUX12		0x30
#define PINMUX13		0x34
#define PINMUX14		0x38
#define PINMUX15		0x3c
#define PINMUX16		0x40
#define PINMUX17		0x44
#define PINMUX18		0x48
#define PINMUX19		0x4c

#define PIN(ev3_name, da850_name) \
EV3_##ev3_name		= DA850_##da850_name,

#define GPIO_PIN(ev3_name, group, pin)			\
EV3_##ev3_name		= DA850_GPIO##group##_##pin,	\
EV3_##ev3_name##_PIN	= GPIO_TO_PIN(group, pin),

enum legoev3_pin_defs {
	/* LCD pins */
	PIN(LCD_DATA_IN, SPI1_SOMI)
	GPIO_PIN(LCD_RESET, 5, 0)
	GPIO_PIN(LCD_A0, 2, 11)
	GPIO_PIN(LCD_CS, 2, 12)

	/* LED pins */
	GPIO_PIN(LED_0, 6, 12)
	GPIO_PIN(LED_1, 6, 14)
	GPIO_PIN(LED_2, 6, 13)
	GPIO_PIN(LED_3, 6, 7)

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
	GPIO_PIN(SYS_5V_POWER, 6, 11)

	/* analog/digital converter pins */
	PIN(ADC_DATA_IN, SPI0_SOMI)
	PIN(ADC_DATA_OUT, SPI0_SOMI)
	PIN(ADC_CS, SPI0_SCS_3)
	PIN(ADC_CLK, SPI0_CLK)
	GPIO_PIN(ADC_ENA, 0, 6)

	/* USB1 VBUS pins */
	GPIO_PIN(USB1_DRV, 1, 4)
	GPIO_PIN(USB1_OVC, 6, 3)
};

int da8xx_register_pru_can(void);
int da8xx_register_pru_suart(void);

//void da830_init_spi0(struct spi_board_info *info, unsigned len);
//int da850_init_mcbsp(struct davinci_mcbsp_platform_data *pdata);

extern const short da830_pru_suart_pins[];


extern const short da850_uart0_pins[];
extern const short da850_uart2_pins[];

extern const short da850_cpgmac_pins[];
extern const short da850_rmii_pins[];
extern const short da850_pru_can_pins[];
extern const short da850_pru_suart_pins[];
extern const short da850_mcasp_pins[];

extern const short da850_mmcsd0_pins[];
extern const short da850_nand_pins[];
extern const short da850_nor_pins[];
extern const short da850_spi0_pins[];
extern const short da850_spi1_pins[];
extern const short da850_mcbsp0_pins[];
extern const short da850_mcbsp1_pins[];
extern const short da850_vpif_capture_pins[];
extern const short da850_vpif_display_pins[];
// extern const short da850_evm_usb11_pins[];
extern const short da850_sata_pins[];

extern const short da850_bt_shut_down_pin[];   // LEGO BT
extern const short da850_bt_slow_clock_pin[];  // LEGO BT


#ifdef CONFIG_DAVINCI_MUX
int da8xx_pinmux_setup(const short pins[]);
#else
static inline int da8xx_pinmux_setup(const short pins[]) { return 0; }
#endif

#endif /* __ASM_ARCH_DAVINCI_DA8XX_H */
