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
#include "semphr.h"

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "ds1722.h"

/* Function Prototypes */
static void prvSetupHardware( void );
static void vLEDTask(void* pvParameters);
static uint16_t rx_char_cb(uint8_t c);

int putchar(int c)
{
    return uart0_putchar(c);
}

/* Global Variables */
xSemaphoreHandle xSemaphore;

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();

    /* Create the Semaphore for synchronization between UART and LED task */
    vSemaphoreCreateBinary( xSemaphore)

    /* Add the only task to the scheduler */
    xTaskCreate(vLEDTask, (const signed char*) "LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

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

    /* Configure the UART module for serial communication */
    uart0_init(UART0_CONFIG_1MHZ_115200);
    uart0_register_callback(rx_char_cb);
    printf("type any char to update the LEDs\r\n");

    /* Enable Interrupts */
    eint();
}

/**
 * The LEDs task function.
 * It waits the semaphore is given, then takes it and update the LEDs
 * \param pvParameters NULL is passed, unused here.
 */
static void vLEDTask(void* pvParameters)
{
    uint16_t leds_state = 0;

    /* Initialize the LEDs */
    LEDS_INIT();

    /* Infinite loop */
    while(1)
    {
        /* Increment the LED state */
        leds_state ++;
        /* Block until the semaphore is given */
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        /* update the LEDs and loop */
        LEDS_SET(leds_state);
    }
}

/**
 * The callback function called when a char is received.
 * It gives the semaphore.
 * \param c the received char
 */
static uint16_t rx_char_cb(uint8_t c)
{
    static portBASE_TYPE xHigherPriorityTaskWoken;
    /* Give the semaphore */
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        return 1;
    }
    return 0;
}
