/*
 * Copyright  2008-2009 INRIA/SensTools
 * 
 * <dev-team@sentools.info>
 * 
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 * 
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/**
 * \addtogroup cc2420
 * @{
 */

/**
 * \file
 * \brief CC2420 driver implementation
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date January 09
 */

/**
 * @}
 */

#include <io.h>
#include <signal.h>

#include "spi1.h"
#include "cc2420.h"


uint16_t cc2420_read_reg(uint8_t addr) {
	uint16_t value;
	spi1_select(SPI1_CC2420);
	spi1_write_single(addr | CC2420_READ_ACCESS);
	value = spi1_read_single();
	value <<= 8;
	value += spi1_read_single();
	spi1_deselect(SPI1_CC2420);

	return value;
}

void cc2420_write_reg(uint8_t addr, uint16_t value) {
	spi1_select(SPI1_CC2420);
	spi1_write_single(addr | CC2420_WRITE_ACCESS);
	spi1_write_single((uint8_t) (value >> 8));
	spi1_write_single((uint8_t) (value & 0xFF));
	spi1_deselect(SPI1_CC2420);
}

uint8_t cc2420_strobe_cmd(uint8_t cmd) {
	uint8_t ret;
	spi1_select(SPI1_CC2420);
	ret = spi1_write_single((cmd));
	spi1_deselect(SPI1_CC2420);
	return ret;
}

void cc2420_read_ram(uint16_t addr, uint8_t* buffer, uint16_t len) {
	uint16_t i;
	spi1_select(SPI1_CC2420);
	spi1_write_single(CC2420_RAM_ACCESS | (addr & 0x7F));
	spi1_write_single(((addr >> 1) & 0xC0) | CC2420_RAM_READ_ACCESS);
	for (i = 0; i < len; i++) {
		buffer[i] = spi1_read_single();
	}
	spi1_deselect(SPI1_CC2420);
}

void cc2420_write_ram(uint16_t addr, uint8_t* buffer, uint16_t len) {
	uint16_t i;
	spi1_select(SPI1_CC2420);
	spi1_write_single(CC2420_RAM_ACCESS | (addr & 0x7F));
	spi1_write_single(((addr >> 1) & 0xC0) | CC2420_RAM_WRITE_ACCESS);
	for (i = 0; i < len; i++) {
		spi1_write_single(buffer[i]);
	}
	spi1_deselect(SPI1_CC2420);
}

static cc2420_cb_t fifo_cb = 0x0, fifop_cb = 0x0, sfd_cb = 0x0, cca_cb = 0x0;

void inline micro_delay(register unsigned int n) {
	__asm__ __volatile__ (
			"1: \n"
			" dec %[n] \n"
			" jne 1b \n"
			: [n] "+r"(n));
}

uint16_t cc2420_init(void) {
	uint16_t reg;

	/* Initialize SPI */
	spi1_init();
	
	/* Initialize IO pins */
	CC2420_IO_INIT();

	micro_delay(2000);
	
	/* Power on the cc2420 and reset it */
	VREGEN_SET(1);
	RESETn_SET(0);
	micro_delay(2000);
	RESETn_SET(1);

	/* Turn on the crystal oscillator. */
	cc2420_strobe_cmd(CC2420_STROBE_XOSCON);

	reg = cc2420_read_reg(CC2420_REG_MDMCTRL0);
#ifdef CC2420_ENABLE_ADR_DECODE
	/* Turn on automatic packet acknowledgment and address decoding. */
	reg |= ADR_DECODE;
#elif CC2420_ENABLE_AUTOACK
	reg |= ADR_DECODE;
	reg |= AUTOACK;
#else
	/* Turn off automatic packet acknowledgment and address decoding. */
	reg &= ~AUTOACK;
	reg &= ~ADR_DECODE;
#endif
	cc2420_write_reg(CC2420_REG_MDMCTRL0, reg);

	/* Change default values as recomended in the data sheet, */
	/* RX bandpass filter = 1.3uA. */
	reg = cc2420_read_reg(CC2420_REG_RXCTRL1);
	reg |= RXBPF_LOCUR;
	cc2420_write_reg(CC2420_REG_RXCTRL1, reg);

	/* Set the FIFOP threshold to maximum. */
	cc2420_write_reg(CC2420_REG_IOCFG0, 127);

	/* Turn off "Security enable" (page 32). */
	reg = cc2420_read_reg(CC2420_REG_SECCTRL0);
	reg &= ~RXFIFO_PROTECTION;
	cc2420_write_reg(CC2420_REG_SECCTRL0, reg);

	return 1;
}

/* FIFOs */
void cc2420_fifo_put(uint8_t* data, uint16_t data_length) {
	uint16_t i;
	spi1_select(SPI1_CC2420);
	spi1_write_single(CC2420_REG_TXFIFO | CC2420_WRITE_ACCESS);
	for (i = 0; i < data_length; i++) {
		spi1_write_single(data[i]);
	}
	spi1_deselect(SPI1_CC2420);
}

void cc2420_fifo_get(uint8_t* data, uint16_t data_length) {
	uint16_t i;
	spi1_select(SPI1_CC2420);
	spi1_write_single(CC2420_REG_RXFIFO | CC2420_READ_ACCESS);
	for (i = 0; i < data_length; i++) {
		data[i] = spi1_read_single();
	}
	spi1_deselect(SPI1_CC2420);
}

/* Interrupts */

// FIFO
void cc2420_io_fifo_register_cb(cc2420_cb_t cb) {
	fifo_cb = cb;
}


// FIFOP
void cc2420_io_fifop_register_cb(cc2420_cb_t cb) {
	fifop_cb = cb;
}


// SFD
void cc2420_io_sfd_register_cb(cc2420_cb_t cb) {
	sfd_cb = cb;
}


// CCA
void cc2420_io_cca_register_cb(cc2420_cb_t cb) {
	cca_cb = cb;
}


void port1irq(void);
/**
 * Interrupt service routine for PORT1.
 * Used for handling CC2420 interrupts triggered on
 * the IO pins.
 */
interrupt(PORT1_VECTOR) port1irq(void) {
	if (P1IFG & FIFO_PIN) {
		FIFO_INT_CLEAR();
		if (fifo_cb != 0x0) {
			if (fifo_cb()) {
				LPM4_EXIT;
			}
		}

	}

	if (P1IFG & FIFOP_PIN) {
		FIFOP_INT_CLEAR();
		if (fifop_cb != 0x0) {
			if (fifop_cb()) {
				LPM4_EXIT;
			}
		}

	}

	if (P1IFG & SFD_PIN) {
		SFD_INT_CLEAR();
		if (sfd_cb != 0x0) {
			if (sfd_cb()) {
				LPM4_EXIT;
			}
		}

	}
}
