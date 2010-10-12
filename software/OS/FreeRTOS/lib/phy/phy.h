/*
 * phy.h
 *
 *  Created on: Sep 27, 2010
 *      Author: burindes
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

/**
 * Initialize the PHY layer.
 * \param callback the function pointer to call for each received frame.
 */
void phy_init(xSemaphoreHandle spi_m, phy_rx_callback_t callback,
		uint8_t channel);
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
