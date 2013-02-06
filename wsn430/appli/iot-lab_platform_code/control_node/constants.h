/*
 * Copyright  2008-2009 INRIA/SensLab
 * 
 * <dev-team@senslab.info>
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

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

/* FRAME */
// first byte
#define SYNC_BYTE 0x80

// type byte
enum frameType {
	OPENNODE_START = 0x15,
	OPENNODE_STARTBATTERY = 0x16,
	OPENNODE_STOP = 0x17,

	GET_RSSI = 0x30,
	GET_TEMPERATURE = 0x31,
	GET_LUMINOSITY = 0x32,

	GET_BATTERY_VOLTAGE = 0x33,
	GET_BATTERY_CURRENT = 0x34,
	GET_BATTERY_POWER = 0x35,
	GET_DC_VOLTAGE = 0x36,
	GET_DC_CURRENT = 0x37,
	GET_DC_POWER = 0x38,
	GET_DIFF_CURRENT = 0x39,
	GET_DIFF_POWER = 0x3A,

	CONFIG_CC1101 = 0x40,
	CONFIG_CC2420 = 0x41,

	CONFIG_POWERPOLL = 0x42,
	CONFIG_SENSORPOLL = 0x43,
	CONFIG_RADIOPOLL = 0x44,

	CONFIG_RADIONOISE = 0x45,

	SET_DAC0 = 0x50,
	SET_DAC1 = 0x51,
	SET_TIME = 0x52,

};

enum responseType {
	ACK = 0x1, NACK = 0x2, POLL_DATA = 0xFF
};

#define FRAME_LENGTH_MAX 33
#define POLL_LENGTH_MAX 255

/* Typedefs */
typedef union {
	struct {
		uint8_t sync;
		uint8_t len;
		uint8_t type;
		uint8_t payload[FRAME_LENGTH_MAX - 1];
	};
	uint8_t data[FRAME_LENGTH_MAX + 2];
} xRXFrame_t;

typedef union {
	struct {
		uint8_t sync;
		uint8_t len;
		uint8_t type;
		uint8_t ack;
		uint8_t payload[FRAME_LENGTH_MAX - 2];
	};
	uint8_t data[FRAME_LENGTH_MAX + 2];
} xTXFrame_t;

#define POLLING_MEASURE_MAX 123
#define POLLING_TIME_MAX 5 // 5s
typedef struct {
	union {
		struct {
			uint8_t sync, len, type, sensors, count, period[2], starttime[4],
					measures[POLLING_MEASURE_MAX][2];
		};
		uint8_t data[2 * POLLING_MEASURE_MAX + 11];

	};
	int index;

} xPollingFrame_t;

#endif
