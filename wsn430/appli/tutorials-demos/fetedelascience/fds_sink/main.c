#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project includes */
#include "clock.h"
#include "uart0.h"

/* MAC task include */
#include "mac.h"

/* Hardware initialization */
static void prvSetupHardware( void );

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();

    /* Create the task of the application */
    vCreateMacTask(configMAX_PRIORITIES-1);

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

    // MANDATORY - Set P2.1 as input for SMBus-Alert signal from INA209 chips
    P2SEL &= ~0x02;		// P2.1 as GPIO
    P2DIR &= ~0x02;		// P2.1 as input


    // set K1 command as output gpio
    P2SEL &= ~0x08;		// P2.3 as GPIO
    P2DIR |= 0x08;		// P2.3 as output
    P2OUT |= 0x08;		// P2.3 at high level

    // set K2 command as output gpio
    P2SEL &= ~0x01;		// P2.0 as GPIO
    P2DIR |= 0x01;		// P2.0 as output
    P2OUT |= 0x01;		// P2.0 at high level

    uart0_init(UART0_CONFIG_1MHZ_115200);

    printf("Fete de la Science, Sink\r\n");

    /* Enable Interrupts */
    eint();
}

int putchar(int c)
{
    return uart0_putchar(c);
}
