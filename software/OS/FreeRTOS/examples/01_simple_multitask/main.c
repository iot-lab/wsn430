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

/* Hardware initialization */
static void prvSetupHardware( void );
static void vLEDTask(void* pvParameters);
static void vTempTask(void* pvParameters);

/**
 * Putchar function required by stdio.h module to be able to use printf()
 */
int putchar(int c)
{
    return uart0_putchar(c);
}

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();
    
    /* Add the two tasks to the scheduler */
    xTaskCreate(vLEDTask,(const signed char*) "LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
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
 * The LED task function.
 * It increments a variable and displays its value on the 3 LEDs,
 * then waits for a given delay and start over again.
 * \param pvParameter NULL is passed as parameter.
 */
static void vLEDTask(void* pvParameters)
{
    uint16_t led_state = 0;
    /* The LEDs are updated every 200 ticks, about 200 ms */
    const uint16_t blinkDelay = 200;
    
    /* Initialize the LEDs */
    LEDS_INIT();
    
    /* Infinite loop */
    while (1)
    {
        /* Set the LEDs according to the value of the led_state value */
        LEDS_SET(led_state);
        
        /* Increment the variable */
        led_state += 1;
        
        /* Block the task for the defined time */
        vTaskDelay(blinkDelay);
    }
}

/**
 * The temperature measurement task function.
 * It reads the temperature from the sensor and print it
 * on the serial port.
 * \param pvParameters NULL is passed, unused here.
 */
static void vTempTask(void* pvParameters)
{
    uint8_t msb;
    uint16_t samplecount = 0;
    uint16_t xLastWakeTime = xTaskGetTickCount();
    /* The display period is 1000 ticks, about 1s */
    const uint16_t xWakePeriod = 1000;
    
    /* Initialize the uart port, and the temperature sensor */
    uart0_init(UART0_CONFIG_1MHZ_115200);
    ds1722_init();
    ds1722_set_res(8);
    ds1722_sample_cont();
    
    /* Infinite loop */
    while(1)
    {
        /* Read the sensor and increment the sample count */
        msb = ds1722_read_MSB();
        samplecount++;
        
        /* Print the result on the uart port */
        printf("Sample #%u: temperature = %u C\r\n", samplecount, msb);
        
        /* Block until xWakePeriod ticks since previous call */
        vTaskDelayUntil(&xLastWakeTime, xWakePeriod);
    }
}
