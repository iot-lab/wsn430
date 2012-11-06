#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "mac.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

uint16_t send_packet(void);
uint16_t frame_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);


int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer
    
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1);
    
    LEDS_INIT();
    LEDS_OFF();
    LED_BLUE_ON();
    
    // uart
    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("Leg Receiver\n");
    eint();
    
    // mac initialization
    mac_init(10);
    mac_set_rx_cb(frame_received);
    
    while(1)
    {
        LPM1;
    }
    
    return 0;
}


uint16_t frame_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi) {
    struct {
        int16_t x, y, z;
    } msg;
    
    uint8_t* p;
    int16_t i;
    
    LED_RED_TOGGLE();
    if (length != 6) return 0;
    
    p = (uint8_t*)&msg.x;
    for (i=0;i<6;i++) {
        p[i] = packet[i];
    }
    
    printf("from %.4x : %i %i %i\n", src_addr, msg.x, msg.y, msg.z);
    return 0;
}
