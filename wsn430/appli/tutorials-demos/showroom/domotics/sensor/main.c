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
#include "leds.h"

/* MAC task include */
#include "blinker.h"
#include "sensor.h"
#include "mac.h"

/* Hardware initialization */
static void prvSetupHardware( void );
static xSemaphoreHandle xSPIMutex;

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();
    
    xSPIMutex = xSemaphoreCreateMutex();
    
    /* Create the 2 tasks of the application */
    vCreateSensorTask(xSPIMutex, 2);
    vCreateMacTask(xSPIMutex, 4);
    
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
    
    /* Initialize the LEDs */
    LEDS_INIT();
    LEDS_OFF();
}

int putchar(int c)
{
    return 0;
}
