#ifndef _ADC_H
#define _ADC_H


/**
 * Initialize and create the ADC task.
 * \param xSPIMutex mutex handle for preventing SPI access confusion
 * \param usPriority priority the task should run at
 */
void vCreateADCTask(uint16_t usPriority);

#endif
