#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "tdma_n.h"

uint16_t mac_ready(void);

int putchar(int c)
{
    return uart0_putchar(c);
}

int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer
    
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1);
    
    LEDS_INIT();
    LEDS_OFF();
    
    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("\n-----------------------------------\n");
    printf("TDMA_NODE test\r\n");
    eint();
    
    mac_init(0);
    mac_set_access_allowed_cb(mac_ready);
    
    printf("*** I'm %u ***\n", node_addr);

    uint8_t dodo = 'a';
    while(1) {
        if (mac_is_access_allowed()) {
            mac_payload[0] = dodo;
            mac_send();
            
            dodo++;
            if (dodo>'z')dodo='a';
            
            LED_RED_TOGGLE();
        }
        LPM3;
    }
    
    return 0;
}

uint16_t mac_ready(void) {
    // return 1 to wake the cpu up
    return 1;
}
