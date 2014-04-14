/* General includes */
#include <io.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* Drivers includes */
#include "clock.h"
#include "uart0.h"
#include "leds.h"

/* Project includes */
#include "radio.h"
#include "crc8.h"

/**
 * Command Frame
 * ----------------------------------------
 * | sync | cmd | arglen | args ... | crc |
 * ----------------------------------------
 *
 * Response Frame
 * ------------------------------------
 * | sync | rsp | arglen | args | crc |
 * ------------------------------------
 *
 * Ack Frame (Response)
 * ---------------------------------
 * | sync | rsp | 1 | status | crc |
 * ---------------------------------
 *
 * Packet Frame (Response)
 * --------------------------------------------------------------
 * | sync | PKT | 7 | src(2) | burst(2) | pktid(2) | rssi | crc |
 * ----- --------------------------------------------------------
 *
 * Commands:
 * - SetID  :   | sync | 0xC0 | 2 | id(2)
 * - CmpID  :   | sync | 0xC1 | 2 | id(2)
 * - ResetRX:   | sync | 0xC2 | 0
 * - SetTXPow:  | sync | 0xC7 | 1 | reg
 * - SendBurst: | sync | 0xC8 | 7 | burstid(2) | burstlen(2) | period(2) | paylen
 *
 * Notifications:
 *  - Booted : | sync | 0xF0 | 0
 *  - PktRx  : | sync | 0xFF | 7 | src(2) | burst(2) | pktid(2) | rssi
 *
 */

#define SYNC_BYTE 0x80

#define PKT_BYTE 0xFF
#define BOOT_BYTE 0xF0
#define ACK_OK  0x0
#define ACK_NOK 0x1
#define ACK_NOID 0x2
#define ACK_BADCRC 0x3

#define ACK_ARG_LEN 1
#define PKT_ARG_LEN 7

#define CMD_SETID_TYPE 0xC0
#define CMD_SETID_ARGLEN 2

#define CMD_CMPID_TYPE 0xC1
#define CMD_CMPID_ARGLEN 2

#define CMD_RESETRX_TYPE 0xC2
#define CMD_RESETRX_ARGLEN 0

#define CMD_SETFREQ_TYPE 0xC3
#define CMD_SETFREQ_ARGLEN 3

#define CMD_SETCHANBW_TYPE 0xC4
#define CMD_SETCHANBW_ARGLEN 2

#define CMD_SETDRATE_TYPE 0xC5
#define CMD_SETDRATE_ARGLEN 2

#define CMD_SETMOD_TYPE 0xC6
#define CMD_SETMOD_ARGLEN 1

#define CMD_SETTXPOW_TYPE 0xC7
#define CMD_SETTXPOW_ARGLEN 1

#define CMD_SEND_TYPE 0xC8
#define CMD_SEND_ARGLEN 7

#define PACKET_TYPE 0xFF

/* Prototypes */
/**
 * Initialize the hardware
 */
void hw_init(void);

/**
 * Parse a received command from the serial link
 * The command is stored in a global buffer
 */
void parse_command(void);
/**
 * Handle a received char from the serial link (callback)
 * @param c the received char
 * @return the same char
 */
uint16_t char_rx(uint8_t c);
/**
 * Handle a received Radio Packet (callback)
 * @param src the sender
 * @param
 */
void packet_rx(uint8_t src[2], uint8_t burstid[2], uint8_t pktid[2],
		uint8_t rssi);

/**
 * Send a response packet on the serial link.
 */
void send_resp(void);
/**
 * Notify of a received packet on the serial link
 */
void notify_packet(void);

/**
 * Notify the node has booted
 */
void notify_boot(void);
/**
 * Send an ACK after a received command.
 * @param cmd the cmd to ACK
 * @param ack the ACK value
 */
void send_ack(uint8_t cmd_byte, uint8_t ack);

/**
 * Flags indicating the reception of a command frame or a packet
 */
volatile int16_t flag_cmd, flag_pkt;
/**
 * Latest received command
 */
struct {
	uint8_t type, arglen, args[10];
} cmd;

struct {
	uint8_t resp, arglen, args[10];
} resp;
/**
 * Latest received packet
 */
struct {
	uint8_t src0, src1;
	uint8_t burstid0, burstid1;
	uint8_t pktid0, pktid1;
	uint8_t rssi;
} pkt;
/**
 * Putchar for printf
 */
int putchar(int c) {
	uart0_putchar(c);
	return c;
}

uint16_t loc_id = 0;
/**
 * The main function.
 */
int main(void) {
	// Stop the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

	// Configure the IO ports
	hw_init();

	// Setup MCLK 8MHz and SMCLK 8MHz
	set_mcu_speed_xt2_mclk_8MHz_smclk_8MHz();
	// Set ACLK @ 4096Hz
	set_aclk_div(8);

	// Initialize the LEDs
	LEDS_INIT();
	LEDS_OFF();
	LED_BLUE_ON();

	// Configure the uart
	uart0_init(UART0_CONFIG_8MHZ_115200);
	uart0_register_callback(char_rx);
	eint();

	// Start the radio
	radio_init();
	radio_set_callback(packet_rx);

	// Clear the flags
	flag_cmd = 0;
	flag_pkt = 0;

	// Say BOOTed
	notify_boot();

	// Enter an infinite loop
	while (1) {
		// Sleep a little
		LPM0;

		// check for a received command
		if (flag_cmd) {
			// clear the flag
			flag_cmd = 0;

			// parse the received command
			LED_GREEN_TOGGLE();
			parse_command();
		}

		// check for a received packet
		if (flag_pkt) {
			// clear the flag
			flag_pkt = 0;

			// print the received packet
			LED_RED_TOGGLE();
			notify_packet();
		}
	}

	return 0;
}

uint16_t char_rx(uint8_t c) {
	static uint8_t buffer[15];
	static int16_t buf_ix = 0;

	LED_BLUE_TOGGLE();
	// depending on buffer index
	if (buf_ix == 0) {
		// check start
		if (c == SYNC_BYTE) {
			buffer[0] = SYNC_BYTE;
			buf_ix = 1;
		}
	} else if (buf_ix == 1) {
		buffer[1] = c;
		buf_ix = 2;
	} else if (buf_ix == 2) {
		// check length
		if (c > 10) {
			// too long, abort
			buf_ix = 0;
			return 0;
		}
		// store length
		buffer[2] = c;
		buf_ix++;

	} else {
		// store data
		buffer[buf_ix] = c;
		buf_ix++;

		// check if frame is complete
		if ((buf_ix - 4) == buffer[2]) {
			// frame is ready, copy to buffer if flag is clear, otherwise drop
			if (!flag_cmd) {
				memcpy(&cmd, &buffer[1], buffer[2] + 3);
				flag_cmd = 1;
			}
			buf_ix = 0;
			return 1;
		}
	}

	return 0;
}

void send_resp(void) {
	uint8_t crc;
	int i;

	// Compute CRC
	crc = crc8_bytes(&resp.resp, 2 + resp.arglen);

	uart0_putchar(SYNC_BYTE);
	uart0_putchar(resp.resp);
	uart0_putchar(resp.arglen);

	for (i = 0; i < resp.arglen; i++) {
		uart0_putchar(resp.args[i]);
	}

	uart0_putchar(crc);
}

void notify_packet(void) {
	resp.resp = PKT_BYTE;
	resp.arglen = PKT_ARG_LEN;

	resp.args[0] = pkt.src0;
	resp.args[1] = pkt.src1;
	resp.args[2] = pkt.burstid0;
	resp.args[3] = pkt.burstid1;
	resp.args[4] = pkt.pktid0;
	resp.args[5] = pkt.pktid1;
	resp.args[6] = pkt.rssi;

	send_resp();
}

void notify_boot() {
	resp.resp = BOOT_BYTE;
	resp.arglen = 0;

	send_resp();
}

void send_ack(uint8_t cmd_byte, uint8_t ack) {
	resp.resp = cmd_byte;
	resp.arglen = ACK_ARG_LEN;
	resp.args[0] = ack;

	send_resp();
}

void packet_rx(uint8_t src[2], uint8_t burstid[2], uint8_t pktid[2],
		uint8_t rssi) {
	// if flag already set, drop the packet
	if (flag_pkt) {
		return;
	}

	pkt.src0 = src[0];
	pkt.src1 = src[1];
	pkt.burstid0 = burstid[0];
	pkt.burstid1 = burstid[1];
	pkt.pktid0 = pktid[0];
	pkt.pktid1 = pktid[1];
	pkt.rssi = rssi;

	flag_pkt = 1;
}

void parse_command() {
	uint16_t id;
	uint16_t burstid, burstlen, burstperiod;
	uint8_t burstdata, crc;

	// Check CRC of received frame
	crc = crc8_bytes(&cmd.type, 2 + cmd.arglen);
	if (crc != cmd.args[cmd.arglen]) {
		// CRC is bad !
		send_ack(cmd.type, ACK_BADCRC);
		return;
	}

	switch (cmd.type) {
	case CMD_SETID_TYPE:
		if (cmd.arglen != CMD_SETID_ARGLEN)
			break;
		id = cmd.args[0];
		id <<= 8;
		id += cmd.args[1];
		loc_id = id;
		radio_set_id(id);
		send_ack(cmd.type, ACK_OK);
		return;

	case CMD_CMPID_TYPE:
		if (cmd.arglen != CMD_CMPID_ARGLEN)
			break;
		id = cmd.args[0];
		id <<= 8;
		id += cmd.args[1];
		if (id == radio_get_id()) {
			send_ack(cmd.type, ACK_OK);
			return;
		}
		break;

	case CMD_RESETRX_TYPE:
		if (cmd.arglen != CMD_RESETRX_ARGLEN)
			break;
		radio_reset_rx();
		send_ack(cmd.type, ACK_OK);
		return;
	case CMD_SETFREQ_TYPE:
		if (cmd.arglen != CMD_SETFREQ_ARGLEN)
			break;
		radio_set_freq(cmd.args[0], cmd.args[1], cmd.args[2]);
		send_ack(cmd.type, ACK_OK);
		return;
	case CMD_SETCHANBW_TYPE:
		if (cmd.arglen != CMD_SETCHANBW_ARGLEN)
			break;
		radio_set_chanbw(cmd.args[0], cmd.args[1]);
		send_ack(cmd.type, ACK_OK);
		return;
	case CMD_SETDRATE_TYPE:
		if (cmd.arglen != CMD_SETDRATE_ARGLEN)
			break;
		radio_set_drate(cmd.args[0], cmd.args[1]);
		send_ack(cmd.type, ACK_OK);
		return;
	case CMD_SETMOD_TYPE:
		if (cmd.arglen != CMD_SETMOD_ARGLEN)
			break;
		radio_set_modulation(cmd.args[0]);
		send_ack(cmd.type, ACK_OK);
		return;
	case CMD_SETTXPOW_TYPE:
		if (cmd.arglen != CMD_SETTXPOW_ARGLEN)
			break;
		radio_set_txpower(cmd.args[0]);
		send_ack(cmd.type, ACK_OK);
		return;
	case CMD_SEND_TYPE:
		if (cmd.arglen != CMD_SEND_ARGLEN)
			break;
		if (radio_get_id() == 0) {
			if (loc_id == 0) {
				send_ack(cmd.type, ACK_NOID);
				return;
			} else {
				radio_set_id(loc_id);
			}
		}

		// burstid(2) | burstlen(2) | period(2) | paylen
		burstid = cmd.args[0];
		burstid <<= 8;
		burstid += cmd.args[1];

		burstlen = cmd.args[2];
		burstlen <<= 8;
		burstlen += cmd.args[3];

		burstperiod = cmd.args[4];
		burstperiod <<= 8;
		burstperiod += cmd.args[5];

		burstdata = cmd.args[6];

		radio_send_burst(burstid, burstlen, burstperiod, burstdata);
		send_ack(cmd.type, ACK_OK);
		return;
	}
	send_ack(cmd.type, ACK_NOK);
}

void hw_init(void) {
	P1SEL = 0;
	P2SEL = 0;
	P3SEL = 0;
	P4SEL = 0;
	P5SEL = 0;
	P6SEL = 0;

	P1DIR = BV(0) + BV(1) + BV(2) + BV(5) + BV(6) + BV(7);
	P1OUT = 0;

	P2DIR = BV(0) + BV(1) + BV(2) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P2OUT = 0;

	P3DIR = BV(0) + BV(2) + BV(4) + BV(6) + BV(7);
	P3OUT = BV(2) + BV(4);

	P4DIR = BV(2) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P4OUT = BV(2) + BV(4);

	P5DIR = BV(0) + BV(1) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P5OUT = 0;

	P6DIR = BV(0) + BV(1) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P6OUT = 0;
}

