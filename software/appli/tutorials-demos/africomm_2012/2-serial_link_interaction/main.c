#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "tsl2550.h"
#include "ds1722.h"
#include "timerA.h"

// UART callback function
uint16_t char_rx(uint8_t c);

// timer alarm function
uint16_t alarm(void);

// printf's putchar
int16_t putchar(int16_t c) {
    return uart0_putchar(c);
}

/* Global variables */
// value storing the character received from the UART, analyzed by the main function
// volatile is required to prevent optimizations on it.
volatile int8_t cmd = 0;

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

    /* Initialize the temperature sensor */
    ds1722_init();
    ds1722_set_res(12);
    ds1722_sample_cont();

    /* Initialize the Luminosity sensor */
    tsl2550_init();
    tsl2550_powerup();
    tsl2550_set_standard();
    tsl2550_read_adc0();

    /* Initialize the UART0 */

	/* We want 115kbaud, and SMCLK is running at 1MHz */
    uart0_init(UART0_CONFIG_1MHZ_115200);

	/* Set the UART callback function it will be called every time a character is received. */
    uart0_register_callback(char_rx);

    /* Print first message */
    printf("\n\nSenslab Simple Demo program\n");

    /* Enable Interrupts */
    eint();

    /* Print information */
    printf("Type command\n");
    printf("\tt:\ttemperature measure\n");
    printf("\tl:\tluminosity measure\n");

    /* Initialize the timer for the LEDs */
    timerA_init();

	/*  TimerA clock is at 512Hz (4096Hz / 8) */
    timerA_start_ACLK_div(TIMERA_DIV_8);

    /* Configure the first timerA period to 1s (periodic) */
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 512, 512);

	/* Set the first timerA callback */
    timerA_register_cb(TIMERA_ALARM_CCR0, alarm);

    // Declare 2 variables for storing the different values
    int16_t value_0=0, value_1=1;
    while (1) {
        printf("cmd > ");
        cmd = 0;

        while (cmd==0) {
            LPM0; // Low Power Mode 1: SMCLK remains active for UART
        }

        switch (cmd) {
        case 't':
            value_0 = ds1722_read_MSB();
            value_1 = ds1722_read_LSB();
            value_1 >>= 5;
            value_1 *= 125;
            printf("Temperature measure: %i.%i\n", value_0, value_1);
            break;
        case 'l':
            tsl2550_init();
            value_0 = tsl2550_read_adc0();
            value_1 = tsl2550_read_adc1();
            uart0_init(UART0_CONFIG_1MHZ_115200);
            uart0_register_callback(char_rx);
            printf("Luminosity measure: %i:%i\n", value_0, value_1);
            break;
        default:
            break;
        }
    }

    return 0;
}

uint16_t char_rx(uint8_t c) {
    if (c=='t' || c=='l') {
        // copy received character to cmd variable.
        cmd = c;
        // return not zero to wake the CPU up.
        return 1;
    } else {
        // if not a valid command don't wake the CPU up.
        return 0;
    }
}

uint16_t alarm(void) {
    LED_RED_TOGGLE();
    LED_BLUE_TOGGLE();
    LED_GREEN_TOGGLE();
    return 0;
}
