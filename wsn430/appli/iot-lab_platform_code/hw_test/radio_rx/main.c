#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Project includes */
#if USE_CC2420
    #warning using CC2420
    #include "cc2420.h"
    #define RADIO "CC2420"
#else
    #warning using CC1101
    #include "cc1101.h"
    #define RADIO "CC1101"
#endif
#include "clock.h"
#include "leds.h"
#include "uart0.h"

uint16_t rxok_cb(void);
void radio_init(void);
void radio_rx(void);
void radio_read_frame(void);

int putchar(int c)
{
    uart0_putchar((char) c);
    return c;
}

uint8_t rxframe[64];
uint8_t rxlen;
volatile int flag;
/**
 * The main function.
 */
int main( void )
{
    /* Stop the watchdog timer. */
    WDTCTL = WDTPW + WDTHOLD;

    // Setup Clock
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(8);

    // Setup UART0
    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("UART0 initialized\r\n");

    // Setup LEDs
    LEDS_INIT();
    LEDS_ON();
    printf("LEDs initialized\r\n");

    // Setup CC1101
    radio_init();
    printf("CC1101 initialized\r\n");

    /* Enable Interrupts */
    eint();

    printf("WSN430v1.4 TestBench Radio RX program started\r\n");


    LEDS_OFF();
    LED_RED_ON();

    flag = 0;

    while (1)
    {
        radio_rx();

        LED_BLUE_OFF();
        LED_GREEN_OFF();
        while (flag == 0);
        LED_BLUE_ON();
        LED_GREEN_ON();

        flag = 0;

        radio_read_frame();
    }

    return 0;
}

#if USE_CC2420==0

void radio_init()
{
/* Initialize the radio driver */
    cc1101_init();
    cc1101_cmd_idle();

    cc1101_cfg_append_status(CC1101_APPEND_STATUS_DISABLE);
    cc1101_cfg_crc_autoflush(CC1101_CRC_AUTOFLUSH_DISABLE);
    cc1101_cfg_white_data(CC1101_DATA_WHITENING_ENABLE);
    cc1101_cfg_crc_en(CC1101_CRC_CALCULATION_ENABLE);
    cc1101_cfg_freq_if(0x0C);
    cc1101_cfg_fs_autocal(CC1101_AUTOCAL_NEVER);

    cc1101_cfg_mod_format(CC1101_MODULATION_MSK);

    cc1101_cfg_sync_mode(CC1101_SYNCMODE_30_32);

    cc1101_cfg_manchester_en(CC1101_MANCHESTER_DISABLE);

    // set channel bandwidth (560 kHz)
    cc1101_cfg_chanbw_e(0);
    cc1101_cfg_chanbw_m(2);

    // set data rate (0xD/0x2F is 250kbps)
    cc1101_cfg_drate_e(0x0D);
    cc1101_cfg_drate_m(0x2F);

    uint8_t table[] = {CC1101_868MHz_TX_m6dBm};
    cc1101_cfg_patable(table, 1);
    cc1101_cfg_pa_power(0);
}

void radio_rx()
{
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();
    cc1101_cmd_calibrate();

    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();
    cc1101_gdo0_register_callback(rxok_cb);

    cc1101_gdo2_int_disable();

    cc1101_cmd_rx();
}

uint16_t rxok_cb(void)
{
    flag = 1;
    return 1;
}

void radio_read_frame(void)
{
    if ( !(cc1101_status_crc_lqi() & 0x80) )
    {
        return;
    }

    /* Check Length is correct */
    cc1101_fifo_get( &rxlen, 1);
    if (rxlen > 64)
    {
        return ;
    }

    /* Check Addresses are correct */
    cc1101_fifo_get( rxframe, rxlen);

    int i;
    printf("Message received:\r\n");
    for (i=0;i<rxlen;i++)
    {
        printf("%c", rxframe[i]);
    }
    printf("\r\n");
}
#else


void radio_init()
{
    /* Initialize the radio driver */
    cc2420_init();
    cc2420_io_sfd_register_cb(rxok_cb);
    cc2420_io_sfd_int_set_falling();
    cc2420_io_sfd_int_clear();
    cc2420_io_sfd_int_enable();
}

void radio_rx()
{
    cc2420_cmd_idle();
    cc2420_cmd_flushrx();
    cc2420_cmd_rx();
}

uint16_t rxok_cb(void)
{
    if (cc2420_io_fifop_read())
    {
        flag = 1;
    }

    return 1;
}

void radio_read_frame(void)
{
    cc2420_fifo_get(&rxlen, 1);

    if ( rxlen < 128 )
    {
        cc2420_fifo_get(rxframe, rxlen);

        // check CRC
        if ( (rxframe[rxlen-1] & 0x80) != 0 )
        {

            int i;
            printf("Message received:\r\n");
            for (i=0;i<rxlen-2;i++)
            {
                printf("%c", rxframe[i]);
            }
            printf("\r\n");
        }
    }
}

#endif
