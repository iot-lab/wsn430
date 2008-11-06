#include <io.h>
#include <stdio.h>

#include "leds.h"
#include "clock.h"
#include "uart0.h"
#include "ds2411.h"

/* Define putchar for printf */
int putchar (int c)
{
    return uart0_putchar(c);
}

/**********************
 * Delay function.
 **********************/

#define DELAY 0x800

void delay(unsigned int d) 
{
    int i,j;
    for(j=0; j < 0xff; j++)
    {
        for (i = 0; i<d; i++) 
        {
            nop();
            nop();
        }
    }
}

int main(void) 
{
    WDTCTL = WDTPW | WDTHOLD;
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    uart0_init(UART0_CONFIG_1MHZ_38400);

    printf("DS2411 unique serial number test program\r\n");

    ds2411_init();
    
    printf("Serial number is:\r\n");
    
    int i;
    for (i=0;i<8;i++)
    {
        printf("%x:", ds2411_id.raw[i]);
    }
    printf("\r\n");
    
    
    while (1)
    {}
}
