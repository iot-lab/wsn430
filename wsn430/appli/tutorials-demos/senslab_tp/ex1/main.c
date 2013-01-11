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
    /* Stop the watchdog timer. */
    WDTCTL = WDTPW + WDTHOLD;
    
    /* Setup MCLK 8MHz and SMCLK 1MHz */
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(8); // ACKL is at 4096Hz
    
    /* Initialize the LEDs */
    LEDS_INIT();
    LEDS_OFF();
    
    /* Initialize the timerA */
    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8); // TimerA clock is at 512Hz
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 512, 512); // 1s period
    timerA_register_cb(TIMERA_ALARM_CCR0, alarm);
    
    /* Enable Interrupts */
    eint();
    
    // Enter an infinite loop
    while (1) {
        LPM3; // Enter Low Power Mode 3. The only clock remaining is ACLK.
    }
    
    return 0;
}

uint16_t alarm(void) {
    LED_RED_TOGGLE();
    return 0;
}
