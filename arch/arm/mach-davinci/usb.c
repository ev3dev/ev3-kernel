/*
 * USB
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>

#include <linux/usb/musb.h>

#include <mach/common.h>
#include <mach/irqs.h>
#include <mach/cputype.h>
#include <mach/usb.h>
#include <mach/mux.h>

#define DAVINCI_USB_OTG_BASE	0x01c64000

static int da8xx_usb_set_power(unsigned port, struct da8xx_ohci_root_hub *hub,
		int on)
{
	struct gpio_based *gpio = (hub->type == GPIO_BASED) ?
		&hub->method.gpio_method : NULL;

	if (hub->type == GPIO_BASED)
		gpio_set_value(gpio->power_control_pin, on);
	return 0;
}

static int da8xx_usb_get_power(unsigned port, struct da8xx_ohci_root_hub *hub)
{
	struct gpio_based *gpio = (hub->type == GPIO_BASED) ?
		&hub->method.gpio_method : NULL;

	if (hub->type == GPIO_BASED)
		return gpio_get_value(gpio->power_control_pin);
	return 0;
}

static int da8xx_usb_get_oci(unsigned port, struct da8xx_ohci_root_hub *hub)
{
	struct gpio_based *gpio = (hub->type == GPIO_BASED) ?
		&hub->method.gpio_method : NULL;

	if (hub->type == GPIO_BASED)
		return !gpio_get_value(gpio->over_current_indicator);
	return 0;
}

static int da8xx_usb_ocic_notify(da8xx_ocic_handler_t handler,
		struct da8xx_ohci_root_hub *hub)
{
	int irq         = -1;
	int error       = 0;
	struct gpio_based *gpio = (hub->type == GPIO_BASED) ?
		&hub->method.gpio_method : NULL;

	if (hub->type == GPIO_BASED)
		irq = gpio_to_irq(gpio->over_current_indicator);

	if (handler != NULL) {

		error = request_irq(irq, hub->board_ocic_handler,
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING,
					"OHCI over-current indicator", handler);
		if (error)
			pr_err("%s: could not request IRQ to watch "
				"over-current indicator changes\n", __func__);
	} else {
		free_irq(irq, NULL);
	}
	return error;
}


#if defined(CONFIG_USB_MUSB_HDRC) || defined(CONFIG_USB_MUSB_HDRC_MODULE)
static struct musb_hdrc_eps_bits musb_eps[] = {
	{ "ep1_tx", 8, },
	{ "ep1_rx", 8, },
	{ "ep2_tx", 8, },
	{ "ep2_rx", 8, },
	{ "ep3_tx", 5, },
	{ "ep3_rx", 5, },
	{ "ep4_tx", 5, },
	{ "ep4_rx", 5, },
};

static struct musb_hdrc_config musb_config = {
	.multipoint	= true,
	.dyn_fifo	= true,
	.soft_con	= true,
	.dma		= true,

	.num_eps	= 5,
	.dma_channels	= 8,
	.ram_bits	= 10,
	.eps_bits	= musb_eps,
};

static struct musb_hdrc_platform_data usb_data = {
	/* OTG requires a Mini-AB connector */
	.mode           = MUSB_PERIPHERAL,
	.clock		= "usb",
	.config		= &musb_config,
};

static struct resource usb_resources[] = {
	{
		/* physical address */
		.start          = DAVINCI_USB_OTG_BASE,
		.end            = DAVINCI_USB_OTG_BASE + 0x5ff,
		.flags          = IORESOURCE_MEM,
	},
	{
		.start          = IRQ_USBINT,
		.flags          = IORESOURCE_IRQ,
		.name		= "mc"
	},
	{
		/* placeholder for the dedicated CPPI IRQ */
		.flags          = IORESOURCE_IRQ,
		.name		= "dma"
	},
};

static u64 usb_dmamask = DMA_BIT_MASK(32);

static struct platform_device usb_dev = {
	.name           = "musb-davinci",
	.id             = -1,
	.dev = {
		.platform_data		= &usb_data,
		.dma_mask		= &usb_dmamask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
	},
	.resource       = usb_resources,
	.num_resources  = ARRAY_SIZE(usb_resources),
};

void __init davinci_setup_usb(unsigned mA, unsigned potpgt_ms)
{
	usb_data.power = mA > 510 ? 255 : mA / 2;
	usb_data.potpgt = (potpgt_ms + 1) / 2;

	if (cpu_is_davinci_dm646x()) {
		/* Override the defaults as DM6467 uses different IRQs. */
		usb_dev.resource[1].start = IRQ_DM646X_USBINT;
		usb_dev.resource[2].start = IRQ_DM646X_USBDMAINT;
	} else	/* other devices don't have dedicated CPPI IRQ */
		usb_dev.num_resources = 2;

	platform_device_register(&usb_dev);
}

#ifdef CONFIG_ARCH_DAVINCI_DA8XX
static struct resource da8xx_usb20_resources[] = {
	{
		.start		= DA8XX_USB0_BASE,
		.end		= DA8XX_USB0_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= IRQ_DA8XX_USB_INT,
		.flags		= IORESOURCE_IRQ,
		.name		= "mc",
	},
};

int __init da8xx_register_usb20(unsigned mA, unsigned potpgt)
{
	usb_data.clock  = "usb20";
	usb_data.power	= mA > 510 ? 255 : mA / 2;
	usb_data.potpgt = (potpgt + 1) / 2;

	usb_dev.resource = da8xx_usb20_resources;
	usb_dev.num_resources = ARRAY_SIZE(da8xx_usb20_resources);
	usb_dev.name = "musb-da8xx";

	return platform_device_register(&usb_dev);
}
#endif	/* CONFIG_DAVINCI_DA8XX */

#else

void __init davinci_setup_usb(unsigned mA, unsigned potpgt_ms)
{
}

#ifdef CONFIG_ARCH_DAVINCI_DA8XX
int __init da8xx_register_usb20(unsigned mA, unsigned potpgt)
{
	return 0;
}
#endif

#endif  /* CONFIG_USB_MUSB_HDRC */

#ifdef	CONFIG_ARCH_DAVINCI_DA8XX
static struct resource da8xx_usb11_resources[] = {
	[0] = {
		.start	= DA8XX_USB1_BASE,
		.end	= DA8XX_USB1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DA8XX_IRQN,
		.end	= IRQ_DA8XX_IRQN,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 da8xx_usb11_dma_mask = DMA_BIT_MASK(32);

static struct platform_device da8xx_usb11_device = {
	.name		= "ohci",
	.id		= 0,
	.dev = {
		.dma_mask		= &da8xx_usb11_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(da8xx_usb11_resources),
	.resource	= da8xx_usb11_resources,
};

int __init da8xx_register_usb11(struct da8xx_ohci_root_hub *pdata)
{
	da8xx_usb11_device.dev.platform_data = pdata;
	return platform_device_register(&da8xx_usb11_device);
}

void __init da8xx_board_usb_init(const short pins[],
		struct da8xx_ohci_root_hub *usb11_pdata)
{
	int ret;
	struct gpio_based *gpio = (usb11_pdata->type == GPIO_BASED) ?
		&usb11_pdata->method.gpio_method : NULL;

	ret = davinci_cfg_reg_list(pins);
	if (ret) {
		pr_warning("%s: USB 1.1 PinMux setup failed: %d\n",
			   __func__, ret);
		return;
	}

	if (usb11_pdata->type == GPIO_BASED) {

		ret = gpio_request_one(gpio->power_control_pin,
				GPIOF_OUT_INIT_LOW, "ON_BD_USB_DRV");
		if (ret) {
			pr_err("%s: failed to request GPIO for USB 1.1 port "
			       "power control: %d\n", __func__, ret);
			return;
		}

		ret = gpio_request_one(gpio->over_current_indicator, GPIOF_IN,
				"ON_BD_USB_OVC");
		if (ret) {
			pr_err("%s: failed to request GPIO for USB 1.1 port "
			       "over-current indicator: %d\n", __func__, ret);
			goto usb11_setup_oc_fail;
		}
	}

	usb11_pdata->set_power = da8xx_usb_set_power;
	usb11_pdata->get_power = da8xx_usb_get_power;
	usb11_pdata->get_oci = da8xx_usb_get_oci;
	usb11_pdata->ocic_notify = da8xx_usb_ocic_notify;
	/* TPS2087 switch @ 5V */
	usb11_pdata->potpgt = (3 + 1) / 2;

	ret = da8xx_register_usb11(usb11_pdata);
	if (ret) {
		pr_warning("%s: USB 1.1 registration failed: %d\n",
			   __func__, ret);
		goto usb11_setup_fail;
	}

	return;

usb11_setup_fail:
	if (usb11_pdata->type == GPIO_BASED)
		gpio_free(gpio->over_current_indicator);
usb11_setup_oc_fail:
	if (usb11_pdata->type == GPIO_BASED)
		gpio_free(gpio->power_control_pin);
}
#endif	/* CONFIG_DAVINCI_DA8XX */
