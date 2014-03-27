#ifndef _MAC_H
#define _MAC_H

/**
 * Initialize and create the MAC task.
 * \param xTXQueue handle to the received packets queue
 * \param xRXQueue handle to the packets to send queue
 * \param usPriority priority of the task
 */
void vCreateMacTask( xQueueHandle xTXQueue, xQueueHandle xRXQueue, uint16_t usPriority);

#endif
