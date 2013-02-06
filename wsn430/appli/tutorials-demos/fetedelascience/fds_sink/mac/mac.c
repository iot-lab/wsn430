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
static void vStartRx(void);
static void vParseFrame(void);
static uint16_t xRxOk_cb(void);

/* Local Variables */
xQueueHandle xEvent;

static frame_t rx_frame;

void vCreateMacTask( uint16_t usPriority)
{
    /* Create an Event Queue */
    xEvent = xQueueCreate(5, sizeof(uint8_t));

    /* Create the task */
    xTaskCreate( vMacTask, "MAC", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vMacTask(void* pvParameters)
{
    uint16_t rx_count;
    uint8_t event;
    vInitMac();

    for (;;)
    {
        vStartRx();

        rx_count = 0;

        while ( xQueueReceive(xEvent, &event, 15000) == pdTRUE)
        {
            if (event == EVENT_FRAME_RECEIVED)
            {
                rx_count ++;
                LED_GREEN_ON();
                vParseFrame();
                LED_GREEN_OFF();
            }

            if (rx_count == 128)
            {
                break;
            }
        }
        LED_RED_TOGGLE();
    }
}

static void vInitMac(void)
{
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

    uint8_t table[] = {CC1101_868MHz_TX_12dBm};
    cc1101_cfg_patable(table, 1);
    cc1101_cfg_pa_power(0);

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

    printf("node=%.2x|rssi=%d|temp=%d:%d|light=%d:%d", \
                rx_frame.addr, rssi, rx_frame.temp[0], \
                rx_frame.temp[1], rx_frame.light[0], \
                rx_frame.light[1]);
    printf("|num=%d", rx_frame.neigh_num);
    uint16_t i;
    for (i=0; i<rx_frame.neigh_num; i++)
    {
        printf("|%.2x", rx_frame.neighbors[i]);
    }
    printf("\r\n");
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

