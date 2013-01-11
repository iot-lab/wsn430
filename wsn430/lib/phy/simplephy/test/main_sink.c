#include <io.h>
#include <signal.h>
#include <stdio.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "simplephy.h"


void frame_received(uint8_t frame[], uint16_t length);


int putchar(int c)
{
    return uart0_putchar(c);
}

int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer

    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("Simplephy test, sink \r\n");
    eint();
    phy_init();
    phy_register_frame_received_handler(frame_received);

    phy_start_rx();

    LEDS_INIT();
    LEDS_OFF();

    while(1);
    {
        LPM3;
    }
    return 0;
}

void frame_received(uint8_t frame[], uint16_t length)
{
    LED_RED_TOGGLE();
    uint16_t i;
    printf("Frame Received:\r\n");
    for (i=0;i<length;i++)
    {
        printf("%x:", frame[i]);
    }
    printf("\r\n");
    phy_start_rx();
}
