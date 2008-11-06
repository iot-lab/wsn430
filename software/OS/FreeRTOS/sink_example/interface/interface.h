#ifndef _INTERFACE_H
#define _INTERFACE_H

/**
 * Initialize and create the interface Task.
 * \param xTXQueue handle to the received packets queue
 * \param xRXQueue handle to the packets to send queue
 * \param usPriority the task priority
 */
void vCreateInterfaceTask( xQueueHandle xTXQueue, xQueueHandle xRXQueue, uint16_t usPriority);

#endif
