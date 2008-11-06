#include <io.h>
#include <signal.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project includes */
#include "clock.h"

/* MAC task include */
#include "queue_types.h"
#include "blinker.h"
#include "sensor.h"
#include "mac.h"

/* Hardware initialization */
static void prvSetupHardware( void );

xQueueHandle xBlinkerCmdQueue, xSensorCmdQueue, xMacDataQueue;

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();
    
    /* Create the 3 required queues */
    xBlinkerCmdQueue = xQueueCreate( 3, sizeof(rxdata_t) );
    xSensorCmdQueue = xQueueCreate( 3, sizeof(rxdata_t) );
    xMacDataQueue = xQueueCreate( 3, sizeof(txdata_t) );
    
    /* Create the 3 tasks of the application */
    vCreateBlinkerTask(xBlinkerCmdQueue, 1);
    vCreateSensorTask(xSensorCmdQueue, xMacDataQueue, 2);
    vCreateMacTask(xBlinkerCmdQueue, xSensorCmdQueue, xMacDataQueue, 3);
    
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

int putchar(int c)
{
    return 0;
}
