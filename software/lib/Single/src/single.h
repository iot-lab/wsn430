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

#define DEBUG

#ifdef DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) 
#endif

#define CHANNEL 10

#define PERIOD 2048
//#define PERIOD 8192
//#define PERIOD 0

#define FRAME_SIZE 16
#define ACTIVE_SLOTS 4
#define FRAME_PERIOD 4

// WARNING this is from csma.c !
#define PACKET_LENGTH_MAX 58

typedef struct {
  uint16_t id_anchor;
  uint16_t time_anchor;
  uint16_t id_mobile;
  int rssi_dbm;
} mobile_info_t;

/* SINGLE PACKET TYPES */

#define SPT_BEACON 42
#define SPT_REPORT 43
#define SPT_ACKRPT 34
#define SPT_HELLO  74

typedef struct {
  uint8_t type;
  uint8_t flag;
  uint16_t seqnum;
  uint8_t nbhops;
} spt_beacon_t;

typedef struct {
  uint8_t type;
  uint8_t size;
  uint16_t seqnum;
  uint16_t id_source;
  uint16_t nbh_source;
  uint16_t id_dest;
  uint8_t ttl;
  mobile_info_t info;
} spt_report_t;

typedef struct {
  uint8_t type;
  uint8_t rfu;
  uint16_t cpt;
  uint8_t bourinator[12]; /* to ensure sufficient length for RSSI evaluation */
} spt_hello_t;

#define UNKNOWN_NB_HOPS 255
#define DEFAULT_TTL 20

/**********************************************************************/

#define STATE_INIT    0
#define STATE_DEFAULT 1
#define STATE_ACTIVE  2

extern uint8_t my_state;
extern uint16_t num_frame;
extern uint16_t num_slot;

extern uint16_t cpt_last_beacon;

/**********************************************************************/

#define THRESHOLD_BEACON_MOBILE 3
#define THRESHOLD_BEACON_ANCHOR 5

/**********************************************************************/

#define START_OF_TIME(x) ((~x & 0xc000) ? 1 : 0)
#define END_OF_TIME(x)   ((x & 0xc000) ? 1 : 0)

#define TIME_CMP(x,y)						\
  (START_OF_TIME(x) && END_OF_TIME(y) ? 0xffff-y+x :		\
   (END_OF_TIME(x) && START_OF_TIME(y) ? -(0xffff-x+y) : x-y ))

/**********************************************************************/

#define BLANK_THRESHOLD 2

void synchronize_mac(void);
void notice_packet_received_mac(void);

/**********************************************************************/

uint16_t char_rx(uint8_t c);

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
uint16_t send_packet(void);

void init_app(void);

#define PKT_BUFFER_SIZE 4
#define NO_PACKET 200 
/* have to be higher than PKT_BUFFER_SIZE and 255 at maximum */

#define BUFFER_OFF 0
#define BUFFER_ON  1

extern uint8_t buffer_state;

void init_buffer(uint8_t mode);
uint16_t my_send(uint8_t packet[], uint16_t length, uint16_t dst_addr);
uint16_t try_next_in_buffer(void);
uint16_t packet_error(void);
uint16_t packet_sent(void);

void init_app(void);
void radio_on(void);
void radio_off(void);

/**********************************************************************/
uint16_t activity_schedule(void);

