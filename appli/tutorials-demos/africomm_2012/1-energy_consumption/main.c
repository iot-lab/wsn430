/* General includes */
#include <io.h>
#include <signal.h>

/* Drivers includes */
#include "clock.h"
#include "leds.h"
#include "timerA.h"

/**
 * Alarm function, that will be called by the timer, and set up the LEDs.
 */
uint16_t alarm(void);

/**
 * The main function.
 */
int main( void )
{
    /* Stop the watchdog timer */
    WDTCTL = WDTPW + WDTHOLD;

    /* Setup the MSP430 micro-controller clock frequency: MCLK, SMCLK and ACLK */

    /* Set MCLK at 8MHz and SMCLK at 1MHz */
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

    /* Set ACKL at 4096Hz (32 768Hz / 8) */
    set_aclk_div(8);

    /* Initialize the LEDs */
    LEDS_INIT();
    LEDS_OFF();

    /* Initialize the timerA (3 timerA are available for MSP430F1611) */
    /* timerA is connected to ACLK clock */
    timerA_init();

    /*  TimerA clock is at 512Hz (4096Hz / 8) */
    timerA_start_ACLK_div(TIMERA_DIV_8);

    /* Configure the first timerA period to 1s (periodic) */
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 512, 512);

    /* Set the first timerA callback */
    timerA_register_cb(TIMERA_ALARM_CCR0, alarm);

    /* Enable Interrupts */
    eint();

    /* Enter an infinite loop */
    while (1) {
        /* Enter Low Power Mode 3. The only clock remaining is ACLK */
        LPM3;
    }

    return 0;
}

uint16_t alarm(void) {
    LED_RED_TOGGLE();
    return 0;
}
