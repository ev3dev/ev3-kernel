/*
 * Copyright (C) 2013 Noralf Tronnes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_FBTFT_H
#define __LINUX_FBTFT_H

#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/platform_data/fbtft.h>
#include <linux/platform_device.h>


#define FBTFT_NOP		0x00
#define FBTFT_SWRESET	0x01
#define FBTFT_RDDID		0x04
#define FBTFT_RDDST		0x09
#define FBTFT_CASET		0x2A
#define FBTFT_RASET		0x2B
#define FBTFT_RAMWR		0x2C

#define FBTFT_ONBOARD_BACKLIGHT 2

#define FBTFT_GPIO_NO_MATCH		0xFFFF
#define FBTFT_MAX_INIT_SEQUENCE      512
#define FBTFT_GAMMA_MAX_VALUES_TOTAL 128

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define write_reg(par, ...)                                              \
do {                                                                     \
	par->fbtftops.write_register(par, NUMARGS(__VA_ARGS__), __VA_ARGS__); \
} while (0)

/* fbtft-core.c */
extern void fbtft_dbg_hex(const struct device *dev,
	int groupsize, void *buf, size_t len, const char *fmt, ...);
extern struct fb_info *fbtft_framebuffer_alloc(struct fbtft_display *display,
	struct device *dev);
extern void fbtft_framebuffer_release(struct fb_info *info);
extern int fbtft_register_framebuffer(struct fb_info *fb_info);
extern int fbtft_unregister_framebuffer(struct fb_info *fb_info);
extern void fbtft_register_backlight(struct fbtft_par *par);
extern void fbtft_unregister_backlight(struct fbtft_par *par);
extern int fbtft_init_display(struct fbtft_par *par);
extern int fbtft_probe_common(struct fbtft_display *display,
	struct spi_device *sdev, struct platform_device *pdev);
extern int fbtft_remove_common(struct device *dev, struct fb_info *info);

/* fbtft-io.c */
extern int fbtft_write_spi(struct fbtft_par *par, void *buf, size_t len);
extern int fbtft_write_spi_emulate_9(struct fbtft_par *par,
	void *buf, size_t len);
extern int fbtft_read_spi(struct fbtft_par *par, void *buf, size_t len);
extern int fbtft_write_gpio8_wr(struct fbtft_par *par, void *buf, size_t len);
extern int fbtft_write_gpio16_wr(struct fbtft_par *par, void *buf, size_t len);
extern int fbtft_write_gpio16_wr_latched(struct fbtft_par *par,
	void *buf, size_t len);

/* fbtft-bus.c */
extern int fbtft_write_vmem8_bus8(struct fbtft_par *par, size_t offset, size_t len);
extern int fbtft_write_vmem16_bus16(struct fbtft_par *par, size_t offset, size_t len);
extern int fbtft_write_vmem16_bus8(struct fbtft_par *par, size_t offset, size_t len);
extern int fbtft_write_vmem16_bus9(struct fbtft_par *par, size_t offset, size_t len);
extern void fbtft_write_reg8_bus8(struct fbtft_par *par, int len, ...);
extern void fbtft_write_reg8_bus9(struct fbtft_par *par, int len, ...);
extern void fbtft_write_reg16_bus8(struct fbtft_par *par, int len, ...);
extern void fbtft_write_reg16_bus16(struct fbtft_par *par, int len, ...);


#define FBTFT_REGISTER_DRIVER(_name, _display)                             \
									   \
static int fbtft_driver_probe_spi(struct spi_device *spi)                  \
{                                                                          \
	return fbtft_probe_common(_display, spi, NULL);                    \
}                                                                          \
									   \
static int fbtft_driver_remove_spi(struct spi_device *spi)                 \
{                                                                          \
	struct fb_info *info = spi_get_drvdata(spi);                       \
									   \
	return fbtft_remove_common(&spi->dev, info);                       \
}                                                                          \
									   \
static int fbtft_driver_probe_pdev(struct platform_device *pdev)           \
{                                                                          \
	return fbtft_probe_common(_display, NULL, pdev);                   \
}                                                                          \
									   \
static int fbtft_driver_remove_pdev(struct platform_device *pdev)          \
{                                                                          \
	struct fb_info *info = platform_get_drvdata(pdev);                 \
									   \
	return fbtft_remove_common(&pdev->dev, info);                      \
}                                                                          \
									   \
static struct spi_driver fbtft_driver_spi_driver = {                       \
	.driver = {                                                        \
		.name   = _name,                                           \
		.owner  = THIS_MODULE,                                     \
	},                                                                 \
	.probe  = fbtft_driver_probe_spi,                                  \
	.remove = fbtft_driver_remove_spi,                                 \
};                                                                         \
									   \
static struct platform_driver fbtft_driver_platform_driver = {             \
	.driver = {                                                        \
		.name   = _name,                                           \
		.owner  = THIS_MODULE,                                     \
	},                                                                 \
	.probe  = fbtft_driver_probe_pdev,                                 \
	.remove = fbtft_driver_remove_pdev,                                \
};                                                                         \
									   \
static int __init fbtft_driver_module_init(void)                           \
{                                                                          \
	int ret;                                                           \
									   \
	ret = spi_register_driver(&fbtft_driver_spi_driver);               \
	if (ret < 0)                                                       \
		return ret;                                                \
	return platform_driver_register(&fbtft_driver_platform_driver);    \
}                                                                          \
									   \
static void __exit fbtft_driver_module_exit(void)                          \
{                                                                          \
	spi_unregister_driver(&fbtft_driver_spi_driver);                   \
	platform_driver_unregister(&fbtft_driver_platform_driver);         \
}                                                                          \
									   \
module_init(fbtft_driver_module_init);                                     \
module_exit(fbtft_driver_module_exit);


/* Debug macros */

/* shorthand debug levels */
#define DEBUG_LEVEL_1	DEBUG_REQUEST_GPIOS
#define DEBUG_LEVEL_2	(DEBUG_LEVEL_1 | DEBUG_DRIVER_INIT_FUNCTIONS | DEBUG_TIME_FIRST_UPDATE)
#define DEBUG_LEVEL_3	(DEBUG_LEVEL_2 | DEBUG_RESET | DEBUG_INIT_DISPLAY | DEBUG_BLANK | DEBUG_FREE_GPIOS | DEBUG_VERIFY_GPIOS | DEBUG_BACKLIGHT | DEBUG_SYSFS)
#define DEBUG_LEVEL_4	(DEBUG_LEVEL_2 | DEBUG_FB_READ | DEBUG_FB_WRITE | DEBUG_FB_FILLRECT | DEBUG_FB_COPYAREA | DEBUG_FB_IMAGEBLIT | DEBUG_FB_BLANK)
#define DEBUG_LEVEL_5	(DEBUG_LEVEL_3 | DEBUG_UPDATE_DISPLAY)
#define DEBUG_LEVEL_6	(DEBUG_LEVEL_4 | DEBUG_LEVEL_5)
#define DEBUG_LEVEL_7	0xFFFFFFFF

#define DEBUG_DRIVER_INIT_FUNCTIONS (1<<3)
#define DEBUG_TIME_FIRST_UPDATE     (1<<4)
#define DEBUG_TIME_EACH_UPDATE      (1<<5)
#define DEBUG_DEFERRED_IO           (1<<6)
#define DEBUG_FBTFT_INIT_FUNCTIONS  (1<<7)

/* fbops */
#define DEBUG_FB_READ               (1<<8)
#define DEBUG_FB_WRITE              (1<<9)
#define DEBUG_FB_FILLRECT           (1<<10)
#define DEBUG_FB_COPYAREA           (1<<11)
#define DEBUG_FB_IMAGEBLIT          (1<<12)
#define DEBUG_FB_SETCOLREG          (1<<13)
#define DEBUG_FB_BLANK              (1<<14)

#define DEBUG_SYSFS                 (1<<16)

/* fbtftops */
#define DEBUG_BACKLIGHT             (1<<17)
#define DEBUG_READ                  (1<<18)
#define DEBUG_WRITE                 (1<<19)
#define DEBUG_WRITE_VMEM            (1<<20)
#define DEBUG_WRITE_REGISTER        (1<<21)
#define DEBUG_SET_ADDR_WIN          (1<<22)
#define DEBUG_RESET                 (1<<23)
#define DEBUG_MKDIRTY               (1<<24)
#define DEBUG_UPDATE_DISPLAY        (1<<25)
#define DEBUG_INIT_DISPLAY          (1<<26)
#define DEBUG_BLANK                 (1<<27)
#define DEBUG_REQUEST_GPIOS         (1<<28)
#define DEBUG_FREE_GPIOS            (1<<29)
#define DEBUG_REQUEST_GPIOS_MATCH   (1<<30)
#define DEBUG_VERIFY_GPIOS          (1<<31)


#define fbtft_init_dbg(dev, format, arg...)                  \
do {                                                         \
	if (unlikely((dev)->platform_data &&                 \
	    (((struct fbtft_platform_data *)(dev)->platform_data)->display.debug & DEBUG_DRIVER_INIT_FUNCTIONS))) \
		dev_info(dev, format, ##arg);                \
} while (0)

#define fbtft_par_dbg(level, par, format, arg...)            \
do {                                                         \
	if (unlikely(par->debug & level))                    \
		dev_info(par->info->device, format, ##arg);  \
} while (0)

#define fbtft_dev_dbg(level, par, dev, format, arg...)       \
do {                                                         \
	if (unlikely(par->debug & level))                    \
		dev_info(dev, format, ##arg);                \
} while (0)

#define fbtft_par_dbg_hex(level, par, dev, type, buf, num, format, arg...) \
do {                                                                       \
	if (unlikely(par->debug & level))                                  \
		fbtft_dbg_hex(dev, sizeof(type), buf, num * sizeof(type), format, ##arg); \
} while (0)

#endif /* __LINUX_FBTFT_H */
