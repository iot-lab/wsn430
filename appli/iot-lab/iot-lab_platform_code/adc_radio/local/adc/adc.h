#ifndef _ADC_H
#define _ADC_H

typedef struct {
    uint16_t    seq;
    uint16_t    v_min[6],
                v_max[6],
                v_avg[6];
} adc_data_t;

/**
 * Initialize and create the ADC task.
 * \param xSPIMutex mutex handle for preventing SPI access confusion
 * \param usPriority priority the task should run at
 */
void vCreateADCTask(uint16_t usPriority);

void adc_ready(adc_data_t* data);

#endif
