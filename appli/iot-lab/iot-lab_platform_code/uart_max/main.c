/* General includes */
#include <io.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* Drivers includes */
#include "clock.h"
#include "uart0.h"
#include "leds.h"


/* Prototypes */
/**
 * Initialize the hardware
 */
void hw_init(void);

/**
 * Putchar for printf
 */
int putchar(int c) {
	uart0_putchar(c);
	return c;
}
/**
 * Callback function for the UART DMA transfer
 */
uint16_t uart_transfer_done(void);

uint16_t dma_ready = 1;

uint8_t buffer[256];

/**
 * The main function.
 */
int main(void) {
	int16_t i;
	// Stop the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

	// Configure the IO ports
	hw_init();

	// Setup MCLK 8MHz and SMCLK 8MHz
	set_mcu_speed_xt2_mclk_8MHz_smclk_8MHz();
	// Set ACLK @ 4096Hz
	set_aclk_div(8);

	// Initialize the LEDs
	LEDS_INIT();
	LEDS_OFF();
	LED_BLUE_ON();

	// Configure the uart
	uart0_init(UART0_CONFIG_8MHZ_115200);
	uart0_register_dma_callback(uart_transfer_done);
	eint();

	for (i=0;i<256;i++) {
		buffer[i] = i;
	}

	// Enter an infinite loop
	while (1) {
		// If DMA ready, transfer
		if (dma_ready) {
			dma_ready = 0;

			uart0_dma_putchars(buffer, 256);
		}

		// Sleep until interrupt
		LPM0;
	}

	return 0;
}

uint16_t uart_transfer_done(void) {
	dma_ready = 1;
	return 1;
}

void hw_init(void) {
	P1SEL = 0;
	P2SEL = 0;
	P3SEL = 0;
	P4SEL = 0;
	P5SEL = 0;
	P6SEL = 0;

	P1DIR = BV(0) + BV(1) + BV(2) + BV(5) + BV(6) + BV(7);
	P1OUT = 0;

	P2DIR = BV(0) + BV(1) + BV(2) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P2OUT = 0;

	P3DIR = BV(0) + BV(2) + BV(4) + BV(6) + BV(7);
	P3OUT = BV(2) + BV(4);

	P4DIR = BV(2) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P4OUT = BV(2) + BV(4);

	P5DIR = BV(0) + BV(1) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P5OUT = 0;

	P6DIR = BV(0) + BV(1) + BV(3) + BV(4) + BV(5) + BV(6) + BV(7);
	P6OUT = 0;
}

