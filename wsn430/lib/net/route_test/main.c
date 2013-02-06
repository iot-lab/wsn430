#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "route.h"
#include "timerA.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr);

uint16_t send_packet(void);
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
    printf("-----------------------------------\n");
    printf("ROUTING test\r\n");
    eint();
    
    net_init();
    net_register_rx_cb(packet_received);
    
    printf("I'm %.4x\n", node_addr);
    
    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8);
    timerA_register_cb(TIMERA_ALARM_CCR0, send_packet);
    
    while(1)
    {
        LPM1;
    }
    
    return 0;
}

uint16_t packet_received(uint8_t packet[], uint16_t length, uint16_t src_addr)
{
    LED_BLUE_OFF();
    printf("Packet from %x: ", src_addr);
    int i;
    for (i=0;i<length;i++)
        printf("%c", packet[i]);
    printf("\n");
    LED_BLUE_ON();
    
    return 0;
}

uint16_t send_packet(void) {
    static uint8_t msg[64];
    static uint16_t count = 0;
    int len;
    len = sprintf(msg, "Hello #%u", count);
    count ++;
    printf("sending: %s\n", msg);
    
    uint16_t dst;
    
    switch (node_addr) {
        case 0xb020: dst=0xbc97;break;
        case 0xb6d8: dst=0xb047;break;
        case 0xc631: dst=0xcc0d;break;
        
        default: dst = NET_BROADCAST;break;
    }
    
    net_send(msg, len, dst);
    
    return 0;
}



#include "cc1101.h"
uint16_t char_rx(uint8_t c) {
    static uint8_t c0 = 0;
    if (c == '\n') {
        if (c0=='s') {
            send_packet();
        } else {
            printf("state=%x\n", cc1101_status()&0x70);
            printf("txb=%x\n", cc1101_status_txbytes());
            printf("rxb=%x\n", cc1101_status_rxbytes());
        }
    }
    
    c0=c;
    
    return 0;
}
