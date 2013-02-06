#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "tdma_c.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

uint16_t mac_new_data(int16_t slot);
volatile int16_t mac_new_dataslot;

int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer

    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1);

    LEDS_INIT();
    LEDS_OFF();
    LED_RED_ON();

    uart0_init(UART0_CONFIG_1MHZ_115200);

    printf("\n-----------------------------------\n");
    printf("TDMA_COORD test\r\n");
    eint();

    mac_init(0);
    mac_set_new_data_cb(mac_new_data);

    printf("*** I'm %u ***\n", node_addr);
    int i;
    while(1) {
        LPM3;
        i = mac_new_dataslot;

        if (mac_slots[i].ready) {
            LED_RED_TOGGLE();
            uart0_putchar('0'+(char)i);
            uart0_putchar(mac_slots[i].data[0]);
            mac_slots[i].ready = 0;
        }
    }
    return 0;
}

uint16_t mac_new_data(int16_t slot) {
    mac_new_dataslot = slot;
    // return 1 to wake the cpu up
    return 1;
}
