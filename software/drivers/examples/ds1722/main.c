#include <io.h>
#include <stdio.h>

#include "leds.h"
#include "clock.h"
#include "uart0.h"
#include "ds1722.h"

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
    uint8_t temp1,temp2;
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    uart0_init(UART0_CONFIG_1MHZ_38400);

    printf("DS1722 temperature sensor test program\r\n");

    ds1722_init();
    ds1722_set_res(12);
    ds1722_sample_cont();

    while (1)
    {
        temp1 = ds1722_read_MSB();
        temp2 = ds1722_read_LSB();
        printf("t=%u,%u\r\n", temp1, temp2);
        delay(DELAY<<2);
    }
}
