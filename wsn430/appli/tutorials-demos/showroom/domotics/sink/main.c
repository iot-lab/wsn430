#include <io.h>
#include <signal.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project includes */
#include "clock.h"

/* MAC task include */
#include "interface.h"
#include "mac.h"

/* Hardware initialization */
static void prvSetupHardware( void );

static xSemaphoreHandle xSPIMutexHandle;

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();

    xSPIMutexHandle = xSemaphoreCreateMutex();

    /* Create the tasks of the application */
    vCreateInterfaceTask(1);
    vCreateMacTask( xSPIMutexHandle, 2);

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
