/*
 * USB related definitions
 *
 * Copyright (C) 2009 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef __ASM_ARCH_USB_H
#define __ASM_ARCH_USB_H

#define DA8XX_USB0_BASE		0x01e00000
#define DA8XX_USB1_BASE		0x01e25000

#include <linux/interrupt.h>

/* DA8xx CFGCHIP2 (USB 2.0 PHY Control) register bits */
#define CFGCHIP2_PHYCLKGD	(1 << 17)
#define CFGCHIP2_VBUSSENSE	(1 << 16)
#define CFGCHIP2_RESET		(1 << 15)
#define CFGCHIP2_OTGMODE	(3 << 13)
#define CFGCHIP2_NO_OVERRIDE	(0 << 13)
#define CFGCHIP2_FORCE_HOST	(1 << 13)
#define CFGCHIP2_FORCE_DEVICE 	(2 << 13)
#define CFGCHIP2_FORCE_HOST_VBUS_LOW (3 << 13)
#define CFGCHIP2_USB1PHYCLKMUX	(1 << 12)
#define CFGCHIP2_USB2PHYCLKMUX	(1 << 11)
#define CFGCHIP2_PHYPWRDN	(1 << 10)
#define CFGCHIP2_OTGPWRDN	(1 << 9)
#define CFGCHIP2_DATPOL 	(1 << 8)
#define CFGCHIP2_USB1SUSPENDM	(1 << 7)
#define CFGCHIP2_PHY_PLLON	(1 << 6)	/* override PLL suspend */
#define CFGCHIP2_SESENDEN	(1 << 5)	/* Vsess_end comparator */
#define CFGCHIP2_VBDTCTEN	(1 << 4)	/* Vbus comparator */
#define CFGCHIP2_REFFREQ	(0xf << 0)
#define CFGCHIP2_REFFREQ_12MHZ	(1 << 0)
#define CFGCHIP2_REFFREQ_24MHZ	(2 << 0)
#define CFGCHIP2_REFFREQ_48MHZ	(3 << 0)

enum usb_power_n_ovc_method {
	GPIO_BASED = 1,
};

/* DA8xx CPPI4.1 DMA registers */
#define USB_REVISION_REG        0x00
#define USB_CTRL_REG            0x04
#define USB_STAT_REG            0x08
#define USB_EMULATION_REG       0x08
#define USB_MODE_REG            0x10    /* Transparent, CDC, [Generic] RNDIS */
#define USB_AUTOREQ_REG         0x14
#define USB_SRP_FIX_TIME_REG    0x18
#define USB_TEARDOWN_REG        0x1c
#define USB_INTR_SRC_REG        0x20
#define USB_INTR_SRC_SET_REG    0x24
#define USB_INTR_SRC_CLEAR_REG  0x28
#define USB_INTR_MASK_REG       0x2c
#define USB_INTR_MASK_SET_REG   0x30
#define USB_INTR_MASK_CLEAR_REG 0x34
#define USB_INTR_SRC_MASKED_REG 0x38
#define USB_END_OF_INTR_REG     0x3c
#define USB_GENERIC_RNDIS_EP_SIZE_REG(n) (0x50 + (((n) - 1) << 2))

#define USB_TX_MODE_REG         USB_MODE_REG
#define USB_RX_MODE_REG         USB_MODE_REG
/* Control register bits */
#define USB_SOFT_RESET_MASK     1

/* Mode register bits */
#define USB_RX_MODE_SHIFT(n)    (16 + (((n) - 1) << 2))
#define USB_RX_MODE_MASK(n)     (3 << USB_RX_MODE_SHIFT(n))
#define USB_TX_MODE_SHIFT(n)    ((((n) - 1) << 2))
#define USB_TX_MODE_MASK(n)     (3 << USB_TX_MODE_SHIFT(n))
#define USB_TRANSPARENT_MODE    0
#define USB_RNDIS_MODE          1
#define USB_CDC_MODE            2
#define USB_GENERIC_RNDIS_MODE  3

/* AutoReq register bits */
#define USB_RX_AUTOREQ_SHIFT(n) (((n) - 1) << 1)
#define USB_RX_AUTOREQ_MASK(n)  (3 << USB_RX_AUTOREQ_SHIFT(n))
#define USB_NO_AUTOREQ          0
#define USB_AUTOREQ_ALL_BUT_EOP 1
#define USB_AUTOREQ_ALWAYS      3

/* Teardown register bits */
#define USB_TX_TDOWN_SHIFT(n)   (16 + (n))
#define USB_TX_TDOWN_MASK(n)    (1 << USB_TX_TDOWN_SHIFT(n))
#define USB_RX_TDOWN_SHIFT(n)   (n)
#define USB_RX_TDOWN_MASK(n)    (1 << USB_RX_TDOWN_SHIFT(n))

/* USB interrupt register bits */
#define USB_INTR_USB_SHIFT      16
#define USB_INTR_USB_MASK       (0x1ff << USB_INTR_USB_SHIFT) /* 8 Mentor */
				/* interrupts and DRVVBUS interrupt */
#define USB_INTR_DRVVBUS        0x100
#define USB_INTR_RX_SHIFT       8
#define USB_INTR_TX_SHIFT       0

#define USB_MENTOR_CORE_OFFSET  0x400

#define USB_CPPI41_NUM_CH 4
#define USB_CPPI41_QMGR_REG0_ALLOC_SIZE         0x3fff

struct	da8xx_ohci_root_hub;

typedef void (*da8xx_ocic_handler_t)(struct da8xx_ohci_root_hub *hub,
				     unsigned port);
struct gpio_based {
	u32 power_control_pin;
	u32 over_current_indicator;
};

/* Passed as the platform data to the OHCI driver */
struct	da8xx_ohci_root_hub {
	/* Switch the port power on/off */
	int	(*set_power)(unsigned port, struct da8xx_ohci_root_hub *hub,
			int on);
	/* Read the port power status */
	int	(*get_power)(unsigned port, struct da8xx_ohci_root_hub *hub);
	/* Read the port over-current indicator */
	int	(*get_oci)(unsigned port, struct da8xx_ohci_root_hub *hub);
	/* Over-current indicator change notification (pass NULL to disable) */
	int	(*ocic_notify)(da8xx_ocic_handler_t handler,
			struct da8xx_ohci_root_hub *hub);
	/* Time from power on to power good (in 2 ms units) */
	u8	potpgt;

	/* Power control and over current control method */
	unsigned int type;
	union power_n_overcurrent_pins {
		struct gpio_based gpio_method;
		/* Add data pertaining other methods here. For example if its
		 * I2C based.
		 */
	} method;

	/* board specific handler */
	irq_handler_t board_ocic_handler;
};

void davinci_setup_usb(unsigned mA, unsigned potpgt_ms);

#endif	/* ifndef __ASM_ARCH_USB_H */
