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
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, rimeaddr_t *sender)
{
  printf("broadcast message received from %u.%u: '%s'\n", \
        sender->u8[0], sender->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 128, &broadcast_call);

//  while(1) {
    if (rimeaddr_node_addr.u8[0] == 249 && rimeaddr_node_addr.u8[1] == 178) {
		static struct etimer et;

		etimer_set(&et, 10*CLOCK_SECOND);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		char msg[] = "Hello Broadcast";

		packetbuf_copyfrom(msg, sizeof(msg));
		broadcast_send(&broadcast);
		printf("broadcast message sent\n");
    }
//  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
