#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "mac.h"
#include "log_rssi.h"

// UART callback function
uint16_t char_rx(uint8_t c);

// MAC callback functions
uint16_t frame_rx(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
uint16_t frame_error(void);
uint16_t frame_sent(void);

// printf's putchar
int16_t putchar(int16_t c) {
    return uart0_putchar(c);
}


// Global variables
volatile int16_t data_rx, data_tx; // flags to know when events happened
uint8_t txframe[32]; // frame to send
uint8_t rxframe[32]; // received frame
uint16_t rxfrom, rxlen; // information on received frame
int16_t rxrssi; // received frame rssi

/**
 * The main function.
 */
int main( void )
{
    uint16_t i;

    // Stop the watchdog timer.
    WDTCTL = WDTPW + WDTHOLD;

    // Setup MCLK 8MHz and SMCLK 1MHz
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1); // ACKL is at 32768Hz

    // Initialize the LEDs
    LEDS_INIT();
    LEDS_OFF();

    // Initialize the UART0
    uart0_init(UART0_CONFIG_1MHZ_115200); // we want 115kbaud,
                                          // and SMCLK is running at 1MHz
    uart0_register_callback(char_rx);

    // Print first message
    printf("Senslab TP Ex3: Radio communication\n");
    printf("Type 's' to send a message...\n");

    // Initialize the MAC layer (radio)
    mac_init(20);
    mac_set_rx_cb(frame_rx);
    mac_set_error_cb(frame_error);
    mac_set_sent_cb(frame_sent);

    printf("My MAC address is %.4x\n", node_addr);

    // Enable Interrupts
    eint();

    // clear the flags
    data_rx = 0;
    data_tx = 0;

    while (1) {
        // Enter low power mode, waiting for a 'send message' command
        // or a 'message received' notification
        LPM1;

        // check why the CPU was woken up
        if (data_rx) {
	    print_log_packet(rxfrom, rxrssi);

            for (i=0;i<rxlen;i++)
                printf("%c",rxframe[i]);
            printf("\n");
            data_rx = 0;
        }
        if (data_tx) {
            printf("Sending data...\n");
            mac_send((uint8_t *)"Hello World!", sizeof("Hello World!"), MAC_BROADCAST);
            data_tx = 0;
        }
    }

    return 0;
}

uint16_t char_rx(uint8_t c) {
    if ( (c=='s') && (data_tx==0) ) {
        // send data:
        data_tx = 1;
        // return 1 to wake the CPU up
        return 1;
    }
    // if not the right command, don't wake the CPU up
    // by returning 0
    return 0;
}

uint16_t frame_rx(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi) {
    uint16_t i;
    if (data_rx != 0) {
        // frame received and not read yet
        // return 0 to not wake the CPU up
        return 0;
    }

    if (length > 32) {
        // length too big
        // don't wake the CPU up, return 0
        return 0;
    }

    LED_BLUE_TOGGLE();

    // store data
    rxfrom = src_addr;
    rxlen = length;
    rxrssi = rssi;
    for (i=0;i<length;i++)
        rxframe[i] = packet[i];

    // set data rx flag
    data_rx = 1;

    // return 1 to wake the CPU up
    return 1;
}

uint16_t frame_error(void) {
    LED_RED_TOGGLE();
    return 0;
}

uint16_t frame_sent(void) {
    LED_GREEN_TOGGLE();
    return 0;
}

