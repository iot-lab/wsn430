#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* Project includes */
#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "tsl2550.h"
#include "ds1722.h"
#include "timerA.h"

#include "mac.h"


// choose channel int [0-20]
#define CHANNEL 17

// UART callback function
static uint16_t char_rx(uint8_t c);

// timer alarm function
static uint16_t alarm(void);

// printf's putchar
int16_t putchar(int16_t c)
{
    return uart0_putchar(c);
}

/* Global variables */
// value storing the character received from the UART, analyzed by the main function
// volatile is required to prevent optimizations on it.
volatile int8_t cmd = 0;
// print help every second
volatile int8_t print_help = 1;

enum {
    NO_EVENT = 0,
    RX_PKT,
    TX_PKT,
    TX_PKT_ERROR,
};

// Got a radio event
volatile struct {
    int8_t got_event;

    uint8_t packet[256];
    uint16_t length;
    uint16_t addr;
    int16_t rssi;
} radio = {0};

/**
 * Sensors
 */
static void temperature_sensor()
{
    int16_t value_0, value_1;

    value_0 = ds1722_read_MSB();
    value_1 = ds1722_read_LSB();
    value_1 >>= 5;
    value_1 *= 125;

    printf("Temperature measure: %i.%i\n", value_0, value_1);
}

static void light_sensor()
{
    int16_t value_0, value_1;
    tsl2550_init();
    value_0 = tsl2550_read_adc0();
    value_1 = tsl2550_read_adc1();

    /* Recover UART0 config for serial */
    uart0_init(UART0_CONFIG_1MHZ_115200);
    uart0_register_callback(char_rx);

    printf("Luminosity measure: %i:%i\n", value_0, value_1);
}

/*
 * Radio config
 */

static void send_packet()
{
    uint16_t ret;
    static uint8_t num = 0;

    // max pkt length <= max(cc2420, cc1101)
    snprintf((char*)radio.packet, 58, "Hello World!: %u", num);
    radio.length = 1 + strlen((char*)radio.packet);
    radio.addr = MAC_BROADCAST;

    ret = mac_send((uint8_t *)radio.packet, radio.length, radio.addr);
    num++;

    if (ret)
        printf("mac_send ret %u\n", ret);
}

static uint16_t mac_rx_isr(uint8_t packet[], uint16_t length,
        uint16_t src_addr, int16_t rssi)
{
    radio.got_event = RX_PKT;

    strcpy((char*)radio.packet, (char*)packet);
    radio.length = length;
    radio.addr = src_addr;
    radio.rssi = rssi;
    return 1;
}

static uint16_t mac_tx_done_isr()
{
    radio.got_event = TX_PKT;
    return 1;
}
static uint16_t mac_tx_fail_isr()
{
    radio.got_event = TX_PKT_ERROR;
    return 1;
}

/*
 * HELP
 */
static void print_usage()
{
    printf("\n\nIoT-LAB Simple Demo program\n");
    printf("Type command\n");
    printf("\th:\tprint this help\n");
    printf("\tt:\ttemperature measure\n");
    printf("\tl:\tluminosity measure\n");
    printf("\ts:\tsend a radio packet\n");
    if (print_help)
        printf("\n Type Enter to stop printing this help\n");
    printf("\n");
}



static void hardware_init()
{
    // Stop the watchdog timer.
    WDTCTL = WDTPW + WDTHOLD;

    // Setup MCLK 8MHz and SMCLK 1MHz
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(8); // ACKL is at 4096Hz

    // Initialize the LEDs
    LEDS_INIT();
    LEDS_OFF();

    // Initialize the temperature sensor
    ds1722_init();
    ds1722_set_res(12);
    ds1722_sample_cont();

    // Initialize the Luminosity sensor
    tsl2550_init();
    tsl2550_powerup();
    tsl2550_set_standard();

    // Init csma Radio mac layer
    mac_init(CHANNEL);
    mac_set_rx_cb(mac_rx_isr);
    mac_set_sent_cb(mac_tx_done_isr);
    mac_set_error_cb(mac_tx_fail_isr);

    // Initialize the UART0
    uart0_init(UART0_CONFIG_1MHZ_115200); // We want 115kbaud,
    // and SMCLK is running at 1MHz
    uart0_register_callback(char_rx);   // Set the UART callback function
    // it will be called every time a
    // character is received.


    // Enable Interrupts
    eint();

    // Initialize the timer for the LEDs
    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8); // TimerA clock is at 512Hz
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 512, 512); // 1s period
    timerA_register_cb(TIMERA_ALARM_CCR0, alarm);
}

static void handle_cmd(uint8_t cmd)
{
    switch (cmd) {
        case 't':
            temperature_sensor();
            break;
        case 'l':
            light_sensor();
            break;
        case 's':
            send_packet();
            break;
        case '\n':
            break;
        case 'h':
        default:
            print_usage();
            break;
    }
}

static void handle_radio()
{
    if (radio.got_event == NO_EVENT)
        return;

    printf("\nradio > ");

    switch (radio.got_event) {
        case RX_PKT:
            printf("Got packet from %x. Len: %u Rssi: %d: '%s'\n",
                    radio.addr, radio.length, radio.rssi, (char*)radio.packet);
            break;
        case TX_PKT:
            printf("Packet sent\n");
            break;
        case TX_PKT_ERROR:
            printf("Packet sent failed\n");
            break;
        default:
            printf("Uknown event\n");
            break;
    }
}

int main( void )
{
    hardware_init();

    while (1) {
        while ((cmd == 0) && (radio.got_event == 0))
            LPM0; // Low Power Mode 1: SMCLK remains active for UART

        if (cmd) {
            handle_cmd(cmd);
            cmd = 0;
        }
        if (radio.got_event) {
            // disable help message
            print_help = 0;

            handle_radio();
            radio.got_event = 0;

        }
        printf("cmd > ");
    }
    return 0;
}


static uint16_t char_rx(uint8_t c) {
    // disable help message after receiving char
    print_help = 0;

    if (c=='t' || c=='l' || c=='h' || c=='s' || c=='\n') {
        // copy received character to cmd variable.
        cmd = c;
        // return not zero to wake the CPU up.
        return 1;
    }

    // if not a valid command don't wake the CPU up.
    return 0;
}

static uint16_t alarm(void) {
    LED_RED_TOGGLE();
    LED_BLUE_TOGGLE();
    LED_GREEN_TOGGLE();

    /* Print help before getting first real \n */
    if (print_help) {
        cmd = 'h';
        return 1;
    }
    return 0;
}
