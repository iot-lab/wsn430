#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "mac.h"
#include "timerA.h"

#define RECEIVING 21
#define SENDING 22

#define TYPE_WHOSHERE  12
#define TYPE_IMHERE    13
#define TYPE_TAKETOKEN 14

// printf's putchar
int16_t putchar(int16_t c) {
    return uart0_putchar(c);
}

// UART callback function
uint16_t char_rx(uint8_t c);

// MAC callback functions
uint16_t frame_rx(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);
uint16_t frame_error(void);
uint16_t frame_sent(void);

// Sending functions
uint16_t send_imhere(void);
uint16_t send_whoshere(void);
uint16_t send_token(void);

uint16_t timeout(void);

// Global variables
typedef struct {
    uint8_t type, // either who's there? / I'm here! / Take my token!
            id; // token id
} tokenframe_t;

tokenframe_t token;

volatile int16_t flag_gottoken,
        flag_gotwhoshere,
        flag_gotimhere;

volatile int16_t state;
uint16_t time_delay;
uint16_t whosfrom, imfrom;

/**
 * The main function.
 */
int main( void )
{
    // Stop the watchdog timer.
    WDTCTL = WDTPW + WDTHOLD;

    // Setup MCLK 8MHz and SMCLK 1MHz
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1); // ACKL is at 32768Hz

    // Initialize the LEDs (OFF)
    LEDS_INIT();
    LEDS_OFF();

    // Initialize the UART0
    uart0_init(UART0_CONFIG_1MHZ_115200); // we want 115kbaud,
                                          // and SMCLK is running at 1MHz
    uart0_register_callback(char_rx);

    // Initialize the MAC layer (radio)
    mac_init(10);
    mac_set_rx_cb(frame_rx);
    mac_set_error_cb(frame_error);
    mac_set_sent_cb(frame_sent);

    // Print first message
    printf("Senslab Demo: Token Ring\n");
    printf("I'm %.4x\n\n", node_addr);
    printf("Type 'T' to give a Token\n");


    printf("My MAC address is %.4x\n", node_addr);
    token.id = 0;


    // Enable Interrupts
    eint();

    // start TimerA
    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_4); // 2048Hz

    // clear the flags
    state = RECEIVING;
    while (1) {
        // Enter low power mode
        LPM1;

        if (flag_gottoken) {
            flag_gottoken = 0;
            state = SENDING;
            token.id++;

            printf("\x1b[101m");
            printf("\x1b[30m");
            printf("\x1b[2J");
            printf("Got New Token <%i>\n", token.id);
            LEDS_ON();
            time_delay = rand() & 0xFFF;
            time_delay += 0x1000;
            timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, time_delay, 0);
            timerA_register_cb(TIMERA_ALARM_CCR0, send_whoshere);
            timerA_set_alarm_from_now(TIMERA_ALARM_CCR1, 4096*5, 0);
            timerA_register_cb(TIMERA_ALARM_CCR1, timeout);
        }

        if ((state==RECEIVING)&&(flag_gotwhoshere)) {
            flag_gotwhoshere=0;
            //~ printf("got who's here\n");
            time_delay = rand() & 0xFFF;
            time_delay += 0x1000;
            timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, time_delay, 0);
            timerA_register_cb(TIMERA_ALARM_CCR0, send_imhere);

        }

        if ((state==SENDING)&&(flag_gotimhere)) {
            flag_gotimhere=0;
            //~ printf("got i'm here\n");
            time_delay = rand() & 0xFFF;
            time_delay += 0x1000;
            timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, time_delay, 0);
            timerA_register_cb(TIMERA_ALARM_CCR0, send_token);
            state=RECEIVING;
            LEDS_OFF();
        }
    }

    return 0;
}

uint16_t timeout(void) {
    state = RECEIVING;
    flag_gottoken=0;
    flag_gotwhoshere=0;
    flag_gotimhere=0;
    LEDS_OFF();
    printf("\x1b[0m");
    printf("\x1b[2J");
    return 0;
}

uint16_t send_token(void) {
    //~ printf("sending token\n");
    mac_send((uint8_t*)&token, sizeof(tokenframe_t), imfrom);
    return 0;
}

uint16_t send_whoshere(void) {
    uint8_t whoshere[1];
    //~ printf("sending who's here\n");

    whoshere[0] = TYPE_WHOSHERE;
    mac_send(whoshere, 1, MAC_BROADCAST);
    return 0;
}

uint16_t send_imhere(void) {
    uint8_t imhere[1];
    //~ printf("sending i'm here\n");

    imhere[0] = TYPE_IMHERE;
    mac_send(imhere, 1, whosfrom);
    return 0;
}

uint16_t char_rx(uint8_t c) {
    if ( c=='T') {
        flag_gottoken = 1;
        token.id=0;
        // return 1 to wake the CPU up
        return 1;
    }
    return 0;
}

uint16_t frame_rx(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi) {

    if ( (state==RECEIVING)&&(length==1)&&(packet[0]==TYPE_WHOSHERE) ) {
        // got valid whoshere
        whosfrom = src_addr;
        flag_gotwhoshere = 1;
        return 1;
    } else if ((state==SENDING)&&(length==1)&&(packet[0]==TYPE_IMHERE)) {
        // got valid imhere
        imfrom = src_addr;
        flag_gotimhere = 1;
        return 1;
    } else if ( length==sizeof(tokenframe_t) ) {
        token.id = packet[1];
        flag_gottoken = 1;
        return 1;
    }

    return 0;
}


uint16_t frame_error(void) {
    printf("se");
    return 0;
}

uint16_t frame_sent(void) {
    printf("so");
    return 0;
}

