/*
 * radio.h
 *
 *  Created on: Mar 15, 2010
 *      Author: burindes
 */

#ifndef RADIO_H_
#define RADIO_H_

/**
 * Initialize the radio with default parameters.
 */
void radio_init(void);
/**
 * Set the node ID to use.
 * @param id the id to set to this node
 */
void radio_set_id(uint16_t id);
/**
 * Get the node ID in use.
 * @return the current ID
 */
uint16_t radio_get_id(void);
/**
 * Force a RX reset.
 */
void radio_reset_rx(void);

/**
 * Set the radio frequency.
 * @param freq2 the FREQ2 register value
 * @param freq1 the FREQ1 register value
 * @param freq0 the FREQ0 register value
 */
void radio_set_freq(uint8_t freq2, uint8_t freq1, uint8_t freq0);
/**
 * Set the radio channel bandwidth.
 * @param chanbw_e the channel bandwidth exponent
 * @param chanbw_e the channel bandwidth mantissa
 */
void radio_set_chanbw(uint8_t chanbw_e, uint8_t chanbw_m);

/**
 * Set the radio datarate.
 * @param drate_e the exponent
 * @param drate_m the mantissa
 */
void radio_set_drate(uint8_t drate_e, uint8_t drate_m);

typedef enum {
	MOD_2FSK = 0, MOD_GFSK = 1, MOD_ASK = 2, MOD_MSK = 3
} radio_modulation_t;

/**
 * Set the radio modulation.
 * @param mod the modulation
 */
void radio_set_modulation(radio_modulation_t mod);

/**
 * Set the tx power.
 * @param pow the tx power parameter
 */
void radio_set_txpower(uint8_t pow);

/**
 * Send a burst of packets.
 * @param burstid the burst id
 * @param burstlen the number of packets in the burst
 * @param period the period between 2 packets
 * @param paylen the extra length to append to the packets
 */
void radio_send_burst(uint16_t burstid, uint16_t burstlen, uint16_t period, uint8_t paylen);

typedef void (*radio_callback_t)(uint8_t src[2], uint8_t burstid[2], uint8_t pktid[2], uint8_t rssi);
/**
 * Set the callback for received packet notification.
 * @param cb the callback function
 */
void radio_set_callback(radio_callback_t cb);

#endif /* RADIO_H_ */
