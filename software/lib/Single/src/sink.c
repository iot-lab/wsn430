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
uint16_t my_clock;

/**********************************************************************/

void init_app(void)
{
  printf("# SINK MODE\n");
  my_seqnum = 0;
  my_clock = 0;
}

uint16_t send_packet(void) {
  static spt_beacon_t pkt;

  synchronize_mac();
  LED_GREEN_TOGGLE();

  num_slot++;
  if ( num_slot >= FRAME_SIZE )
    num_slot = 0;

  PRINTF("#> slot %u\r\n", num_slot);

  if ( num_slot == 1 )
    {
      pkt.type = SPT_BEACON;
      my_seqnum++;
      pkt.seqnum = my_seqnum;
      pkt.nbhops = 0;
      PRINTF("#> sending BEACON\n");
      
      mac_send((uint8_t *)&pkt, sizeof(pkt), MAC_BROADCAST);
      /* note that we do not use my_send since the sink sends only few
	 packets */
    }

  my_clock++;
  
  return 0;
}

/**********************************************************************/

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi)
{
  spt_report_t *pkt_report=(spt_report_t *)packet;

  LED_RED_ON();
  notice_packet_received_mac();

  PRINTF("#> packet from %x, type %d\r\n", src_addr, pkt_report->type);
  if ( pkt_report->type == SPT_REPORT )
    {
      printf("%5u %5u %04x %04x %5u %04x %4d %4d\n" , my_clock, timerA_time(),
	     src_addr,
	     pkt_report->info.id_anchor,
	     pkt_report->info.time_anchor,
	     pkt_report->info.id_mobile,
	     pkt_report->info.rssi_dbm,
	     pkt_report->nbh_source);
    }
  LED_RED_OFF();

  return 0;
}

/**********************************************************************/

