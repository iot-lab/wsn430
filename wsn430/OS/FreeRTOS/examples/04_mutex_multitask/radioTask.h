#ifndef _RADIOTASK_H_
#define _RADIOTASK_H_

/**
 * Function called by the main function. It stores the data queue
 * and SPI mutex handles, and adds the task to the scheduler.
 * \param xQueue handle of the queue for temperature measurements
 * \param xMutex handle of the SPI mutex
 */
void vCreateRadioTask(xQueueHandle xQueue, xSemaphoreHandle xMutex);

#endif
