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

#ifndef __LINUX_PLATFORM_DATA_FBTFT_H
#define __LINUX_PLATFORM_DATA_FBTFT_H

#define FBTFT_GPIO_NAME_SIZE	32

/**
 * struct fbtft_gpio - Structure that holds one pinname to gpio mapping
 * @name: pinname (reset, dc, etc.)
 * @gpio: GPIO number
 *
 */
struct fbtft_gpio {
	char name[FBTFT_GPIO_NAME_SIZE];
	unsigned gpio;
};

struct fbtft_par;

/**
 * struct fbtft_ops - FBTFT operations structure
 * @write: Writes to interface bus
 * @read: Reads from interface bus
 * @write_vmem: Writes video memory to display
 * @write_reg: Writes to controller register
 * @set_addr_win: Set the GRAM update window
 * @reset: Reset the LCD controller
 * @mkdirty: Marks display lines for update
 * @update_display: Updates the display
 * @init_display: Initializes the display
 * @blank: Blank the display (optional)
 * @request_gpios_match: Do pinname to gpio matching
 * @request_gpios: Request gpios from the kernel
 * @free_gpios: Free previously requested gpios
 * @verify_gpios: Verify that necessary gpios is present (optional)
 * @register_backlight: Used to register backlight device (optional)
 * @unregister_backlight: Unregister backlight device (optional)
 * @set_var: Configure LCD with values from variables like @rotate and @bgr
 *           (optional)
 * @set_gamma: Set Gamma curve (optional)
 *
 * Most of these operations have default functions assigned to them in
 *     fbtft_framebuffer_alloc()
 */
struct fbtft_ops {
	int (*write)(struct fbtft_par *par, void *buf, size_t len);
	int (*read)(struct fbtft_par *par, void *buf, size_t len);
	int (*write_vmem)(struct fbtft_par *par, size_t offset, size_t len);
	void (*write_register)(struct fbtft_par *par, int len, ...);

	void (*set_addr_win)(struct fbtft_par *par,
		int xs, int ys, int xe, int ye);
	void (*reset)(struct fbtft_par *par);
	void (*mkdirty)(struct fb_info *info, int from, int to);
	void (*update_display)(struct fbtft_par *par,
				unsigned start_line, unsigned end_line);
	int (*init_display)(struct fbtft_par *par);
	int (*blank)(struct fbtft_par *par, bool on);

	unsigned long (*request_gpios_match)(struct fbtft_par *par,
		const struct fbtft_gpio *gpio);
	int (*request_gpios)(struct fbtft_par *par);
	void (*free_gpios)(struct fbtft_par *par);
	int (*verify_gpios)(struct fbtft_par *par);

	void (*register_backlight)(struct fbtft_par *par);
	void (*unregister_backlight)(struct fbtft_par *par);

	int (*set_var)(struct fbtft_par *par);
	int (*set_gamma)(struct fbtft_par *par, unsigned long *curves);
};

/**
 * struct fbtft_display - Describes the display properties
 * @width: Width of display in pixels
 * @height: Height of display in pixels
 * @regwidth: LCD Controller Register width in bits
 * @buswidth: Display interface bus width in bits
 * @backlight: Backlight type.
 * @fbtftops: FBTFT operations provided by driver or device (platform_data)
 * @bpp: Bits per pixel
 * @fps: Frames per second
 * @txbuflen: Size of transmit buffer
 * @init_sequence: Pointer to LCD initialization array
 * @gamma: String representation of Gamma curve(s)
 * @gamma_num: Number of Gamma curves
 * @gamma_len: Number of values per Gamma curve
 * @debug: Initial debug value
 *
 * This structure is not stored by FBTFT except for init_sequence.
 */
struct fbtft_display {
	unsigned width;
	unsigned height;
	unsigned regwidth;
	unsigned buswidth;
	unsigned backlight;
	struct fbtft_ops fbtftops;
	unsigned bpp;
	unsigned fps;
	int txbuflen;
	int *init_sequence;
	char *gamma;
	int gamma_num;
	int gamma_len;
	unsigned long debug;
};

/**
 * struct fbtft_platform_data - Passes display specific data to the driver
 * @display: Display properties
 * @gpios: Pointer to an array of piname to gpio mappings
 * @rotate: Display rotation angle
 * @bgr: LCD Controller BGR bit
 * @fps: Frames per second (this will go away, use @fps in @fbtft_display)
 * @txbuflen: Size of transmit buffer
 * @startbyte: When set, enables use of Startbyte in transfers
 * @gamma: String representation of Gamma curve(s)
 * @extra: A way to pass extra info
 */
struct fbtft_platform_data {
	struct fbtft_display display;
	const struct fbtft_gpio *gpios;
	unsigned rotate;
	bool bgr;
	unsigned fps;
	int txbuflen;
	u8 startbyte;
	char *gamma;
	void *extra;
};

/**
 * struct fbtft_par - Main FBTFT data structure
 *
 * This structure holds all relevant data to operate the display
 *
 * See sourcefile for documentation since nested structs is not
 * supported by kernel-doc.
 *
 */
/* @spi: Set if it is a SPI device
 * @pdev: Set if it is a platform device
 * @info: Pointer to framebuffer fb_info structure
 * @pdata: Pointer to platform data
 * @ssbuf: Not used
 * @pseudo_palette: Used by fb_set_colreg()
 * @txbuf.buf: Transmit buffer
 * @txbuf.len: Transmit buffer length
 * @buf: Small buffer used when writing init data over SPI
 * @startbyte: Used by some controllers when in SPI mode.
 *             Format: 6 bit Device id + RS bit + RW bit
 * @fbtftops: FBTFT operations provided by driver or device (platform_data)
 * @dirty_lock: Protects dirty_lines_start and dirty_lines_end
 * @dirty_lines_start: Where to begin updating display
 * @dirty_lines_end: Where to end updating display
 * @gpio.reset: GPIO used to reset display
 * @gpio.dc: Data/Command signal, also known as RS
 * @gpio.rd: Read latching signal
 * @gpio.wr: Write latching signal
 * @gpio.latch: Bus latch signal, eg. 16->8 bit bus latch
 * @gpio.cs: LCD Chip Select with parallel interface bus
 * @gpio.db[16]: Parallel databus
 * @gpio.led[16]: Led control signals
 * @gpio.aux[16]: Auxillary signals, not used by core
 * @init_sequence: Pointer to LCD initialization array
 * @gamma.lock: Mutex for Gamma curve locking
 * @gamma.curves: Pointer to Gamma curve array
 * @gamma.num_values: Number of values per Gamma curve
 * @gamma.num_curves: Number of Gamma curves
 * @debug: Pointer to debug value
 * @current_debug:
 * @first_update_done: Used to only time the first display update
 * @update_time: Used to calculate 'fps' in debug output
 * @bgr: BGR mode/\n
 * @extra: Extra info needed by driver
 */
struct fbtft_par {
	struct spi_device *spi;
	struct platform_device *pdev;
	struct fb_info *info;
	struct fbtft_platform_data *pdata;
	u16 *ssbuf;
	u32 pseudo_palette[16];
	struct {
		void *buf;
		dma_addr_t dma;
		size_t len;
	} txbuf;
	u8 *buf;
	u8 startbyte;
	struct fbtft_ops fbtftops;
	spinlock_t dirty_lock;
	unsigned dirty_lines_start;
	unsigned dirty_lines_end;
	struct {
		int reset;
		int dc;
		int rd;
		int wr;
		int latch;
		int cs;
		int db[16];
		int led[16];
		int aux[16];
	} gpio;
	int *init_sequence;
	struct {
		struct mutex lock;
		unsigned long *curves;
		int num_values;
		int num_curves;
	} gamma;
	unsigned long debug;
	bool first_update_done;
	struct timespec update_time;
	bool bgr;
	struct hrtimer backlight_pwm_timer;
	struct work_struct backlight_pwm_work;
	void *extra;
};

#endif /* __LINUX_PLATFORM_DATA_FBTFT_H */