/*
 * TI OMAPL PRU SUART Emulation device driver
 * Author: subhasish@mistralsolutions.com
 *
 * This driver supports TI's PRU SUART Emulation and the
 * specs for the same is available at <http://www.ti.com>
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed as is WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/module.h>
#include <mach/da8xx.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/clk.h>
#include <linux/serial_reg.h>
#include <linux/delay.h>
#include <linux/ti_omapl_pru_suart.h>
#include "omapl_suart_board.h"
#include "suart_api.h"
#include "suart_utils.h"
#include "suart_err.h"
#include "pru.h"

#define NR_SUART 2
#define DRV_NAME "ti_omapl_pru_suart"
#define DRV_DESC "TI PRU SUART Controller Driver v0.1"
#define MAX_SUART_RETRIES 100
#define SUART_CNTX_SZ 512
#define PLATFORM_SUART_RES_SZ 5
#define SUART_FIFO_TIMEOUT_DFLT 10
#define SUART_FIFO_TIMEOUT_MIN 4
#define SUART_FIFO_TIMEOUT_MAX 500

//#define __SUART_DEBUG 1
#ifdef __SUART_DEBUG
#define __suart_debug(fmt, args...) printk(KERN_DEBUG "suart_debug: " fmt, ## args)
#else
#define __suart_debug(fmt, args...)
#endif
#define __suart_err(fmt, args...) printk(KERN_ERR "suart_err: " fmt, ## args)

#if defined(CONFIG_SERIAL_SUART_OMAPL_PRU) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

/* Default timeout set to 5ms */
static int suart_timeout = SUART_FIFO_TIMEOUT_DFLT;
module_param(suart_timeout, int, S_IRUGO);
MODULE_PARM_DESC(suart_timeout, "fifo timeout in milli seconds (min: 4; max: 500)");

struct suart_dma {
	void *dma_vaddr_buff_tx;
	void *dma_vaddr_buff_rx;
	dma_addr_t dma_phys_addr_tx;
	dma_addr_t dma_phys_addr_rx;
};

static dma_addr_t dma_phys_addr;
static void *dma_vaddr_buff;

struct omapl_pru_suart {
	struct uart_port port[NR_SUART];
	arm_pru_iomap pru_arm_iomap;
	struct semaphore port_sem[NR_SUART];
	struct clk *clk_pru;
	struct clk *clk_mcasp;
	const struct firmware *fw;
	suart_struct_handle suart_hdl[NR_SUART];
	struct suart_dma suart_dma_addr[NR_SUART];
	u32 clk_freq_pru;
	u32 clk_freq_mcasp;
	u32 tx_loadsz;
};

static u32 suart_get_duplex(struct omapl_pru_suart *soft_uart, u32 uart_no)
{
	return (soft_uart->suart_hdl[uart_no].uartType);
}

static inline void __stop_tx(struct omapl_pru_suart *soft_uart, u32 uart_no)
{
	u16 txready;
	u32 i;

	/* Check if any TX in progress */
	for (i = 0, txready = 1; (i < 10000) && txready; i++) {
		txready =
		    (pru_softuart_getTxStatus(&soft_uart->suart_hdl[uart_no])
		    & CHN_TXRX_STATUS_RDY);
	}
	/* To stop tx, disable the TX interrupt */
	suart_intr_clrmask(soft_uart->suart_hdl[uart_no].uartNum, PRU_TX_INTR,
			   CHN_TXRX_IE_MASK_CMPLT);
	pru_softuart_clrTxStatus(&soft_uart->suart_hdl[uart_no]);
}


static void pru_suart_stop_tx(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
	    container_of(port, struct omapl_pru_suart, port[port->line]);

	__stop_tx(soft_uart, port->line);
}

static void omapl_pru_tx_chars(struct omapl_pru_suart *soft_uart, u32 uart_no)
{
	struct circ_buf *xmit = &soft_uart->port[uart_no].state->xmit;
	int count = 0;

	if (!(suart_get_duplex(soft_uart, uart_no) & ePRU_SUART_HALF_TX)) {
		return;
	}

	if (down_trylock(&soft_uart->port_sem[uart_no]))
		return;

	if (uart_circ_empty(xmit) || uart_tx_stopped(&soft_uart->port[uart_no])) {
		pru_suart_stop_tx(&soft_uart->port[uart_no]);
		up(&soft_uart->port_sem[uart_no]);
		return;
	}

	for (count = 0; count <= soft_uart->tx_loadsz; count++) {
		*((char *)soft_uart->suart_dma_addr[uart_no].dma_vaddr_buff_tx +
		  count) = xmit->buf[xmit->tail];
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);	/* limit to PAGE_SIZE */
		soft_uart->port[uart_no].icount.tx++;
		if (uart_circ_empty(xmit)) {
			uart_circ_clear(xmit);
			break;
		}
	}

	if (count == (SUART_FIFO_LEN + 1))
			count = SUART_FIFO_LEN;

	/* Write the character to the data port */
	if (SUART_SUCCESS != pru_softuart_write(&soft_uart->suart_hdl[uart_no],
						(unsigned int *)
						&soft_uart->suart_dma_addr
						[uart_no].dma_phys_addr_tx,
						count)) {
		__suart_err("failed to tx data\n");
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&soft_uart->port[uart_no]);

#if 0
	if (uart_circ_empty(xmit)){
		__stop_tx(soft_uart, uart_no);
	}
#endif
}

static void omapl_pru_rx_chars(struct omapl_pru_suart *soft_uart, u32 uart_no)
{
	struct tty_port *port = &soft_uart->port[uart_no].state->port;
	char tty_flg[SUART_FIFO_LEN + 1] = {[0 ... SUART_FIFO_LEN] = TTY_NORMAL};
	u16 rx_status, data_len = SUART_FIFO_LEN;
	unsigned int data_len_read = 0;
	unsigned char suart_data[SUART_FIFO_LEN + 1];
	int i = 0, ignore_break = 0;

	if (!(suart_get_duplex(soft_uart, uart_no) & ePRU_SUART_HALF_RX))
		return;
	/* read the status */
	rx_status = pru_softuart_getRxStatus(&soft_uart->suart_hdl[uart_no]);
	pru_softuart_read_data (&soft_uart->suart_hdl[uart_no], suart_data,
					data_len + 1 , &data_len_read);

	for (i = 0; i < data_len_read; i++) {
		/* check for sys rq */
		soft_uart->port[uart_no].icount.rx++;
		if (uart_handle_sysrq_char(&soft_uart->port[uart_no],
							suart_data[i]))
			continue;
	}

	/* check for errors */
	if (rx_status & CHN_TXRX_STATUS_ERR) {
		if (rx_status & CHN_TXRX_STATUS_FE)
			soft_uart->port[uart_no].icount.frame++;
		if (rx_status & CHN_TXRX_STATUS_OVRNERR)
			soft_uart->port[uart_no].icount.overrun++;
		if (rx_status & CHN_TXRX_STATUS_BI) {
			soft_uart->port[uart_no].icount.brk++;
			if (uart_handle_break(&soft_uart->port[uart_no]))
				ignore_break = 1;
		}
		rx_status &= (~soft_uart->port[uart_no].ignore_status_mask
				& soft_uart->port[uart_no].read_status_mask);
		if (rx_status & CHN_TXRX_STATUS_FE)
			tty_flg[data_len_read] = TTY_FRAME;
		if (rx_status & CHN_TXRX_STATUS_OVRNERR)
			memset(tty_flg, TTY_OVERRUN, SUART_FIFO_LEN + 1);
		if (rx_status & CHN_TXRX_STATUS_BI)
			tty_flg[data_len_read] = TTY_BREAK;
	}
	/* update the tty data structure */
	if(data_len_read)
		tty_insert_flip_string_flags(port, suart_data,
					tty_flg, data_len_read);

	if ((rx_status & CHN_TXRX_STATUS_ERR) && !ignore_break
				&& (data_len_read < (SUART_FIFO_LEN + 1)))
		tty_insert_flip_char(port, suart_data[data_len_read],
					tty_flg[data_len_read]);

	pru_softuart_clrRxStatus(&soft_uart->suart_hdl[uart_no]);
	/* push data into tty */
	spin_unlock(&soft_uart->port[uart_no].lock);
	tty_flip_buffer_push(port);
	spin_lock(&soft_uart->port[uart_no].lock);
}

static irqreturn_t omapl_pru_suart_interrupt(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);
	u16 txrx_flag;
	u32 ret;
	unsigned long flags = 0;
	u16 uartNum = port->line + 1;

	spin_lock_irqsave(&soft_uart->port[port->line].lock, flags);
	do {
		ret = pru_softuart_get_isrstatus(uartNum, &txrx_flag);
		if (PRU_SUART_SUCCESS != ret) {
			__suart_err
			("suart%d: failed to get interrupt, ret: 0x%X txrx_flag 0x%X\n",
			port->line, ret, txrx_flag);
			spin_unlock_irqrestore(&soft_uart->port[port->line].lock, flags);
			return IRQ_NONE;
		}
		if ((PRU_RX_INTR & txrx_flag) == PRU_RX_INTR) {
			pru_intr_clr_isrstatus(uartNum, PRU_RX_INTR);
			if ((soft_uart->port[port->line].ignore_status_mask &
				CHN_TXRX_STATUS_RDY) == CHN_TXRX_STATUS_RDY) {
				pru_softuart_clrRxStatus(&soft_uart->suart_hdl[port->line]);
			} else {
				omapl_pru_rx_chars(soft_uart, port->line);
			}
		}

		if ((PRU_TX_INTR & txrx_flag) == PRU_TX_INTR) {
			pru_intr_clr_isrstatus(uartNum, PRU_TX_INTR);
			pru_softuart_clrTxStatus(&soft_uart->suart_hdl[port->line]);
			up(&soft_uart->port_sem[port->line]);
			omapl_pru_tx_chars(soft_uart, port->line);
		}
	} while (txrx_flag & (PRU_RX_INTR | PRU_TX_INTR));

	spin_unlock_irqrestore(&soft_uart->port[port->line].lock, flags);
	return IRQ_HANDLED;
}

static void pru_suart_stop_rx(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
	    container_of(port, struct omapl_pru_suart, port[port->line]);
	/* disable rx interrupt */
	suart_intr_clrmask(soft_uart->suart_hdl[port->line].uartNum,
	                   PRU_RX_INTR, CHN_TXRX_IE_MASK_BI
	                   | CHN_TXRX_IE_MASK_FE | CHN_TXRX_IE_MASK_CMPLT
	                   | CHN_TXRX_IE_MASK_TIMEOUT);
}

static void pru_suart_enable_ms(struct uart_port *port)
{
	__suart_err("modem control timer not supported\n");
}

static void pru_suart_start_tx(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);
	/* unmask the tx interrupts */

	suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
			   PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
	omapl_pru_tx_chars(soft_uart, port->line);
}

static unsigned int pru_suart_tx_empty(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);

	return(pru_softuart_getTxStatus(&soft_uart->suart_hdl[port->line])
		& CHN_TXRX_STATUS_RDY) ? 0 : TIOCSER_TEMT;
}

static unsigned int pru_suart_get_mctrl(struct uart_port *port)
{
	return -ENOTSUPP;
}

static void pru_suart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	__suart_debug("modem control not supported\n");
}

static void pru_suart_break_ctl(struct uart_port *port, int break_state)
{
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);
	unsigned long flags = 0;

	spin_lock_irqsave(&port->lock, flags);

	if (break_state == -1)
		suart_intr_clrmask(soft_uart->suart_hdl[port->line].uartNum,
				   PRU_RX_INTR, CHN_TXRX_IE_MASK_BI);
	else
		suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
				   PRU_RX_INTR, CHN_TXRX_IE_MASK_BI);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void pru_suart_set_termios(struct uart_port *port,
				  struct ktermios *termios,
				  struct ktermios *old)
{
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);
	unsigned char cval = 0;
	unsigned long flags = 0;
	unsigned int baud = 0;
	unsigned int old_csize = old ? old->c_cflag & CSIZE : CS8;

/*
 * Do not allow unsupported configurations to be set
 */
	if (1) {
		termios->c_cflag &= ~(HUPCL | CRTSCTS | CMSPAR | CSTOPB
				      | PARENB | PARODD | CMSPAR);
		termios->c_cflag |= CLOCAL;
	}

	switch (termios->c_cflag & CSIZE) {
	case CS6:
		cval = ePRU_SUART_DATA_BITS6;
		break;
	case CS7:
		cval = ePRU_SUART_DATA_BITS7;
		break;
	default:
	case CS8:
		cval = ePRU_SUART_DATA_BITS8;
		break;
	}
	/*
	 * We do not support CS5.
	 */
	if ((termios->c_cflag & CSIZE) == CS5) {
		termios->c_cflag &= ~CSIZE;
		termios->c_cflag |= old_csize;
	}

	if (SUART_SUCCESS !=
	    pru_softuart_setdatabits(&soft_uart->suart_hdl[port->line], cval,
				     cval))
		__suart_err("failed to set data bits to: %d\n", cval);

/*
 * Ask the core to calculate the divisor for us.
 */
	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff,
				  port->uartclk / 16);

/*
 * Ok, we're now changing the port state.  Do it with
 * interrupts disabled.
 */
	spin_lock_irqsave(&port->lock, flags);

	/* Set the baud */
	if (SUART_SUCCESS !=
	    pru_softuart_setbaud(&soft_uart->suart_hdl[port->line],
				 SUART_DEFAULT_BAUD / baud,
				 SUART_DEFAULT_BAUD / baud))
		__suart_err("failed to set baud to: %d\n", baud);

/*
 * update port->read_config_mask and port->ignore_config_mask
 * to indicate the events we are interested in receiving
 */
	port->read_status_mask = CHN_TXRX_STATUS_OVRNERR | CHN_TXRX_STATUS_ERR;
	if (termios->c_iflag & INPCK) {	/* Input parity check not supported, just enabled FE */
		port->read_status_mask |= CHN_TXRX_STATUS_FE;
		suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
				   PRU_RX_INTR, CHN_TXRX_IE_MASK_FE);
	}
	if (termios->c_iflag & (BRKINT | PARMRK)) {
		port->read_status_mask |= CHN_TXRX_STATUS_BI;
		suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
				   PRU_RX_INTR, CHN_TXRX_IE_MASK_BI);
	}
/*
 * Characters to ignore
 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= CHN_TXRX_STATUS_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= CHN_TXRX_STATUS_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= CHN_TXRX_STATUS_OVRNERR;
	}
/*
 * ignore all characters if CREAD is not set
 */
	if ((termios->c_cflag & CREAD) == 0) {
		port->ignore_status_mask |= CHN_TXRX_STATUS_RDY;
		pru_suart_stop_rx(port);
	}
	/*
	 * update the per port timeout
	 */
	uart_update_timeout(port,termios->c_cflag,baud);

	spin_unlock_irqrestore(&port->lock, flags);
	/* Don't rewrite B0 */
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
}

/*
 *	Grab any interrupt resources and initialise any low level driver
 *	state.  Enable the port for reception.  It should not activate
 *	RTS nor DTR; this will be done via a separate call to set_mctrl.
 *
 *	This method will only be called when the port is initially opened.
 *
 *	Locking: port_sem taken.
 *	Interrupts: globally disabled.
 */
static int pru_suart_startup(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);
	int retval;

	/*
	 * Disable interrupts from this port
	 */
	suart_intr_clrmask(soft_uart->suart_hdl[port->line].uartNum,
			   PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
	suart_intr_clrmask(soft_uart->suart_hdl[port->line].uartNum,
			   PRU_RX_INTR, CHN_TXRX_IE_MASK_BI
			   | CHN_TXRX_IE_MASK_FE | CHN_TXRX_IE_MASK_CMPLT
			   | CHN_TXRX_IE_MASK_TIMEOUT);

	retval = request_irq(port->irq, omapl_pru_suart_interrupt,
			     port->irqflags, "suart_irq", port);
	if (retval) {
		free_irq(port->irq, port);	/* should we free this if err */
		goto out;
	}
	/*
	 * enable interrupts from this port
	 */
	suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
					PRU_RX_INTR, SUART_GBL_INTR_ERR_MASK);

	suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
	                   PRU_RX_INTR, CHN_TXRX_IE_MASK_CMPLT | CHN_TXRX_IE_MASK_TIMEOUT);

	suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
		   PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);

	suart_intr_setmask(soft_uart->suart_hdl[port->line].uartNum,
		   PRU_RX_INTR, CHN_RX_IE_MASK_OVRN);

	if ((suart_get_duplex(soft_uart, port->line) & ePRU_SUART_HALF_TX)
	    == ePRU_SUART_HALF_TX) {
		suart_pru_to_host_intr_enable(soft_uart->suart_hdl[port->line].uartNum,
					      PRU_TX_INTR, true);
	}
	/* Seed RX if port is half-rx or full-duplex */
	if ((suart_get_duplex(soft_uart, port->line) & ePRU_SUART_HALF_RX)
	    == ePRU_SUART_HALF_RX) {
		suart_pru_to_host_intr_enable(soft_uart->suart_hdl[port->line].uartNum,
					      PRU_RX_INTR , true);
		pru_softuart_read(&soft_uart->suart_hdl[port->line],
				  (unsigned int *)
				  &soft_uart->suart_dma_addr[port->line].
				  dma_phys_addr_rx, SUART_FIFO_LEN);
	}

out:
	return retval;
}

/*
 *	Disable the port, disable any break condition that may be in
 *	effect, and free any interrupt resources.  It should not disable
 *	RTS nor DTR; this will have already been done via a separate
 *	call to set_mctrl.
 *
 *	Drivers must not access port->info once this call has completed.
 *
 *	This method will only be called when there are no more users of
 *	this port.
 *
 *	Locking: port_sem taken.
 *	Interrupts: caller dependent.
 */
static void pru_suart_shutdown(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
		container_of(port, struct omapl_pru_suart, port[port->line]);

	/*
	 * Disable interrupts from this port
	 */
	/* Disable BI and FE intr */
	suart_intr_clrmask(soft_uart->suart_hdl[port->line].uartNum,
			   PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
	suart_intr_clrmask(soft_uart->suart_hdl[port->line].uartNum,
			   PRU_RX_INTR, CHN_TXRX_IE_MASK_BI
			   | CHN_TXRX_IE_MASK_FE | CHN_TXRX_IE_MASK_CMPLT
			   | CHN_TXRX_IE_MASK_TIMEOUT | CHN_RX_IE_MASK_OVRN);

	/* free interrupts */
	free_irq(port->irq, port);
}

/*
 *	Return a pointer to a string constant describing the specified
 *  port, or return NULL, in which case the string 'unknown' is
 *  substituted.
 *
 *  Locking: none.
 *	Interrupts: caller dependent.
 */
static const char *pru_suart_type(struct uart_port *port)
{
	return "suart_tty";
}

/*
 *	Release any memory and IO region resources currently in use by
 *	the port.
 *
 *	Locking: none.
 *	Interrupts: caller dependent.
 */
static void pru_suart_release_port(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
	    container_of(port, struct omapl_pru_suart, port[port->line]);
	struct platform_device *pdev = to_platform_device(port->dev);

	if (0 != pru_softuart_close(&soft_uart->suart_hdl[port->line])) {
		dev_err(&pdev->dev, "failed to close suart\n");
	}
	return;
}

/*
 *	Request any memory and IO region resources required by the port.
 *	If any fail, no resources should be registered when this function
 *	returns, and it should return -EBUSY on failure.
 *
 *	Locking: none.
 *	Interrupts: caller dependent.
 *
 *	We need to d/l the f/w in probe and since this api is called per uart, the 
 *	request_mem_region should be called in probe itself.
 *	We call the pru_open routine only here. not sure if aesthetically correct.
 */
static int pru_suart_request_port(struct uart_port *port)
{
	struct omapl_pru_suart *soft_uart =
	    container_of(port, struct omapl_pru_suart, port[port->line]);
	struct platform_device *pdev = to_platform_device(port->dev);
	suart_config pru_suart_config;
	u32 timeout = 0;
	u32 err = 0;
	if (soft_uart == NULL) {
		__suart_err("soft_uart ptr failed\n");
		return -ENODEV;
	}
	err = pru_softuart_open(&soft_uart->suart_hdl[port->line]);
	if (PRU_SUART_SUCCESS != err) {
		dev_err(&pdev->dev, "failed to open suart: %d\n", err);
		err = -ENODEV;
		goto exit;
	}

	/* set fifo timeout */
	if (SUART_FIFO_TIMEOUT_MIN > suart_timeout){
		__suart_err("fifo timeout less than %d ms not supported\n", SUART_FIFO_TIMEOUT_MIN);
		suart_timeout = SUART_FIFO_TIMEOUT_MIN;
	} else if (SUART_FIFO_TIMEOUT_MAX < suart_timeout){
		__suart_err("fifo timeout more than %d ms not supported\n", SUART_FIFO_TIMEOUT_MAX);
		suart_timeout = SUART_FIFO_TIMEOUT_MAX;
	}

	/* This is only for x8 */
	timeout = (SUART_DEFAULT_BAUD * suart_timeout) / 1000;
	pru_set_fifo_timeout(timeout);

	if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART1) {
		pru_suart_config.TXSerializer = PRU_SUART1_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART1_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART2) {
		pru_suart_config.TXSerializer = PRU_SUART2_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART2_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART3) {
		pru_suart_config.TXSerializer = PRU_SUART3_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART3_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART4) {
		pru_suart_config.TXSerializer = PRU_SUART4_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART4_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART5) {
		pru_suart_config.TXSerializer = PRU_SUART5_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART5_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART6) {
		pru_suart_config.TXSerializer = PRU_SUART6_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART6_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART7) {
		pru_suart_config.TXSerializer = PRU_SUART7_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART7_CONFIG_RX_SER;
	} else if (soft_uart->suart_hdl[port->line].uartNum == PRU_SUART_UART8) {
		pru_suart_config.TXSerializer = PRU_SUART8_CONFIG_TX_SER;
		pru_suart_config.RXSerializer = PRU_SUART8_CONFIG_RX_SER;
	} else {
		return -ENOTSUPP;
	}

	/* Some defaults to startup. reconfigured by terimos later */
	pru_suart_config.txClkDivisor = 1;
	pru_suart_config.rxClkDivisor = 1;
	pru_suart_config.txBitsPerChar = ePRU_SUART_DATA_BITS8;
	pru_suart_config.rxBitsPerChar = ePRU_SUART_DATA_BITS8;	/* including start and stop bit 8 + 2 */
	pru_suart_config.Oversampling = SUART_DEFAULT_OVRSMPL;

	if (PRU_SUART_SUCCESS !=
	    pru_softuart_setconfig(&soft_uart->suart_hdl[port->line],
				   &pru_suart_config)) {
		dev_err(&pdev->dev,
			"pru_softuart_setconfig: failed to set config: %X\n",
			err);
	}
exit:
	return err;
}

/*
 * 	Perform any autoconfiguration steps required for the port.  `flag`
 * 	contains a bit mask of the required configuration.  UART_CONFIG_TYPE
 * 	indicates that the port requires detection and identification.
 * 	port->type should be set to the type found, or PORT_UNKNOWN if
 * 	no port was detected.
 *
 * 	UART_CONFIG_IRQ indicates autoconfiguration of the interrupt signal,
 * 	which should be probed using standard kernel autoprobing techniques.
 * 	This is not necessary on platforms where ports have interrupts
 * 	internally hard wired (eg, system on a chip implementations).
 *
 * 	Locking: none.
 * 	Interrupts: caller dependent.
 *
 */
static void pru_suart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE && pru_suart_request_port(port) == 0)
		port->type = OMAPL_PRU_SUART;
}

/*
 *	Verify the new serial port information contained within serinfo is
 *	suitable for this port type.
 *
 * 	Locking: none.
 * 	Interrupts: caller dependent.
 */
static int pru_suart_verify_port(struct uart_port *port,
				 struct serial_struct *ser)
{
	struct omapl_pru_suart *soft_uart =
	    container_of(port, struct omapl_pru_suart, port[port->line]);
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != OMAPL_PRU_SUART)
		ret = -EINVAL;
	if (soft_uart->port[port->line].irq != ser->irq)
		ret = -EINVAL;
	if (ser->io_type != UPIO_MEM)
		ret = -EINVAL;
	if (soft_uart->port[port->line].uartclk / 16 != ser->baud_base)
		ret = -EINVAL;
	if ((void *)soft_uart->port[port->line].mapbase != ser->iomem_base)
		ret = -EINVAL;
	if (soft_uart->port[port->line].iobase != ser->port)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops pru_suart_ops = {
	.tx_empty = pru_suart_tx_empty,
	.set_mctrl = pru_suart_set_mctrl,
	.get_mctrl = pru_suart_get_mctrl,
	.stop_tx = pru_suart_stop_tx,
	.start_tx = pru_suart_start_tx,
	.stop_rx = pru_suart_stop_rx,
	.enable_ms = pru_suart_enable_ms,
	.break_ctl = pru_suart_break_ctl,
	.startup = pru_suart_startup,
	.shutdown = pru_suart_shutdown,
	.set_termios = pru_suart_set_termios,
	.type = pru_suart_type,
	.release_port = pru_suart_release_port,
	.request_port = pru_suart_request_port,
	.config_port = pru_suart_config_port,
	.verify_port = pru_suart_verify_port,
};

static struct uart_driver pru_suart_reg = {
	.owner = THIS_MODULE,
	.driver_name = DRV_NAME,
	.dev_name = "ttySU",
	.major = 0,
	.minor = 16,
	.nr = NR_SUART,
};

static int omapl_pru_suart_probe(struct platform_device *pdev)
{
	struct omapl_pru_suart *soft_uart;
	struct ti_pru_suart_platform_data *pdata;
	struct resource *res_mem[PLATFORM_SUART_RES_SZ];
	int err, i;
	unsigned char *fw_data = NULL;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data provided for pru!\n");
		return -ENODEV;
	}

	soft_uart = kzalloc(sizeof(struct omapl_pru_suart), GFP_KERNEL);
	if (!soft_uart)
		return -ENOMEM;

	for (i = 0; i < PLATFORM_SUART_RES_SZ; i++) {
		res_mem[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res_mem[i]) {
			dev_err(&pdev->dev,
				"unable to get pru memory resources!\n");
			err = -ENODEV;
			goto probe_exit;
		}
	}

	if (!request_mem_region(res_mem[0]->start, resource_size(res_mem[0]),
				dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "pru memory region already claimed!\n");
		err = -EBUSY;
		goto probe_exit;
	}
	if (!request_mem_region(res_mem[1]->start, resource_size(res_mem[1]),
				dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "mcasp memory region already claimed!\n");
		err = -EBUSY;
		goto probe_exit_1;
	}
	if (!request_mem_region(res_mem[2]->start, resource_size(res_mem[2]),
				dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "psc0 memory region already claimed!\n");
		err = -EBUSY;
		goto probe_exit_2;
	}
	if (!request_mem_region(res_mem[3]->start, resource_size(res_mem[3]),
				dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "psc1 memory region already claimed!\n");
		err = -EBUSY;
		goto probe_exit_3;
	}

	soft_uart->pru_arm_iomap.pru_io_addr = ioremap(res_mem[0]->start,
						       resource_size(res_mem
								     [0]));
	if (!soft_uart->pru_arm_iomap.pru_io_addr) {
		dev_err(&pdev->dev, "ioremap failed\n");
		err = -ENOMEM;
		goto probe_exit_free_region;
	}
	soft_uart->pru_arm_iomap.mcasp_io_addr = ioremap(res_mem[1]->start,
							 resource_size(res_mem
								       [1]));
	if (!soft_uart->pru_arm_iomap.mcasp_io_addr) {
		dev_err(&pdev->dev, "ioremap failed\n");
		err = -ENOMEM;
		goto probe_exit_iounmap_1;
	}
	soft_uart->pru_arm_iomap.psc0_io_addr = ioremap(res_mem[2]->start,
							resource_size(res_mem
								      [2]));
	if (!soft_uart->pru_arm_iomap.psc0_io_addr) {
		dev_err(&pdev->dev, "ioremap failed\n");
		err = -ENOMEM;
		goto probe_exit_iounmap_2;
	}
	soft_uart->pru_arm_iomap.psc1_io_addr = ioremap(res_mem[3]->start,
							resource_size(res_mem
								      [3]));
	if (!soft_uart->pru_arm_iomap.psc1_io_addr) {
		dev_err(&pdev->dev, "ioremap failed\n");
		err = -ENOMEM;
		goto probe_exit_iounmap_3;
	}
	soft_uart->pru_arm_iomap.syscfg_io_addr =
	    IO_ADDRESS(DA8XX_SYSCFG0_BASE);
	if (!soft_uart->pru_arm_iomap.syscfg_io_addr) {
		dev_err(&pdev->dev, "ioremap failed\n");
		err = -ENOMEM;
		goto probe_exit_iounmap_4;
	}

	soft_uart->clk_pru = clk_get(&pdev->dev, "pruss");
	if (IS_ERR(soft_uart->clk_pru)) {
		dev_err(&pdev->dev, "no clock available: pruss\n");
		err = PTR_ERR(soft_uart->clk_pru);
		soft_uart->clk_pru = NULL;
		goto probe_exit_iounmap_2;
	}
	soft_uart->clk_freq_pru = clk_get_rate(soft_uart->clk_pru);

	soft_uart->clk_mcasp = clk_get(NULL, "mcasp");
	if (IS_ERR(soft_uart->clk_mcasp)) {
		dev_err(&pdev->dev, "no clock available: mcasp\n");
		err = PTR_ERR(soft_uart->clk_mcasp);
		soft_uart->clk_mcasp = NULL;
		goto probe_exit_clk_pru;
	}
	soft_uart->clk_freq_mcasp = clk_get_rate(soft_uart->clk_mcasp);
	clk_enable(soft_uart->clk_mcasp);
	clk_enable(soft_uart->clk_pru);
	err = request_firmware(&soft_uart->fw, "PRU_SUART.bin",
			       &pdev->dev);
	if (err) {
		dev_err(&pdev->dev, "can't load firmware\n");
		err = -ENODEV;
		goto probe_exit_clk;
	}
	dev_info(&pdev->dev, "fw size %td. downloading...\n",
		 soft_uart->fw->size);

	/* download firmware into pru  & init */
	fw_data = kmalloc(soft_uart->fw->size, GFP_KERNEL);
	memcpy((void *)fw_data, (const void *)soft_uart->fw->data,
	       soft_uart->fw->size);

	dma_phys_addr = res_mem[4]->start;
	dma_vaddr_buff = ioremap(res_mem[4]->start, resource_size(res_mem[4]));
	if (!dma_vaddr_buff) {
		__suart_err("Failed to allocate shared ram.\n");
		err = -EFAULT;
		goto probe_exit_clk;
	}

	soft_uart->pru_arm_iomap.pFifoBufferPhysBase = (void *)dma_phys_addr;
	soft_uart->pru_arm_iomap.pFifoBufferVirtBase = (void *)dma_vaddr_buff;
	soft_uart->pru_arm_iomap.pru_clk_freq = (soft_uart->clk_freq_pru / 1000000);

	err = pru_softuart_init(SUART_DEFAULT_BAUD, SUART_DEFAULT_BAUD,
				SUART_DEFAULT_OVRSMPL, fw_data,
				soft_uart->fw->size, &soft_uart->pru_arm_iomap);
	if (err) {
		dev_err(&pdev->dev, "pru init error\n");
		err = -ENODEV;
		kfree((const void *)fw_data);
		goto probe_release_fw;
	}
	kfree((const void *)fw_data);

	for (i = 0; i < NR_SUART; i++) {
		soft_uart->port[i].ops = &pru_suart_ops;
		soft_uart->port[i].iotype = UPIO_MEM;	/* user conf parallel io */
		soft_uart->port[i].flags = UPF_BOOT_AUTOCONF | UPF_IOREMAP;
		soft_uart->port[i].mapbase = res_mem[1]->start;
		soft_uart->port[i].membase =
				(unsigned char *)&soft_uart->pru_arm_iomap;
		soft_uart->port[i].type = OMAPL_PRU_SUART;
		soft_uart->port[i].irq = platform_get_irq(pdev, i);
		soft_uart->port[i].dev = &pdev->dev;
		soft_uart->port[i].irqflags = IRQF_SHARED;
		soft_uart->port[i].uartclk = soft_uart->clk_freq_mcasp;	/* 24MHz */
		soft_uart->port[i].fifosize = SUART_FIFO_LEN;
		soft_uart->tx_loadsz = SUART_FIFO_LEN;
		soft_uart->port[i].custom_divisor = 1;
		soft_uart->port[i].line = i;	/* need the id/line from pdev */
		soft_uart->suart_hdl[i].uartNum = i + 1;
		spin_lock_init(&soft_uart->port[i].lock);
		soft_uart->port[i].serial_in = NULL;

		soft_uart->suart_dma_addr[i].dma_vaddr_buff_tx =
				dma_vaddr_buff + (2 * SUART_CNTX_SZ * i);

		soft_uart->suart_dma_addr[i].dma_vaddr_buff_rx =
			dma_vaddr_buff + ((2 * SUART_CNTX_SZ * i) + SUART_CNTX_SZ);

		soft_uart->suart_dma_addr[i].dma_phys_addr_tx =
					dma_phys_addr + (2 * SUART_CNTX_SZ * i);

		soft_uart->suart_dma_addr[i].dma_phys_addr_rx =
			dma_phys_addr + ((2 * SUART_CNTX_SZ * i) + SUART_CNTX_SZ);

		soft_uart->port[i].serial_out = NULL;
		uart_add_one_port(&pru_suart_reg, &soft_uart->port[i]);
		sema_init(&soft_uart->port_sem[i], 1);
	}
	platform_set_drvdata(pdev, &soft_uart->port[0]);

	dev_info(&pdev->dev,
		 "%s device registered"
		 "(pru_clk=%d, asp_clk=%d)\n",
		 DRV_NAME, soft_uart->clk_freq_pru, soft_uart->clk_freq_mcasp);

	return 0;

probe_release_fw:
	release_firmware(soft_uart->fw);
probe_exit_clk:
	clk_put(soft_uart->clk_mcasp);
probe_exit_clk_pru:
	clk_put(soft_uart->clk_pru);
probe_exit_iounmap_4:
	iounmap(soft_uart->pru_arm_iomap.psc1_io_addr);
probe_exit_iounmap_3:
	iounmap(soft_uart->pru_arm_iomap.psc0_io_addr);
probe_exit_iounmap_2:
	iounmap(soft_uart->pru_arm_iomap.mcasp_io_addr);
probe_exit_iounmap_1:
	iounmap(soft_uart->pru_arm_iomap.pru_io_addr);
probe_exit_free_region:
	release_mem_region(res_mem[3]->start, resource_size(res_mem[3]));
probe_exit_3:
	release_mem_region(res_mem[2]->start, resource_size(res_mem[2]));
probe_exit_2:
	release_mem_region(res_mem[1]->start, resource_size(res_mem[1]));
probe_exit_1:
	release_mem_region(res_mem[0]->start, resource_size(res_mem[0]));
probe_exit:
	kfree(soft_uart);
	return err;
}

static int omapl_pru_suart_remove(struct platform_device *pdev)
{
	struct ti_pru_suart_platform_data *pdata;
	struct omapl_pru_suart *soft_uart = platform_get_drvdata(pdev);
	struct resource *res_mem[4];
	int i;
	u32 err = 0;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data provided for pru!\n");
		return -ENODEV;
	}
	platform_set_drvdata(pdev, NULL);

	for (i = 0; i < 4; i++) {
		res_mem[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res_mem[i]) {
			dev_err(&pdev->dev,
				"unable to get pru memory resources!\n");
			err = -ENODEV;
		}
	}

	if (soft_uart) {
		for (i = 0; i < NR_SUART; i++) {
			uart_remove_one_port(&pru_suart_reg,
					     &soft_uart->port[i]);
		}
	}
	release_firmware(soft_uart->fw);
	clk_put(soft_uart->clk_mcasp);
	clk_put(soft_uart->clk_pru);
	pru_mcasp_deinit ();
	clk_disable(soft_uart->clk_mcasp);
	pru_softuart_deinit();
	clk_disable(soft_uart->clk_pru);
	iounmap(soft_uart->pru_arm_iomap.mcasp_io_addr);
	iounmap(soft_uart->pru_arm_iomap.pru_io_addr);
	iounmap(soft_uart->pru_arm_iomap.psc0_io_addr);
	iounmap(soft_uart->pru_arm_iomap.psc1_io_addr);
	release_mem_region(res_mem[0]->start, resource_size(res_mem[0]));
	release_mem_region(res_mem[1]->start, resource_size(res_mem[1]));
	release_mem_region(res_mem[2]->start, resource_size(res_mem[2]));
	release_mem_region(res_mem[3]->start, resource_size(res_mem[3]));
	kfree(soft_uart);
	return err;
}

#define omapl_pru_suart_suspend NULL
#define omapl_pru__suart_resume NULL

static struct platform_driver serial_omapl_pru_driver = {
	.probe = omapl_pru_suart_probe,
	.remove = omapl_pru_suart_remove,
	.suspend = omapl_pru_suart_suspend,
	.resume = omapl_pru__suart_resume,
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init pru_suart_init(void)
{
	int ret;

	__suart_debug("SUART serial driver loaded\n");
	pru_suart_reg.nr = NR_SUART;
	ret = uart_register_driver(&pru_suart_reg);
	if (ret)
		return ret;
	ret = platform_driver_register(&serial_omapl_pru_driver);
	if (ret)
		goto out;

	return ret;
out:
	uart_unregister_driver(&pru_suart_reg);
	return ret;
}

module_init(pru_suart_init);

static void __exit omapl_pru_suart_exit(void)
{
	platform_driver_unregister(&serial_omapl_pru_driver);
	uart_unregister_driver(&pru_suart_reg);
	iounmap(dma_vaddr_buff);
	__suart_debug("SUART serial driver unloaded\n");
}

module_exit(omapl_pru_suart_exit);

/* Module information */
MODULE_AUTHOR("Subhasish Ghosh <subhasish@mistralsolutions.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:"DRV_NAME);
