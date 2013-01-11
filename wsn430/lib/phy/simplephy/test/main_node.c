#include <io.h>
#include <signal.h>
#include <stdio.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "simplephy.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

static void delay(uint16_t n)
{
	unsigned int i, j;
	for (i=0;i<n;i++)
	{
		for (j=0;j<0xFF;j++)
		{
			nop();
			nop();
		}
	}
}

int main (void)
{
    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer

    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("Simplephy test, node \r\n");
    eint();
    phy_init();

    LEDS_INIT();
    LEDS_OFF();

    uint8_t i = 0;
    while(1)
    {
        printf("sending frame: %x\n", i);
        phy_send_frame(&i, 1);
        LED_RED_TOGGLE();
        i++;
        delay(4800);
    }

    return 0;
}

