/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Clément Burin des Roziers
 */

#include "contiki.h"
#include "net/rime.h"

#include "dev/leds.h"

#include <stdio.h>
extern process_event_t serial_line_event_message;
/*---------------------------------------------------------------------------*/
PROCESS(example_netflood_process, "NetFlood example");
AUTOSTART_PROCESSES(&example_netflood_process);
/*---------------------------------------------------------------------------*/
static int netflood_recv(struct netflood_conn *c, rimeaddr_t *from, rimeaddr_t *originator, uint8_t seqno, uint8_t hops);
static void netflood_sent(struct netflood_conn *c);
static void netflood_dropped(struct netflood_conn *c);
/*---------------------------------------------------------------------------*/
static const struct netflood_callbacks netflood_call = {netflood_recv, netflood_sent, netflood_dropped};
static struct netflood_conn netflood;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_netflood_process, ev, data)
{
  static int pktcount = 0;
  PROCESS_EXITHANDLER(netflood_close(&netflood);)

  PROCESS_BEGIN();

  netflood_open(&netflood, CLOCK_SECOND*2, 128, &netflood_call);
  
  while(1) {
    
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
      packetbuf_copyfrom("Test!", sizeof("Test!"));
      netflood_send(&netflood, (uint8_t)pktcount);
      pktcount++;
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

static int netflood_recv(struct netflood_conn *c, rimeaddr_t *from, rimeaddr_t *originator, uint8_t seqno, uint8_t hops) {
  printf("NetFlood packet received from %u.%u through %u.%u, seqno=%u, hops=%u\n",\
        originator->u8[0], originator->u8[1], from->u8[0], from->u8[1], seqno, hops);
  
  if (originator->u8[0] == 206 && originator->u8[1] == 178) {
    // emitter is senslab node 1
    printf("signal received, lighting LEDS !!!\n");
    leds_on(LEDS_ALL);
  }
  
  // return 1 means do forward
  return 1;
}
static void netflood_sent(struct netflood_conn *c) {
  printf("NetFlood packet sent !\n");
}
static void netflood_dropped(struct netflood_conn *c) {
  printf("NetFlood packet dropped !\n");
}

