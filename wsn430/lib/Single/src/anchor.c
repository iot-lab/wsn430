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
uint16_t my_parent;
uint8_t new_seqnum;
uint8_t my_nbhops;

/**********************************************************************/

void init_app(void)
{
  printf("# ANCHOR MODE\n");
  my_seqnum = 0;
  new_seqnum = 0;
  init_buffer(BUFFER_ON);
}

uint16_t send_packet(void) {
  activity_schedule();
  if ( num_slot == 0 )
    new_seqnum = 0;
  return 0;
}

/**********************************************************************/

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi)
{
  spt_hello_t *pkt=(spt_hello_t *)packet;
  spt_beacon_t *pkt_beacon;
  spt_report_t *pkt_report, my_report;

  //  notice_packet_received_mac();

  PRINTF("#> packet from %x (type=%u)\r\n", src_addr, pkt->type);

  switch ( pkt->type ) {
  case SPT_HELLO: /******************************/
#ifdef DEBUG_TRACK
    LED_RED_ON();    
#endif
    if ( my_state == STATE_ACTIVE )
      {
#ifdef DEBUG
	LED_RED_ON();
#endif
	// send report
	PRINTF("#> sending REPORT (parent %4x)\n", my_parent);
	my_report.type = SPT_REPORT;
	my_report.id_source = node_addr;
	my_report.nbh_source = my_nbhops;
	my_report.id_dest = my_parent;
	my_report.ttl = DEFAULT_TTL;
	my_report.info.id_anchor = node_addr;
	my_report.info.time_anchor = timerA_time();
	my_report.info.id_mobile = src_addr;
	my_report.info.rssi_dbm = rssi;
	my_send((uint8_t *)&my_report, sizeof(spt_report_t), my_parent);
#ifdef DEBUG
	LED_RED_OFF();
#endif
      }
    break;
  case SPT_BEACON: /******************************/
    pkt_beacon = (spt_beacon_t *)packet;
    if ( my_seqnum != pkt_beacon->seqnum && !new_seqnum )
      {
	if ( my_state == STATE_DEFAULT && my_seqnum != pkt_beacon->seqnum)
	  {
	    my_seqnum = pkt_beacon->seqnum;
	    my_nbhops = pkt_beacon->nbhops+1;
	    my_parent = src_addr;
	    my_state = STATE_ACTIVE;
	    cpt_last_beacon = 0;
	  }
	else
	  if ( my_state == STATE_ACTIVE )
	    {
	      if ( src_addr == my_parent )
		{
		  my_seqnum = pkt_beacon->seqnum;
		  my_nbhops = pkt_beacon->nbhops+1;
		  cpt_last_beacon = 0;
		}
	      else
		if ( TIME_CMP(my_seqnum, pkt_beacon->seqnum) < 0 )
		  {
		    my_seqnum = pkt_beacon->seqnum;
		    my_nbhops = pkt_beacon->nbhops+1;
		    my_parent = src_addr;
		    cpt_last_beacon = 0;
		  }
	    } 
      }
    break;
  case SPT_REPORT: /******************************/
    pkt_report = (spt_report_t *)packet;
    if ( my_state == STATE_ACTIVE && pkt_report->id_dest == node_addr
	 && pkt_report->ttl != 0 )
      {
#ifdef DEBUG
	LED_GREEN_ON();
#endif
	// send the report
	PRINTF("#> relaying REPORT (from %4x to %4x)\n", src_addr, my_parent);
	pkt_report->ttl--;
	pkt_report->id_dest = my_parent;
	my_send((uint8_t *)pkt_report, sizeof(spt_report_t), my_parent);
#ifdef DEBUG
	LED_GREEN_OFF();
#endif
      }
  }

  return 0;
}

/**********************************************************************/

