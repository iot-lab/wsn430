#include <io.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "mac.h"
#include "adc.h"
#include "leds.h"

#define ADC_COUNT_AVG 256

typedef struct {
    uint16_t    seq;
    uint16_t    v_min[6],
                v_max[6],
                v_avg[6];
} adc_data_t;

/* Function Prototypes */
static void vADCTask(void* pvParameters);
static void vADCInit(void);

/* Local Variables */
static xQueueHandle xDataQueue;

static adc_data_t adc_data, send_data;
static uint32_t adc_sum[6];
static uint16_t adc_count;

static uint16_t* ADC12MEMx = 0x140;

static void inline vADCClear()
{
    int i;
    for (i=0;i<6;i++)
    {
        adc_sum[i] = 0;
        adc_data.v_min[i] = 4095;
        adc_data.v_max[i] = 0;
    }
    
    adc_count = 0;
}


void vCreateADCTask( uint16_t usPriority )
{
    /* Create a ADC data Queue */
    xDataQueue = xQueueCreate(1, sizeof(adc_data_t));
    
    /* Create the task */
    xTaskCreate( vADCTask, "adc", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vADCTask(void* pvParameters)
{
    vADCInit();
    
    LED_GREEN_OFF();
    
    for (;;)
    {
        if ( xQueueReceive(xDataQueue, &send_data, portMAX_DELAY) )
        {
            LED_GREEN_ON();
            xSendPacket(sizeof(adc_data_t), (uint8_t*)&send_data);
            LED_GREEN_OFF();
        }
    }
}

static void vADCInit(void)
{
    /* Configure ADC pins for analog input */
    P6SEL |=  ( (0x1) | (0x1<<1) | (0x1<<3) | (0x1<<4) | (0x1<<6) | (0x1<<7) );
    P6DIR &= ~( (0x1) | (0x1<<1) | (0x1<<3) | (0x1<<4) | (0x1<<6) | (0x1<<7) );
    
    /* 38us sampling, internal reference 1.5V */
    ADC12CTL0 = SHT0_7 | REFON | ADC12ON;
    
    /* Start@ADD0, TimerB OUT1 source, Repeat seq. of chan. */
    ADC12CTL1 = SHS_3 | SHP | CONSEQ_3;
    
    /* Channel A0, vREF */
    ADC12MCTL0 = INCH_0 | SREF_1;
    
    /* Channel A1, vREF */
    ADC12MCTL1 = INCH_1 | SREF_1;
    
    /* Channel A4, vREF */
    ADC12MCTL2 = INCH_4 | SREF_1;
    
    /* Channel A3, vREF */
    ADC12MCTL3 = INCH_3 | SREF_1;
    
    /* Channel A6, vREF */
    ADC12MCTL4 = INCH_6 | SREF_1;
    
    /* Channel A7, vREF, End Of Sequence */
    ADC12MCTL5 = EOS | INCH_7 | SREF_1;
    
    /* Interrupt on end of sequence */
    ADC12IE = (0x1<<5);

    /* Timer B clear*/
    TBCTL = TBCLR;
    
    /* Source from SMCLK (1MHz), UP mode */
    TBCTL |= TBSSEL_2 | MC_1;
    
    /* Up mode until 475us (we want a period of 476us) */
    TBCCR0 = 475;
    TBCCR1 = 474;
    
    /* CCR1 set/reset output mode */
    TBCCTL1 = OUTMOD_3;
    
    /* Start Timer */
    TBCTL &= ~(TBCLR);
    
    vADCClear();

    /* Start ADC conversions */
    ADC12CTL0 |= ENC;
}


/**
 * ADC12 Interrupt service routine
 * Execute once the 6 ADCs have been sampled
 */
interrupt(ADC12_VECTOR) adc12irq(void)
{
    uint16_t measure;    
    uint16_t adc;
    
    for (adc = 0; adc < 6; adc++)
    {
        measure = ADC12MEMx[adc];
        
        if ( measure < adc_data.v_min[adc] )
        {
            adc_data.v_min[adc] = measure;
        }
        
        if ( measure > adc_data.v_max[adc] )
        {
            adc_data.v_max[adc] = measure;
        }
        
        adc_sum[adc] += measure;
    }
    
    adc_count ++;
    
    if (adc_count == ADC_COUNT_AVG)
    {
        adc_data.seq ++;
        
        for (adc = 0; adc < 6; adc++)
        {
            adc_data.v_avg[adc] = adc_sum[adc] >> 8;
        }
        
        uint16_t woken;
        xQueueSendToBackFromISR(xDataQueue, &adc_data, &woken);
        
        vADCClear();
        
        if (woken)
        {
            vPortYield();
        }
        
    }
}
