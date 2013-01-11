/**
 * \file simplephy.h
 * \brief Simple PHY layer header
 */

#ifndef _SIMPLEPHY_H
#define _SIMPLEPHY_H

/**
 * PHY received frame handler function pointer.
 */
typedef void (*phy_handler_t)(uint8_t frame[], uint16_t length);

/**
 * PHY sent frame notifier function pointer.
 */
typedef void (*phy_notifier_t)(void);

/**
 * PHY initialization procedure.
 */
void phy_init(void);

/**
 * Send a frame over the air.
 * \param frame data array to send
 * \param length number of bytes to send (max = 255)
 * \return 1 if error encountered, 0 otherwise
 */
uint16_t phy_send_frame(uint8_t frame[], uint16_t length);

/**
 * Start radio RX.
 * \return 1 if error encountered, 0 otherwise
 */
 uint16_t phy_start_rx(void);
 
/**
 * Stop radio RX.
 */
uint16_t phy_stop_rx(void);


/**
 * Do a CCA.
 * Duration is 485µs if not in RX, 56µs else.
 * \return 1 if channel is clear, 0 if busy
 */
uint16_t phy_cca(void);

/** 
 * Do an RSSI measurment.
 * Duration is 485µs if not in RX, 65µs else.
 * \return raw rssi value as given by CC1100
 */
uint8_t phy_rssi(void);

/**
 * Register a function pointer that'll be called if a frame is received.
 * \param cb function pointer
 */
void phy_register_frame_received_handler(phy_handler_t cb);

/**
 * Register a function pointer that'll be called when a frame has been sent.
 * \param cb function pointer
 */
void phy_register_frame_sent_notifier(phy_notifier_t cb);

#endif
