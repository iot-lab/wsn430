#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Project includes */
#if USE_CC2420
    #warning using CC2420
    #include "cc2420.h"
    #define RADIO "CC2420"
#else
    #warning using CC1100
    #include "cc1100.h"
    #define RADIO "CC1100"
#endif

#include "clock.h"
#include "ds1722.h"
#include "ds2411.h"
#include "leds.h"
#include "m25p80.h"
#include "tsl2550.h"
#include "timerA.h"
#include "uart0.h"

#include "spi1.h"

#define MCP73861_STAT1_PIN (1<<1)
#define MCP73861_STAT2_PIN (1<<0)
    
void exec_test(void);
uint16_t mcp_sample(void);
uint16_t radio_init(void);
void radio_send(uint8_t* buffer, uint8_t buflen);

int putchar(int c)
{
    uart0_putchar((char) c);
    return c;
}

// Variables
uint16_t prev_on = 0, prev_fault = 0;
int mcp_state = 0;

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
    printf("\r\n\r\nClock and UART0 initialized\r\n");
    
    printf("Initializing DS1722... ");
    // Setup DS1722 temperature sensor
    ds1722_init();
    ds1722_set_res(9);
    ds1722_sample_cont();
    printf("OK\r\n");
    
    printf("Initializing DS2411... ");
    // Setup DS2411 unique ID
    ds2411_init();
    printf("OK\r\n");
    
    // Setup LEDs
    LEDS_INIT();
    LEDS_ON();
    
    printf("Initializing M25P80... ");
    // Setup M25P80 flash
    m25p80_init();
    m25p80_wakeup();
    printf("OK\r\n");
    
    
    // Setup MCP73861 battery charger
    P4SEL &= ~(MCP73861_STAT1_PIN | MCP73861_STAT2_PIN);
    P4DIR &= ~(MCP73861_STAT1_PIN | MCP73861_STAT2_PIN);
    
    
    printf("Initializing TSL2550... ");
    // Setup TSL2550 light sensor
    tsl2550_init();
    tsl2550_powerup();
    tsl2550_read_adc0();
    uart0_init(UART0_CONFIG_1MHZ_115200);
    printf("OK\r\n");
    
    
    printf("Initializing timerA... ");
    // Setup TimerA periodic alarm
    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8);
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 1024, 1024);
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR1, 512, 512);
    timerA_register_cb(TIMERA_ALARM_CCR1, mcp_sample);
    printf("OK\r\n");
    
    
    // Setup the radio
    printf("Initializing " RADIO "... ");
    if ( radio_init() )
    {
        ;
    } else
    {
        printf("OK\r\n");
    }
    
    
    /* Enable Interrupts */
    eint();
    
    LED_BLUE_OFF();
    
    while (1)
    {
        _BIS_SR(LPM0_bits);
        exec_test();
        
    }
    
    return 0;
}

void exec_test(void)
{
    static int test_num = 0;
    int i;
    
    printf("\r\n **** Test #%i ****\r\n", test_num);
    LED_BLUE_ON();
    
    printf("- HW address\t(DS2411)\t%x", ds2411_id.raw[1]);
    for (i=2;i<7;i++)
    {
        printf(":%x", ds2411_id.raw[i]);
    }
    printf("\r\n");
    
    
    printf("- Batt. Charger\t(MCP73861)\t");
    switch (mcp_state)
    {
        case 0xF0:
            printf("OK, charging\r\n");
            break;
        case 0xA0:
            printf("OK, charge complete\r\n");
            break;
        case 0x00:
            printf("Error, disconnected?\r\n");
            break;
        default:
            printf("Error, fault\r\n");
    
    }
    
    printf("- Flash mem\t(M25P80)\t");
    uint8_t sig;
    sig = m25p80_get_signature();
    if (sig == 0x13)
    {
        printf("OK\r\n");
    } else {
        printf("Error, signature = 0x%x\r\n", sig);
    }
    
    
    printf("- Temp. Sensor\t(DS1722)\t");
    uint8_t temp_h, temp_l;
    temp_h = ds1722_read_MSB();
    temp_l = ds1722_read_LSB();
    if (temp_h > 10 && temp_h < 40)
    {
        printf("OK, T=%d.%u C\r\n", temp_h, 5*(temp_l!=0));
    } else {
        printf("Error, T=%d.%u C\r\n", temp_h, 5*(temp_l!=0));
    }
    
    
    
    printf("- Light Sensor\t(TSL2550)\t");
    uint8_t adc0, adc1;
    tsl2550_init();
    adc0 = tsl2550_read_adc0();
    adc1 = tsl2550_read_adc1();
    uart0_init(UART0_CONFIG_1MHZ_115200);
    
    if ( !(adc0&0x80) || !(adc1&0x80) )
    {
        printf("Error (Invalid Result)\r\n");
    } else {
        printf("OK, adc0=%x, adc1=%x\r\n", adc0, adc1);
    }
    
    
    printf("- Radio\t\t("RADIO")\t");
    uint8_t msg[64];
    uint8_t msglen;
    
    msglen = sprintf(msg, "Test Echo #%i", test_num);
    radio_send(msg, msglen);
    printf("Sent: %s\r\n", msg);
    
    LED_BLUE_OFF();
    
    test_num ++;
}

#if USE_CC2420==0
uint16_t radio_init()
{
/* Initialize the radio driver */
    cc1100_init();
    cc1100_cmd_idle();

    cc1100_gdo0_int_disable();
    cc1100_gdo2_int_disable();
    
    cc1100_cfg_append_status(CC1100_APPEND_STATUS_DISABLE);
    cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
    cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
    cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
    cc1100_cfg_freq_if(0x0C);
    cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);

    cc1100_cfg_mod_format(CC1100_MODULATION_MSK);

    cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);

    cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);

    // set channel bandwidth (560 kHz)
    cc1100_cfg_chanbw_e(0);
    cc1100_cfg_chanbw_m(2);

    // set data rate (0xD/0x2F is 250kbps)
    cc1100_cfg_drate_e(0x0D);
    cc1100_cfg_drate_m(0x2F);

    uint8_t table[1];
    table[0] = 0x0F; // -20dBm
    cc1100_cfg_patable(table, 1);
    cc1100_cfg_pa_power(0);
    
    
    return 0;
}

void radio_send(uint8_t* buffer, uint8_t buflen)
{
    cc1100_cmd_idle();
    cc1100_cmd_flush_rx();
    cc1100_cmd_flush_tx();
    
    cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
    cc1100_cmd_calibrate();
    
    cc1100_fifo_put(&buflen, 1);
    cc1100_fifo_put(buffer, buflen);
    
    cc1100_cmd_tx();
    while (cc1100_gdo0_read() == 0);
    while (cc1100_gdo0_read() != 0);
    
}
#else

uint16_t radio_init()
{
    cc2420_init();
    
    uint16_t ver = cc2420_get_version();
    if (ver != 0x3000) {
        printf("error reading version (%x, should be 3000)\r\n", ver);
        return 1;
    }
    return 0;
}

void radio_send(uint8_t* buffer, uint8_t buflen)
{
    //~ printf("Radio Send()\r\n");
    cc2420_cmd_idle();
    cc2420_cmd_flushtx();
    
    //~ printf("cc2420_put()... ");
    
    buflen += 2;
    
    cc2420_fifo_put(&buflen, 1);
    cc2420_fifo_put(buffer, buflen-2);
    
    //~ printf("ok\r\ncc2420_tx()...");
    
    cc2420_cmd_tx();
    
    //~ printf("ok\r\n");
    uint16_t stat;
    
    do {
        stat = cc2420_get_status();
        //~ printf("status=%x\r\n", stat);
    } while (stat & CC2420_STATUS_TX_ACTIVE);
}

#endif

uint16_t mcp_sample(void)
{
    uint16_t mcp_on, mcp_fault;
    mcp_on = !(P4IN & MCP73861_STAT1_PIN);
    mcp_fault = !(P4IN & MCP73861_STAT2_PIN);
    
    if (mcp_on && prev_on)
    {
        mcp_state = 0xF0;
    } else if (!mcp_on && !prev_on)
    {
        mcp_state = 0x00;
    } else {
        mcp_state = 0xA0;
    }
    
    if (mcp_fault && prev_fault)
    {
        mcp_state |= 0xF;
    } else if (!mcp_fault && !prev_fault)
    {
        mcp_state |= 0x0;
    } else {
        mcp_state |= 0xA;
    }
    
    prev_on = mcp_on;
    prev_fault = mcp_fault;
    
    return 1;
}
