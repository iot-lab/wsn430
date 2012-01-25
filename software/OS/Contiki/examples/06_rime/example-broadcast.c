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

#include "common-config.h"

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *sender)
{
	printf("broadcast message received from %u.%u: '%.*s'\n",
	       sender->u8[0], sender->u8[1],
	       packetbuf_datalen(), (char *)packetbuf_dataptr());
}

static const struct broadcast_callbacks broadcast_call = { broadcast_recv };

static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&broadcast));
	PROCESS_BEGIN();

	broadcast_open(&broadcast, 128, &broadcast_call);

	/* Receiver node does nothing else than listening */
	if (rimeaddr_node_addr.u8[0] == ref_node_rime_addr[0]
		&& rimeaddr_node_addr.u8[1] == ref_node_rime_addr[1]) {
		printf("Receiver node listening\n");
		PROCESS_WAIT_EVENT_UNTIL(0);
	}

	while(1) {
		static struct etimer et;
		char msg[] = "Hello Broadcast";

		etimer_set(&et, 10 * CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		packetbuf_copyfrom(msg, sizeof(msg));
		broadcast_send(&broadcast);
		printf("broadcast message sent\n");
	}


	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
