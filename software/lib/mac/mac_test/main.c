#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "mac.h"
#include "timerA.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);

uint16_t send_packet(void);
uint16_t packet_error(void);
uint16_t packet_sent(void);
uint16_t char_rx(uint8_t c);

int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer
    
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1);
    
    LEDS_INIT();
    LEDS_OFF();
    LED_BLUE_ON();
    
    uart0_init(UART0_CONFIG_1MHZ_115200);
    uart0_register_callback(char_rx);
    printf("MAC test\r\n");
    eint();
    
    mac_init(10);
    mac_set_rx_cb(packet_received);
    mac_set_error_cb(packet_error);
    mac_set_sent_cb(packet_sent);
    
    printf("I'm %.4x\n", node_addr);
    
    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8);
    timerA_register_cb(TIMERA_ALARM_CCR0, send_packet);
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 8192, 8192);
    while(1)
    {
        LPM1;
    }
    
    return 0;
}

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi)
{
    LED_BLUE_OFF();
    printf("Packet from %x: ", src_addr);
    uint16_t i;
    for (i = 0; i < length; i++)
        printf("%c", packet[i]);
    printf("\n");
    LED_BLUE_ON();
    
    return 0;
}

uint16_t send_packet(void) {
    static uint8_t msg[64];
    static uint16_t count = 0;
    int len;
    len = sprintf((char *)msg, "Hello #%u", count);
    count ++;
    printf("sending: %s\n", msg);
    mac_send(msg, len, MAC_BROADCAST);
    
    return 0;
}

uint16_t packet_sent(void) {
    printf("packet sent\n");
    return 0;
}

uint16_t packet_error(void) {
    printf("packet error\n");
    return 0;
}

uint16_t char_rx(uint8_t c) {
    if (c == ' ') timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 12, 0);
    return 0;
}
