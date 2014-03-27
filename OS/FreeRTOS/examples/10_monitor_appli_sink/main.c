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
#include "interface.h"
#include "mac.h"

/* Hardware initialization */
static void prvSetupHardware( void );

static xQueueHandle xTXDataQueue, xRXDataQueue;

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();

    /* Create the required queues */
    xTXDataQueue = xQueueCreate(3, sizeof(txdata_t));
    xRXDataQueue = xQueueCreate(3, sizeof(rxdata_t));

    /* Create the tasks of the application */
    vCreateInterfaceTask( xTXDataQueue, xRXDataQueue, 1);
    vCreateMacTask( xTXDataQueue, xRXDataQueue, 2);

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
    set_mcu_speed_xt2_mclk_8MHz_smclk_8MHz();

    /* Enable Interrupts */
    eint();
}
