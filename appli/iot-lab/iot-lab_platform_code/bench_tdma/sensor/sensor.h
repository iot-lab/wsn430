#ifndef _SENSOR_H
#define _SENSOR_H

/**
 * Initialize and create the ADC task.
 * \param xSPIMutex mutex handle for preventing SPI access confusion
 * \param usPriority priority the task should run at
 */
void vCreateSensorTask(uint16_t usPriority);

#endif

