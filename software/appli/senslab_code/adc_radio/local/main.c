#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project includes */
#include "clock.h"
#include "uart0.h"

/* MAC task include */
#include "mac.h"
#include "adc.h"
#include "leds.h"


typedef struct {
    uint16_t from;
    uint8_t data[sizeof(adc_data_t)];
} adc_from_t;

/* Hardware initialization */
static void prvSetupHardware( void );
/* printing task */
static void vPrintfTask(void* pvParameters);

/* Global Variables */
static xSemaphoreHandle xSPIMutex;
static xQueueHandle rxADCQueue;
static xQueueHandle localADCQueue;


/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();
    
    /* Create the SPI mutex */
    xSPIMutex = xSemaphoreCreateMutex();
    
    /* Create the MAC task */
    vCreateMacTask(xSPIMutex, configMAX_PRIORITIES-1);
    
    /* Create the ADC sampling Task */
    vCreateADCTask(configMAX_PRIORITIES-2);
    
    
    /* create ADC queues */
    rxADCQueue =    xQueueCreate(2, sizeof(adc_from_t));
    localADCQueue = xQueueCreate(1, sizeof(adc_data_t));
    
    /* Create printf task */
    xTaskCreate( vPrintfTask, "print", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

    /* Start the scheduler. */
    vTaskStartScheduler();
    
    /* As the scheduler has been started we should never get here! */
    return 0;
}

/**
 * Initialize the main hardware parameters.
 */
static void prvSetupHardware( void )
{
    /* Stop the watchdog timer. */
    WDTCTL = WDTPW + WDTHOLD;
    
    /* Setup MCLK 8MHz and SMCLK 1MHz */
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    
    uart0_init(UART0_CONFIG_1MHZ_115200);
    
    printf("FreeRTOS Star Network, local ADC sampler\r\n");
    
    LEDS_INIT();
    LEDS_OFF();
    LED_RED_ON();
    
    /* Enable Interrupts */
    eint();
}

int putchar(int c)
{
    return uart0_putchar(c);
}

void vApplicationIdleHook( void );
void vApplicationIdleHook( void )
{
    _BIS_SR(LPM0_bits);
}

void vPacketReceivedFrom(uint8_t srcAddr, uint16_t pktLength, uint8_t* pkt)
{
    if (pktLength != sizeof(adc_data_t))
    {
        return;
    }
    
    adc_from_t adc_converted;
    
    adc_converted.from = srcAddr;
    uint16_t i;
    for (i=0;i<pktLength;i++)
    {
        adc_converted.data[i] = pkt[i];
    }
    
    xQueueSendToBack(rxADCQueue, &adc_converted, 0);
}

void adc_ready(adc_data_t *data)
{
    xQueueSendToBack(localADCQueue, data, 0);
}

static void vPrintfTask(void* pvParameters)
{
    adc_data_t local_adc, *adc_val;
    adc_from_t rx_adc;
    
    while (1)
    {
        if ( xQueueReceive(rxADCQueue, &rx_adc, 100) )
        {
            adc_val = (adc_data_t*)rx_adc.data;
            printf("ADC from node %x\r\n", rx_adc.from);
            printf("\tADC sequence #%d\r\n", adc_val->seq);
            
            uint16_t i = 0;
            for (i=0; i<6; i++)
            {
                printf("\t%d\t%d\t%d\r\n", adc_val->v_min[i], adc_val->v_max[i], adc_val->v_avg[i]);
            }
            printf("\r\n");
        }
        
        if ( xQueueReceive(localADCQueue, &local_adc, 100) )
        {
            printf("ADC from node local\r\n");
            printf("\tADC sequence #%d\r\n", local_adc.seq);
            
            uint16_t i = 0;
            for (i=0; i<6; i++)
            {
                printf("\t%u\t%u\t%u\r\n", local_adc.v_min[i], local_adc.v_max[i], local_adc.v_avg[i]);
            }
            printf("\r\n");   
        }
        
    }
}
