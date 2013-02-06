#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "mac.h"
#include "timerA.h"
#include "lis3lv02dq.h"

int putchar(int c)
{
    return uart0_putchar(c);
}

uint16_t send_packet(void);
uint16_t frame_received(uint8_t packet[], uint16_t length, uint16_t src_addr, int16_t rssi);

volatile int16_t send = 0;

struct {
    int16_t x, y, z;
} msg;

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
    printf("Leg Accelerometer\n");
    eint();

    // mac initialization
    mac_init(10);

    // accel initialization
    lis3lv02dq_init();
    lis3lv02dq_powerup(1);
    lis3lv02dq_set_full_scale(0);
    lis3lv02dq_set_datarate(DR40HZ);

    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8); //4096Hz
    timerA_register_cb(TIMERA_ALARM_CCR0, send_packet);
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 400, 400);

    send = 0;
    while(1)
    {
        LPM0;
        if (send == 1) {
            lis3lv02dq_readXYZ(&msg.x, &msg.y, &msg.z);

            mac_send((uint8_t*)&msg.x, 6, MAC_BROADCAST);
            LED_RED_TOGGLE();
            send = 0;
        }
    }

    return 0;
}


uint16_t send_packet(void) {
    LED_GREEN_TOGGLE();
    send = 1;
    return 1;
}
