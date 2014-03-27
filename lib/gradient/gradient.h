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

#define CHANNEL 4
#define PERIOD 8192

#define BEACON_T 16
#define HELLO_T 42
#define REPORT_T 34

#define ST_OFFSET(st, field)\
  ( (uint16_t ) &((st *)(0))->field )

typedef union {
	struct {
		uint8_t type;
		uint8_t seqnum[2];
		uint8_t hop;
		uint8_t crc;
	};
	uint8_t raw[1];
} beacon_t;


/* Change the structure of the hello message to add data you want to be transmited */
typedef union {
	struct {
		uint8_t type;
		uint8_t seqnum[2];
		uint8_t crc;
	};
	uint8_t raw[1];
} hello_t;

typedef union {
  struct {
    uint8_t type;
    uint8_t mobile_id[2];
    uint8_t anchor_id[2];
    uint8_t seqnum[2];
    int8_t rssi;
    uint8_t crc;
  };
  uint8_t raw[1];
} report_t;

uint16_t char_rx(uint8_t c);
uint16_t send_packet(void);

uint16_t packet_sent(void);
void packet_received(uint16_t src_addr, uint8_t* data, uint16_t length,
		int8_t rssi);
uint16_t my_send(uint16_t dest_addr, uint8_t* data, uint16_t length);
