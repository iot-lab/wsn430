#ifndef _BLINKER_H
#define _BLINKER_H

/**
 * Initialize and create the blinker Task.
 * \param xQueue the queue handle for received data
 * \param usPriority the task priority
 */
void vCreateBlinkerTask(xQueueHandle xQueue, uint16_t usPriority);

#endif
