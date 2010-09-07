/**
 * \file main.c
 */
#include <stdio.h>
#include <io.h>
#include <signal.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "ds1722.h"

/* Function Prototypes */
static void prvSetupHardware( void );
static void vPrintTask(void* pvParameters);
static void vTempTask(void* pvParameters);

int putchar(int c)
{
    return uart0_putchar(c);
}

/* Global Variables */
xQueueHandle xQueue;

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();
    
    /* Create the Queue for communication between the tasks */
    xQueue = xQueueCreate( 5, sizeof(uint8_t) );
    
    /* Add the two tasks to the scheduler */
    xTaskCreate(vPrintTask, (const signed char*) "Print", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate(vTempTask, (const signed char*) "Temperature", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    
    
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
    
    /* Enable Interrupts */
    eint();
}

/**
 * The Print task function.
 * It waits until a sensor value has been put in the queue,
 * and prints its value on the uart port.
 * \param pvParameter NULL is passed as parameter.
 */
static void vPrintTask(void* pvParameters)
{
    uint8_t temp_meas;
    uint16_t samplecount = 0;
    
    /* Initialize the uart port */   
    uart0_init(UART0_CONFIG_1MHZ_115200);
    
    /* Infinite loop */
    while (1)
    {
        /* Wait until an element is received from the queue */
        if (xQueueReceive(xQueue, &temp_meas, portMAX_DELAY))
        {
            samplecount++;
            /* Print the result on the uart port */
            printf("Sample #%u: temperature = %u C\r\n", samplecount, temp_meas);
        }
    }
}

/**
 * The temperature measurement task function.
 * It reads the temperature from the sensor and puts it on the queue
 * \param pvParameters NULL is passed, unused here.
 */
static void vTempTask(void* pvParameters)
{
    uint8_t msb;
    uint16_t xLastWakeTime = xTaskGetTickCount();
    
    /* The sample period is 1000 ticks, about 1s */
    const uint16_t xWakePeriod = 1000;
    
    /* Initialize the temperature sensor */
    ds1722_init();
    ds1722_set_res(8);
    ds1722_sample_cont();
    
    /* Infinite loop */
    while(1)
    {
        /* Read the sensor */
        msb = ds1722_read_MSB();
        
        /* Put the read value on the queue */
        xQueueSendToBack(xQueue, &msb, 0);
        
        /* Block until xWakePeriod(=1000) ticks since previous call */
        vTaskDelayUntil(&xLastWakeTime, xWakePeriod);
    }
}
