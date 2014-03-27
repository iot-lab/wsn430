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

/**********************************************************************/

#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "mac.h"
#include "timerA.h"

#include "single.h"

/**********************************************************************/

uint16_t my_seqnum;
uint8_t beacon_received;

/**********************************************************************/

void init_app(void)
{
  printf("# MOBILE MODE\n");
  beacon_received = 0;
  return;
}

/**********************************************************************/

uint16_t send_packet(void) {
  static spt_hello_t pkt_hello;

  activity_schedule();
  if ( num_slot == 0 )
    beacon_received = 0;
  if ( num_slot == 2 && my_state == STATE_ACTIVE )
    {
#ifdef DEBUG
      LED_GREEN_ON();
#endif
      pkt_hello.type = SPT_HELLO;
      PRINTF("#> Sending HELLO\n");
      mac_send((uint8_t *)&pkt_hello, sizeof(pkt_hello), MAC_BROADCAST);
#ifdef DEBUG
      LED_GREEN_OFF();
#endif
    }

  return 0;
}

/**********************************************************************/

uint16_t packet_received(uint8_t packet[], uint16_t length,
			 uint16_t src_addr, int16_t rssi)
{
  spt_beacon_t *pkt=(spt_beacon_t *)packet;

#ifdef DEBUG
  LED_RED_ON();
#endif
  //notice_packet_received_mac();
  PRINTF("#> Packet from %x:\n", src_addr);
  if ( pkt->type == SPT_BEACON && my_seqnum != pkt->seqnum)
    {
      my_state = STATE_ACTIVE;
      cpt_last_beacon = 0;
      num_slot = 1;
      my_seqnum = pkt->seqnum;
      beacon_received = 1;
    }
#ifdef DEBUG
  LED_RED_OFF();
#endif

  return 0;
}

/**********************************************************************/
