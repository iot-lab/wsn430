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


#ifndef PHY_H_
#define PHY_H_

/**
 * Callback function prototype for received frames
 * \param data pointer to the received data
 * \param length number of data bytes received
 * \param rssi rssi in dBm of the received data
 * \param time the timerA time of the received frame
 */
typedef void (*phy_rx_callback_t)(uint8_t * data, uint16_t length, int8_t rssi,
		uint16_t time);

enum phy_tx_power {
	PHY_TX_20dBm, PHY_TX_10dBm, PHY_TX_5dBm, PHY_TX_0dBm
};

/**
 * Initialize the PHY layer.
 * \param callback the function pointer to call for each received frame.
 * \param channel the radio channel to use.
 * \param power the TX power to use.
 */
void phy_init(xSemaphoreHandle spi_m, phy_rx_callback_t callback,
		uint8_t channel, enum phy_tx_power power);
/**
 * Enter RX at the PHY layer. The PHY RX callback will be called upon frame RX
 */
void phy_rx(void);
/**
 * Stop TX and RX, interrupt any transfer.
 */
void phy_idle(void);
/**
 * Send data immediately, interrupting any running transfer.
 * \param data a pointer to the data to send;
 * \param length the number of bytes to send, max 125;
 * \param timestamp pointer to a variable for storing the timerA time of TX.
 * \return 1 if frame was sent, 0 if not.
 */
uint16_t phy_send(uint8_t* data, uint16_t length, uint16_t *timestamp);
/**
 * Send data immediately if the radio channel is clear,
 * interrupting any running transfer.
 * \param data a pointer to the data to send;
 * \param length the number of bytes to send;
 * \param timestamp pointer to a variable for storing the timerA time of TX.
 * \return 1 if frame was sent, 0 if it was not
 */
uint16_t phy_send_cca(uint8_t* data, uint16_t length, uint16_t *timestamp);

#endif /* PHY_H_ */
