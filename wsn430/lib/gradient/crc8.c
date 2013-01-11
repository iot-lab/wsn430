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
	uint16_t i;
	uint8_t crc = 0;
	for (i = 0; i < len; i++) {
		crc = crc8_byte(crc, bytes[i]);
	}
	return crc;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
