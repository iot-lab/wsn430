#ifndef _MAC_H
#define _MAC_H


#define BLINKER_CMD_TYPE 0X30
#define SENSOR_CMD_TYPE  0x31
#define SAMPLE_DATA_TYPE 0x32

/**
 * Initialize and create the MAC task.
 * \param xBlinkerCmdQueue handle of the blinker command queue
 * \param xSensorCmdQueue handle of the sensor command queue
 * \param xMacDataQueue handle of the data queue
 * \param usPriority priority of the task
 */
void vCreateMacTask( xQueueHandle xBlinkerCmdQueue, xQueueHandle xSensorCmdQueue, xQueueHandle xMacDataQueue, uint16_t usPriority);

#endif
