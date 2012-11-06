/*
 * radio.c
 *
 *  Created on: Mar 15, 2010
 *      Author: burindes
 */
#include <io.h>
#include "radio.h"
#include "cc1100.h"
#include "timerA.h"
#include "crc8.h"

/* Prototypes */
static uint16_t eop_int(void);
static void radio_send(uint16_t burstid, uint16_t pktid, uint8_t data);

/* Variables */
static uint16_t node_id;
static radio_callback_t radio_cb = 0;
static struct {
	uint8_t len;
	uint8_t crc;
	uint8_t chk[2];
	uint8_t src[2];
	uint8_t burstid[2];
	uint8_t pktid[2];
	uint8_t data[60];
} packet;

#define PACKET_LENGTH_MIN 9
#define PACKET_LENGTH_MAX 60
#define PACKET_TOTAL_LENGTH_MIN (PACKET_LENGTH_MIN + 3)
#define PACKET_TOTAL_LENGTH_MAX (PACKET_LENGTH_MAX + 3)

#define CHK0 0xAA
#define CHK1 0x55

void radio_init() {
	node_id = 0;

	// Initialize the radio to standard parameters
	cc1100_init();

	// Configure general registers
	cc1100_cfg_append_status(CC1100_APPEND_STATUS_ENABLE);
	cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
	cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
	cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
	cc1100_cfg_freq_if(0x12);
	cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);
	cc1100_cfg_mod_format(CC1100_MODULATION_MSK);
	cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);
	cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);
	cc1100_cfg_rxoff_mode(CC1100_RXOFF_MODE_STAY_RX);
	cc1100_cfg_txoff_mode(CC1100_TXOFF_MODE_IDLE);
	cc1100_cfg_pa_power(0);

	// Radio configuration
	radio_set_freq(0x20, 0x28, 0xC5);
	radio_set_chanbw(0, 2);
	radio_set_drate(13, 47);
	//radio_set_txpower(0xC2); // 10dBm
	//radio_set_txpower(0x50); // 0dBm
	radio_set_txpower(0x27); // -10 dBm
	//radio_set_txpower(0x0F); // -20dBm
	//radio_set_txpower(0x03); // -30dBm
	radio_set_modulation(MOD_MSK);

	// Configure interruptions
	cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
	cc1100_cfg_gdo2(CC1100_GDOx_SYNC_WORD);
	cc1100_gdo0_register_callback(eop_int);
	cc1100_gdo2_int_disable();
	cc1100_gdo0_int_set_falling_edge();
	cc1100_gdo0_int_clear();
	cc1100_gdo0_int_enable();

	uint16_t i;
	for (i = 0; i < sizeof(packet.data); i++)
		packet.data[i] = (0xAA ^ (0xFF - i));

	// Set RX
	radio_reset_rx();

	timerA_init();
}

void radio_set_id(uint16_t id) {
	node_id = id;
}

uint16_t radio_get_id(void) {
	return node_id;
}

void radio_reset_rx() {
	cc1100_gdo0_int_disable();

	cc1100_cmd_idle();
	cc1100_cmd_flush_rx();
	cc1100_cmd_flush_tx();
	cc1100_cmd_calibrate();

	cc1100_gdo0_int_clear();
	cc1100_gdo0_int_enable();
	cc1100_cmd_rx();
}

void radio_set_freq(uint8_t freq2, uint8_t freq1, uint8_t freq0) {
	cc1100_cmd_idle();
	cc1100_write_reg(CC1100_REG_FREQ2, freq2);
	cc1100_write_reg(CC1100_REG_FREQ1, freq1);
	cc1100_write_reg(CC1100_REG_FREQ0, freq0);

	radio_reset_rx();
}

void radio_set_chanbw(uint8_t chanbw_e, uint8_t chanbw_m) {
	cc1100_cmd_idle();
	cc1100_cfg_chanbw_e(chanbw_e);
	cc1100_cfg_chanbw_m(chanbw_m);

	radio_reset_rx();
}
void radio_set_drate(uint8_t drate_e, uint8_t drate_m) {
	cc1100_cmd_idle();
	cc1100_cfg_drate_e(drate_e);
	cc1100_cfg_drate_m(drate_m);

	radio_reset_rx();
}

void radio_set_txpower(uint8_t pow) {
	cc1100_cmd_idle();
	uint8_t table = pow;
	cc1100_cfg_patable(&table, 1);

	radio_reset_rx();
}
void radio_set_modulation(radio_modulation_t mod) {
	cc1100_cmd_idle();
	switch (mod) {
	case MOD_2FSK:
		cc1100_cfg_mod_format(CC1100_MODULATION_2FSK);
		break;
	case MOD_ASK:
		cc1100_cfg_mod_format(CC1100_MODULATION_ASK);
		break;
	case MOD_GFSK:
		cc1100_cfg_mod_format(CC1100_MODULATION_GFSK);
		break;
	case MOD_MSK:
		cc1100_cfg_mod_format(CC1100_MODULATION_MSK);
		break;
	}

	radio_reset_rx();
}

void radio_send_burst(uint16_t burstid, uint16_t burstlen, uint16_t period,
		uint8_t paylen) {
	uint16_t count;
	uint16_t now;
	TAR = 0;

	// flush radio
	cc1100_gdo0_int_disable();
	cc1100_cmd_idle();
	cc1100_cmd_flush_tx();
	cc1100_cmd_calibrate();
	cc1100_gdo0_int_clear();

	// Start the timer @ 1024Hz
	timerA_start_ACLK_div(TIMERA_DIV_4);

	for (count = 0; count < burstlen; count++) {
		// record time
		now = timerA_time();
		// send packet
		radio_send(burstid, count, paylen);

		// WAIT for period
		while ((timerA_time() - now) < period)
			;
	}

	radio_reset_rx();
}
static void radio_send(uint16_t burstid, uint16_t pktid, uint8_t data) {
	packet.len = PACKET_LENGTH_MIN + data;
	packet.chk[0] = CHK0;
	packet.chk[1] = CHK1;
	packet.burstid[0] = (burstid >> 8);
	packet.burstid[1] = (burstid & 0xFF);
	packet.pktid[0] = (pktid >> 8);
	packet.pktid[1] = (pktid & 0xFF);
	packet.src[0] = node_id >> 8;
	packet.src[1] = node_id & 0xFF;

	packet.crc = crc8_bytes(packet.chk, packet.len - 1);

	cc1100_fifo_put((uint8_t*) &packet, packet.len + 1);
	cc1100_cmd_tx();

	while (!cc1100_gdo0_read())
		;
	while (cc1100_gdo0_read())
		;
}

void radio_set_callback(radio_callback_t cb) {
	radio_cb = cb;
}

static uint16_t eop_int(void) {
	uint8_t c = cc1100_status_rxbytes();
	uint8_t lqi, rssi, crc;
	// check FIFO length
	if ((c < PACKET_TOTAL_LENGTH_MIN) || (c > PACKET_TOTAL_LENGTH_MAX)) {
		radio_reset_rx();
		return 0;
	}

	// get packet
	cc1100_fifo_get(&packet.len, c);

	// check len field
	if ((packet.len < PACKET_LENGTH_MIN) || (packet.len > PACKET_LENGTH_MAX)) {
		radio_reset_rx();
		return 0;
	}

	// check added CRC
	crc = crc8_bytes(packet.chk, packet.len - 1);
	if (crc != packet.crc) {
		radio_reset_rx();
		return 0;
	}

	// check CHK fields
	if ((packet.chk[0] != CHK0) || (packet.chk[1] != CHK1)) {
		radio_reset_rx();
		return 0;
	}

	// check CRC
	rssi = packet.data[packet.len - PACKET_LENGTH_MIN];
	lqi = packet.data[(packet.len - PACKET_LENGTH_MIN) + 1];
	if (!(lqi & 0x80)) {
		radio_reset_rx();
		return 0;
	}

	// call callback if any
	if (radio_cb != 0x0) {
		radio_cb(packet.src, packet.burstid, packet.pktid, rssi);
		radio_reset_rx();
		return 1;
	}

	radio_reset_rx();
	return 0;
}
