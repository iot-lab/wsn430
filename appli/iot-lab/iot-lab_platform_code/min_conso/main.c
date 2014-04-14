/* General includes */
#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Drivers includes */
#include "clock.h"
#include "uart0.h"
#include "leds.h"

#if VERSION == 13
#include "cc1101.h"
#warning "using version 13"
#elif VERSION == 14
#include "cc2420.h"
#warning "using version 14"
#else
#error "Bad Version"
#endif

#include "ds1722.h"
#include "ds2411.h"
#include "m25p80.h"
#include "tsl2550.h"


void io_init(void);

int putchar(int c) {
	return uart0_putchar(c);
}

/**
 * The main function.
 */
int main( void )
{
    /* Stop the watchdog timer. */
    WDTCTL = WDTPW + WDTHOLD;

    /* Configure the IO ports */
    io_init();

    /* Setup MCLK 8MHz and SMCLK 1MHz */
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(8); // ACKL is at 4096Hz

    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("Starting low power\n");

    /* Initialize the LEDs */
    LEDS_INIT();
    LEDS_OFF();

    // shutdown everything
    // temp sensor
    ds1722_init();
    ds1722_stop();

    // light sensor
    tsl2550_init();
    tsl2550_powerdown();

    // flash mem
    m25p80_init();
    m25p80_power_down();

    // unique ID
    ds2411_init();

    // radio
#if VERSION == 13
    cc1101_init();
    cc1101_cmd_pwd();
#elif VERSION == 14
    cc2420_init();
    cc2420_cmd_xoscoff();
#else
#error "Bad Version"
#endif


    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("low power enabled\n");
    uart0_stop();

    // Enter an infinite loop
    while (1) {
        LPM4; // do nothing
    }

    return 0;
}

void io_init(void) {
    P1SEL = 0;
    P2SEL = 0;
    P3SEL = 0;
    P4SEL = 0;
    P5SEL = 0;
    P6SEL = 0;

    P1DIR = BV(0)+BV(1)+BV(2)+BV(5)+BV(6)+BV(7);
    P1OUT = 0;

    P2DIR = BV(0)+BV(1)+BV(2)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
    P2OUT = 0;

    P3DIR = BV(0)+BV(2)+BV(4)+BV(6)+BV(7);
    P3OUT = BV(2)+BV(4);

    P4DIR = BV(2)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
    P4OUT = BV(2)+BV(4);

    P5DIR = BV(0)+BV(1)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
    P5OUT = 0;

    P6DIR = BV(0)+BV(1)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
    P6OUT = 0;
}
