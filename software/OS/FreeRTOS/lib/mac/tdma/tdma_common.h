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


#ifndef TDMA_COMMON_H_
#define TDMA_COMMON_H_

#define MS_TO_TICKS(a) (uint16_t)(((uint32_t)(a) * 32768) / 1000)
#define TICKS_TO_MS(a) ((uint16_t)(((uint32_t)(a) * 1000) / 32768))

#include <tdma_userconfig.h>
#include <timerB.h>

#define MAX_PACKET_LENGTH 119
#define MAX_COORD_SEND_LENGTH 15
#define MAX_BEACON_DATA_LENGTH 55

#ifndef SLOT_COUNT
#define SLOT_COUNT 5 // For (SLOT_COUNT-1) nodes!, total period = (SLOT_COUNT+1) slots
#endif

#ifndef SLOT_TIME_MS
#define SLOT_TIME_MS 15
#endif

#ifndef BEACON_LOSS_MAX
#define BEACON_LOSS_MAX 10
#endif

#ifndef RADIO_CHANNEL
#define RADIO_CHANNEL 4
#endif

#ifndef RADIO_POWER
#define RADIO_POWER PHY_TX_0dBm
#endif

#ifndef MAC_TX_QUEUE_LENGTH
#define MAC_TX_QUEUE_LENGTH 8
#endif

#define FRAME_HEADER_LENGTH 5

enum mac_alarm {
	ALARM_BEACON = TIMERB_ALARM_CCR0,
	ALARM_SLOT = TIMERB_ALARM_CCR1,
	ALARM_TIMEOUT = TIMERB_ALARM_CCR2
};

static inline void hton_s(uint16_t s, uint8_t* d) {
	d[0] = s >> 8;
	d[1] = s & 0xFF;
}
static inline uint16_t ntoh_s(uint8_t* a) {
	return (((uint16_t) a[0]) << 8) + a[1];
}

enum mac_internal_event {
	EVENT_TIMEOUT = 0x01,
	EVENT_RX = 0x02,
	EVENT_ASSOCIATE_REQ = 0x04,
	EVENT_DISSOCIATE_REQ = 0x08,
	EVENT_BEACON_TIME = 0x10,
	EVENT_SLOT_TIME = 0x20
};

enum mac_timing {
	TIME_SLOT = MS_TO_TICKS(SLOT_TIME_MS),
	TIME_GUARD = 100,
	TIME_INTERPACKET = 5
};

enum mac_frame_type {
	FRAME_TYPE_BEACON = 0x1, FRAME_TYPE_MGT = 0x2, FRAME_TYPE_DATA = 0x3
};

enum mac_mgt_value {
	MGT_ASSOCIATE = 0x1, MGT_DISSOCIATE = 0x2, MGT_DATA = 0x3
};
#define MGT_TYPE_MASK 0x0F
#define MGT_LENGTH_MASK  0xF0

enum mac_state {
	STATE_IDLE = 0,
	STATE_BEACON_SEARCH = 1,
	STATE_ASSOCIATING = 2,
	STATE_ASSOCIATED = 3,
	STATE_WAITING = 4
};

typedef union {
	uint16_t _align;
	struct {
		uint8_t dstAddr[2];
		uint8_t srcAddr[2];
		uint8_t type;
		uint8_t data[MAX_PACKET_LENGTH];
		uint8_t length;
	};
	uint8_t raw[FRAME_HEADER_LENGTH + MAX_PACKET_LENGTH];
} frame_t;

typedef union {
	uint16_t _align;
	struct {
		uint8_t dstAddr[2];
		uint8_t srcAddr[2];
		uint8_t type;
		uint8_t beacon_id;
		uint8_t beacon_data[MAX_BEACON_DATA_LENGTH];
		uint8_t length;
	};
	uint8_t raw[FRAME_HEADER_LENGTH + 1 + MAX_BEACON_DATA_LENGTH];
} beacon_t;

#endif /* TDMA_COMMON_H_ */
