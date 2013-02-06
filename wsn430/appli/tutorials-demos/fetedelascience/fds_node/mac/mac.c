#include <io.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "mac.h"

/* Drivers Include */
#include "cc1101.h"
#include "ds2411.h"
#include "ds1722.h"
#include "tsl2550.h"
#include "timerB.h"
#include "leds.h"
#include "uart0.h"

#define EVENT_FRAME_RECEIVED 0x10
#define EVENT_TIMEOUT 0x20

#define RSSI_THRESHOLD 190

typedef struct {
    uint8_t length;
    uint8_t addr;
    uint8_t temp[2];
    uint8_t light[2];
    uint8_t neigh_num;
    uint8_t neighbors[10];
} frame_t;

/* Function Prototypes */
static void vMacTask(void* pvParameters);
static void vInitMac(void);
static void vEraseTable(void);
static void vStartRx(void);
static void vParseFrame(void);
static void vReadSensors(void);
static void vSendFrame(void);

static uint16_t xRxOk_cb(void);
static uint16_t xTxOk_cb(void);

static uint16_t xTimeout_cb(void);

/* Local Variables */
xQueueHandle xEvent;
xSemaphoreHandle xSendingSem;

static frame_t rx_frame, tx_frame;
uint8_t node_addr;

void vCreateMacTask( uint16_t usPriority)
{
    /* Create an Event Queue */
    xEvent = xQueueCreate(5, sizeof(uint8_t));

    /* Create a Semaphore for waiting end of TX */
    vSemaphoreCreateBinary( xSendingSem );
    /* Make sure the semaphore is taken */
    xSemaphoreTake( xSendingSem, 0 );

    /* Create the task */
    xTaskCreate( vMacTask, "MAC", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vMacTask(void* pvParameters)
{
    vInitMac();

    for (;;)
    {
        vEraseTable();
        vStartRx();

        uint8_t event;
        while ( xQueueReceive(xEvent, &event, portMAX_DELAY))
        {
            if (event == EVENT_FRAME_RECEIVED)
            {
                LED_BLUE_ON();
                vParseFrame();
                LED_BLUE_OFF();
            } else if (event == EVENT_TIMEOUT)
            {
                if (tx_frame.neigh_num == 0)
                {
                    LED_GREEN_ON();
                } else
                {
                    LED_GREEN_OFF();
                }
                break;
            }
        }
        vReadSensors();
        //~ LED_RED_ON();
        vSendFrame();
        LED_RED_OFF();
        xSemaphoreTake(xSendingSem, portMAX_DELAY);

    }
}

static void vInitMac(void)
{
    /* Initialize the unique electronic signature and read it */
    ds2411_init();
    node_addr = ds2411_id.serial0;

    printf("Address = %.2x\n", node_addr);

    tx_frame.addr = node_addr;

    /* Seed the random number generator */
    uint16_t seed;
    seed = ( ((uint16_t)ds2411_id.serial0) << 8) + (uint16_t)ds2411_id.serial1;
    srand(seed);

    /* TimerB init */
    timerB_init();
    timerB_start_SMCLK_div(TIMERB_DIV_8);
    timerB_register_cb(TIMERB_ALARM_CCR0, xTimeout_cb);
    timerB_set_alarm_from_now(TIMERB_ALARM_CCR0, 65535, 65535);

    /* Leds */
    LEDS_INIT();
    LEDS_OFF();

    /* Initialize the radio driver */
    cc1101_init();
    cc1101_cmd_idle();

    cc1101_cfg_append_status(CC1101_APPEND_STATUS_ENABLE);
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

    uint8_t table[1];
    table[0] = 0xC2; // 10dBm
    cc1101_cfg_patable(table, 1);
    cc1101_cfg_pa_power(0);

    /* Setup the ds1722 temperature sensor */
    ds1722_init();
    ds1722_set_res(12);
    ds1722_sample_cont();

    /* Setup the TSL2550 light sensor */
    tsl2550_init();
    tsl2550_powerup();
    uart0_init(UART0_CONFIG_1MHZ_115200);

}

static void vEraseTable(void)
{
    tx_frame.neigh_num = 0;
}

static void vStartRx(void)
{
    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();
    cc1101_cmd_calibrate();

    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();
    cc1101_gdo0_register_callback(xRxOk_cb);

    cc1101_gdo2_int_disable();

    cc1101_cmd_rx();
}

static void vParseFrame(void)
{
    // verify CRC result
    if ( !(cc1101_status_crc_lqi() & 0x80) )
    {
        cc1101_cmd_flush_rx();
        cc1101_cmd_rx();
        return;
    }

    cc1101_fifo_get(&(rx_frame.length), 1);

    if (rx_frame.length > sizeof(rx_frame)-1)
    {
        cc1101_cmd_flush_rx();
        cc1101_cmd_rx();
        return;
    }

    cc1101_fifo_get(&(rx_frame.addr), rx_frame.length);

    uint8_t rssi;
    cc1101_fifo_get(&rssi, 1);

    cc1101_cmd_flush_rx();
    cc1101_cmd_rx();

    /* Compare RSSI */
    rssi += 0x80;
    if (rssi >= RSSI_THRESHOLD)
    {
        tx_frame.neighbors[tx_frame.neigh_num] = rx_frame.addr;
        tx_frame.neigh_num ++;
    }
    //~ printf("node %x rssi %d\r\n", rx_frame.addr, rssi);
}

static void vReadSensors(void)
{
    tsl2550_init();
    tx_frame.light[0] = tsl2550_read_adc0();
    tx_frame.light[1] = tsl2550_read_adc1();
    uart0_init(UART0_CONFIG_1MHZ_115200);

    tx_frame.temp[0] = ds1722_read_MSB();
    tx_frame.temp[1] = ds1722_read_LSB();
}

static void vSendFrame(void)
{
    uint16_t delay;

    cc1101_gdo0_int_disable();
    cc1101_gdo2_int_disable();

    tx_frame.length = 0x6 + tx_frame.neigh_num;

    /* Wait until CCA */
    while ( !(0x10 & cc1101_status_pktstatus() ) )
    {

        cc1101_cmd_idle();
        cc1101_cmd_flush_rx();
        cc1101_cmd_flush_tx();
        cc1101_cmd_rx();

        delay = (rand() & 0x7F) +1;
        vTaskDelay( delay );
    }

    LED_RED_ON();

    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();

    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();
    cc1101_gdo0_register_callback(xTxOk_cb);

    cc1101_gdo2_int_disable();

    cc1101_fifo_put((uint8_t*)&tx_frame, tx_frame.length+1);

    cc1101_cmd_tx();
}

static uint16_t xRxOk_cb(void)
{
    uint16_t xHigherPriorityTaskWoken;
    uint8_t event = EVENT_FRAME_RECEIVED;

    xQueueSendToBackFromISR(xEvent, &event, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        vPortYield();
        return 1;
    }

    return 0;
}

static uint16_t xTxOk_cb(void)
{
    uint16_t xHigherPriorityTaskWoken;

    xSemaphoreGiveFromISR(xSendingSem, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        vPortYield();
        return 1;
    }

    return 0;
}

static uint16_t xTimeout_cb(void)
{
    uint16_t xHigherPriorityTaskWoken;
    uint8_t event = EVENT_TIMEOUT;

    xQueueSendToBackFromISR(xEvent, &event, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        vPortYield();
        return 1;
    }

    return 0;
}
