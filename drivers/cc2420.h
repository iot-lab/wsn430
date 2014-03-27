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
 * \defgroup cc2420 CC2420 radio driver
 * \ingroup wsn430
 * @{
 * This module allows the user to control the CC2420 radio chip.
 *
 * The CC2420 is a 2.4GHz radio chip designed to implement the 802.15.4
 * MAC layer specifications, with ZigBee as upper layer.
 * Nevertheless, it may be used for implementing many other
 * radio communication protocols.
 */

/**
 * \file
 * \brief CC2420 driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date January 09
 */
#ifndef _CC2420_H
#define _CC2420_H

#include "cc2420_io.h"
#include "cc2420_globals.h"

/**
 * \brief The callback function type, used for all interrupts.
 *
 * The registered function should return 0 if the CPU must remain
 * in its low power state, or anything else in order to wake the CPU up.
 * \return 1 for wake up, 0 for stay the same
 */
typedef uint16_t (*cc2420_cb_t)(void);

/**
 * \brief Initialize procedure.
 *
 * This function initializes the CC2420, by setting the default register values.
 * It should be called first, before any other function.
 * \return 1
 */
uint16_t cc2420_init(void);

/* Configuration */

/**
 * Read a CC2420 register via SPI
 * \param addr register address
 * \return the read value
 */
uint16_t cc2420_read_reg(uint8_t addr);

/**
 * Write a value to a CC2420 register via SPI
 * \param addr register address
 * \param value value to write
 */
void cc2420_write_reg(uint8_t addr, uint16_t value);

/**
 * Send a command to CC2420 via SPI
 * \param cmd command to execute
 * \return the status byte
 */
uint8_t cc2420_strobe_cmd(uint8_t cmd);

/**
 * Read data from RXFIFO.
 * \param buffer a pointer to the buffer for storing the data.
 * \param len the number of bytes to read
 */
void cc2420_read_fifo(uint8_t* buffer, uint16_t len);

/**
 * Write data to TXFIFO.
 * \param buffer a pointer to a buffer to read data from.
 * \param len the number of bytes to write.
 */
void cc2420_write_fifo(uint8_t* buffer, uint16_t len);

/**
 * Read data from RAM.
 * \param addr the 9bit address to start reading from.
 * \param buffer a pointer to the buffer to store the data.
 * \param len the number of bytes to read.
 */
void cc2420_read_ram(uint16_t addr, uint8_t* buffer, uint16_t len);

/**
 * Write data to RAM.
 * \param addr the 9bit address to start writing at.
 * \param buffer a pointer to the buffer to read the data from.
 * \param len the number of bytes to write.
 */
void cc2420_write_ram(uint16_t addr, uint8_t* buffer, uint16_t len);

/**
 * \brief Set the operating frequency in MHz.
 * \param freq the frequency, should be between 2048 and 3072
 */
#define cc2420_set_frequency(freq) do { \
	uint16_t reg, _freq; \
	_freq = (freq); \
	if (_freq < 2048) _freq = 2048; \
	_freq -= 2048; \
	if (_freq >= 1024) _freq = 1023; \
	reg = cc2420_read_reg(CC2420_REG_FSCTRL) & FREQ_MASK; \
	reg |= _freq; \
	cc2420_write_reg(CC2420_REG_FSCTRL, reg); \
} while (0)



/**
 * \enum cc2420_2.45GHz_power_config
 * \brief Defines PA_LEVEL value for each power configuration at 2.45GHz
 *        Values taken from cc2420 documentation revision SWRS041B
 */
enum cc2420_2_45GHz_power_config {
        CC2420_2_45GHz_TX_0dBm   = 31,    /**<   0 dBm */
        CC2420_2_45GHz_TX_m1dBm  = 27,    /**<  -1 dBm */
        CC2420_2_45GHz_TX_m3dBm  = 23,    /**<  -3 dBm */
        CC2420_2_45GHz_TX_m5dBm  = 19,    /**<  -5 dBm */
        CC2420_2_45GHz_TX_m7dBm  = 15,    /**<  -7 dBm */
        CC2420_2_45GHz_TX_m10dBm = 11,    /**< -10 dBm */
        CC2420_2_45GHz_TX_m15dBm =  7,    /**< -15 dBm */
        CC2420_2_45GHz_TX_m25dBm =  3,    /**< -25 dBm */
};

/**
 * \brief Set the TX power.
 * \param power the power value
 */
#define cc2420_set_txpower(power) do { \
	uint16_t reg; \
	reg = cc2420_read_reg(CC2420_REG_TXCTRL); \
	reg &= ~PALEVEL_MASK; \
	reg |= (power & PALEVEL_MASK); \
	cc2420_write_reg( CC2420_REG_TXCTRL, reg); \
} while (0)

/**
 * \brief Set the FIFO threshold for setting the FIFOP pin.
 *
 * \param thr the number of bytes in RXFIFO needed by the FIFOP pin
 */
#define cc2420_set_fifopthr(thr) do { \
	uint16_t reg; \
	reg = cc2420_read_reg(CC2420_REG_IOCFG0); \
	reg &= ~FIFOPTHR_MASK; \
	reg |= (thr & FIFOPTHR_MASK); \
	cc2420_write_reg(CC2420_REG_IOCFG0, reg); \
} while (0)

/**
 * \brief Set the CCA threshold.
 *
 * If the RSSI measure is below this value,
 * CCA is asserted. Offset is the same as the RSSI measures.
 * \param thr the threshold value (0x00-0xFF)
 */
#define cc2420_set_ccathr(thr) do { \
	uint16_t reg; \
	reg = cc2420_read_reg(CC2420_REG_RSSI); \
	reg &= ~CCATHR_MASK; \
	reg |= (thr << 8); \
	cc2420_write_reg(CC2420_REG_RSSI, reg); \
} while (0)

/**
 * \brief Set in RAM the PANID value
 * \param panid a pointer to the pan id value (0x0000-0xFFFF)
 * \return 1
 */
#define cc2420_set_panid(panid) \
		cc2420_write_ram(CC2420_RAM_PANID, panid, 2)

/**
 * \brief Set in RAM the SHORTADR value
 * \param shortadr a pointer to the short addess value (0x0000-0xFFFF)
 * \return 1
 */
#define cc2420_set_shortadr(shortadr) \
		cc2420_write_ram(CC2420_RAM_SHORTADR, shortadr, 2)

/**
 * \brief Set in RAM the IEEEADR value
 * \param ieeeadr a pointer to the extended address value (0x0000000000000000-0xFFFFFFFFFFFFFFFF)
 * \return 1
 */
#define cc2420_set_ieeeadr(ieeeadr) \
	cc2420_write_ram(CC2420_RAM_IEEEADR, ieeeadr, 8)

/* Info */
/**
 * \name CC2420 status values
 * @{
 */
#define CC2420_STATUS_XOSC_STABLE  (1<<6)
#define CC2420_STATUS_TX_UNDERFLOW (1<<5)
#define CC2420_STATUS_ENC_BUSY     (1<<4)
#define CC2420_STATUS_TX_ACTIVE    (1<<3)
#define CC2420_STATUS_LOCK         (1<<2)
#define CC2420_STATUS_RSSI_VALID   (1<<1)
/**
 * @}
 */

/**
 * \brief Get the CC2420 chip status.
 *
 * The bits of interest are described above.
 * \return the chip status value
 */

#define cc2420_get_status() cc2420_strobe_cmd(CC2420_STROBE_NOP)

/**
 * \brief Get the CC2420 version word.
 *
 * \return the chip's version
 */

#define cc2420_get_version() cc2420_read_reg(CC2420_REG_MANFIDH)
/**
 * Read the RSSI value. Measure validity can be checked
 * with the RSSI_VALID bit of the Status byte.
 * \return the rssi readout.
 */
#define cc2420_get_rssi() (cc2420_read_reg(CC2420_REG_RSSI) & 0xFF)

/* Commands */
/**
 * Stop the CC2420 oscillator.
 * \return 1
 */
#define cc2420_cmd_xoscoff() cc2420_strobe_cmd(CC2420_STROBE_XOSCOFF)

/**
 * Start the CC2420 oscillator.
 * Check the XOSC_STABLE bit of the Status byte to know when it's stable.
 * \return 1
 */
#define cc2420_cmd_xoscon() cc2420_strobe_cmd(CC2420_STROBE_XOSCON)

/**
 * Stop RX or TX and go to the idle state.
 * \return 1
 */
#define cc2420_cmd_idle() cc2420_strobe_cmd(CC2420_STROBE_RFOFF)

/**
 * Start RX.
 * \return 1
 */
#define cc2420_cmd_rx() cc2420_strobe_cmd(CC2420_STROBE_RXON)

/**
 * Start TX. A complete frame should be present in the TX FIFO.
 * \return 1
 */
#define cc2420_cmd_tx() cc2420_strobe_cmd(CC2420_STROBE_TXON)

/**
 * Start TX if CCA. A complete frame should be present in the TX FIFO.
 * \return 1
 */
#define cc2420_cmd_txoncca() cc2420_strobe_cmd(CC2420_STROBE_TXONCCA)

/**
 * Flush the RX FIFO.
 * \return 1
 */
#define cc2420_cmd_flushrx() cc2420_strobe_cmd(CC2420_STROBE_FLUSHRX)

/**
 * Flush the TX FIFO.
 * \return 1
 */
#define cc2420_cmd_flushtx() cc2420_strobe_cmd(CC2420_STROBE_FLUSHTX)

/**
 * Sens an ACK frame, pending field cleared.
 * \return 1;
 */
#define cc2420_cmd_sack() cc2420_strobe_cmd(CC2420_STROBE_ACK)

/**
 * Sens an ACK frame, pending field set.
 * \return 1;
 */
#define cc2420_cmd_sackpend() cc2420_strobe_cmd(CC2420_STROBE_ACKPEND)

/* FIFOs */
/**
 * Write data to the TX FIFO. Take care to respect the frame format.
 * A complete frame put in FIFO should look like this:
 * length of the following bytes (1 byte) | data (n bytes) | FCS (2 bytes)
 * The FCS bytes are filled by the chip and need not to be written.
 * The length byte value must be (n+3)
 * \param data a pointer to the data to write.
 * \param data_length the number of bytes to write.
 */
void cc2420_fifo_put(uint8_t* data, uint16_t data_length);

/**
 * Read data from the RX FIFO. Respect the frame format.
 * If a frame has been received, the first byte read is the length of the frame,
 * then the data is read, and finally an extra 2byte is read containing
 * the RSSI measure, and the CRC/LQI byte. The CRC bit of this later byte
 * is set to 1 if the CRC was correct, or 0 if not. It should be checked.
 * \param data a pointer to a buffer to store the data.
 * \param data_length the number of bytes to read.
 */
void cc2420_fifo_get(uint8_t* data, uint16_t data_length);

/* Interrupts & IOs */
/**
 * Register a callback function to be called when an interrupt
 * happens on the FIFO pin.
 * \param cb a callback function pointer.
 */
void cc2420_io_fifo_register_cb(cc2420_cb_t cb);
/**
 * Register a callback function to be called when an interrupt
 * happens on the FIFOP pin.
 * \param cb a callback function pointer.
 */
void cc2420_io_fifop_register_cb(cc2420_cb_t cb);
/**
 * Register a callback function to be called when an interrupt
 * happens on the SFD pin.
 * \param cb a callback function pointer.
 */
void cc2420_io_sfd_register_cb(cc2420_cb_t cb);
/**
 * Register a callback function to be called when an interrupt
 * happens on the CCA pin.
 * \param cb a callback function pointer.
 */
void cc2420_io_cca_register_cb(cc2420_cb_t cb);


#define cc2420_io_fifo_int_enable() FIFO_INT_ENABLE()
#define cc2420_io_fifo_int_disable() FIFO_INT_DISABLE()
#define cc2420_io_fifo_int_clear() FIFO_INT_CLEAR()
#define cc2420_io_fifo_int_set_falling() FIFO_INT_SET_FALLING()
#define cc2420_io_fifo_int_set_rising() FIFO_INT_SET_RISING()
#define cc2420_io_fifo_read() FIFO_READ()

#define cc2420_io_fifop_int_enable() FIFOP_INT_ENABLE()
#define cc2420_io_fifop_int_disable() FIFOP_INT_DISABLE()
#define cc2420_io_fifop_int_clear() FIFOP_INT_CLEAR()
#define cc2420_io_fifop_int_set_falling() FIFOP_INT_SET_FALLING()
#define cc2420_io_fifop_int_set_rising() FIFOP_INT_SET_RISING()
#define cc2420_io_fifop_read() FIFOP_READ()

#define cc2420_io_sfd_int_enable() SFD_INT_ENABLE()
#define cc2420_io_sfd_int_disable() SFD_INT_DISABLE()
#define cc2420_io_sfd_int_clear() SFD_INT_CLEAR()
#define cc2420_io_sfd_int_set_falling() SFD_INT_SET_FALLING()
#define cc2420_io_sfd_int_set_rising() SFD_INT_SET_RISING()
#define cc2420_io_sfd_read() SFD_READ()

#define cc2420_io_cca_int_enable() CCA_INT_ENABLE()
#define cc2420_io_cca_int_disable() CCA_INT_DISABLE()
#define cc2420_io_cca_int_clear() CCA_INT_CLEAR()
#define cc2420_io_cca_int_set_falling() CCA_INT_SET_FALLING()
#define cc2420_io_cca_int_set_rising() CCA_INT_SET_RISING()
#define cc2420_io_cca_read() CCA_READ()

/* Other */
void micro_delay(unsigned int n);

#endif

/**
 * @}
 */
