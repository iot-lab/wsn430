/**
 *  \file   crc8.c
 *  \brief  crc8
 *  \author Antoine Fraboulet
 *  \date   2009
 **/

#include "stdint.h"
#include "crc8.h"

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/*
 * CRC  x8 + x5 + x4 + 1
 *
 * Numerical Recipies in C : the art of scientific computing
 * ch 20.3 Cyclic Redundancy and Other Checksums (page 896)
 *
 */

static uint8_t crc8_byte(uint8_t crc, uint8_t byte) {
	int i;
	crc ^= byte;
	for (i = 0; i < 8; i++) {
		if (crc & 1)
			crc = (crc >> 1) ^ 0x8c;
		else
			crc >>= 1;
	}
	return crc;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

uint8_t crc8_bytes(uint8_t* bytes, uint16_t len) {
	int i;
	uint8_t crc = 0;
	for (i = 0; i < len; i++) {
		crc = crc8_byte(crc, bytes[i]);
	}
	return crc;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
