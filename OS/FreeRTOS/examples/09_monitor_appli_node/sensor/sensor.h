#ifndef _SENSOR_H
#define _SENSOR_H

/**
 * Initialize and create the sensor Task.
 * \param xCmdQueue the queue handle for received data
 * \param xDataQueue the queue handle for sending data
 * \param usPriority the task priority
 */
void vCreateSensorTask(xQueueHandle xSensorCmdQueue, xQueueHandle xMacDataQueue, uint16_t usPriority);

#endif
