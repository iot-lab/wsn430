/*
 * Copyright  2008-2009 INRIA/SensTools
 *
 * <dev-team@sentools.info>
 *
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

#include <io.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "starnet_node.h"
#include "leds.h"

/* Drivers Include */
#include "cc1101.h"
#include "ds2411.h"
#include "spi1.h"

#define UNKNOWN_ADDRESS 0xFF

#define EVENT_FRAME_RECEIVED 0x10
#define EVENT_FRAME_TO_SEND  0x20

#define FRAME_TYPE_ATTACH_REQUEST 0x01
#define FRAME_TYPE_ATTACH_REPLY   0x02
#define FRAME_TYPE_DATA           0x03

#define FRAME_LENGTH_ATTACH 3

#define STATE_ATTACHING 0x1
#define STATE_ATTACHED  0x2

typedef struct frame {
    uint8_t length;
    uint8_t srcAddr;
    uint8_t dstAddr;
    uint8_t type;
    uint8_t payload[MAX_PACKET_LENGTH];
} frame_t;

/* Function Prototypes */
static void vMacTask(void* pvParameters);
static void vInitMac(void);
static void vStartRx(void);
static void vSendAttachFrame(void);
static void vSendFrame(void);
static uint16_t xParseAttachFrame(void);
static void vParseFrame(void);
static uint16_t vRxOk_cb(void);
static uint16_t vTxOk_cb(void);

/* Local Variables */
static xQueueHandle xEventQ, xTXFrameQ;
static xSemaphoreHandle xSPIM, xSendingS;
static uint8_t nodeAddr, coordAddr;
static frame_t txFrame, rxFrame;
static uint16_t macState;

void vCreateMacTask( xSemaphoreHandle xSPIMutex, uint16_t usPriority )
{
    /* Stores the mutex handle */
    xSPIM = xSPIMutex;

    /* Create an Event Queue */
    xEventQ = xQueueCreate(TX_BUF_LENGTH + 5, sizeof(uint8_t));

    /* Create a frame Queue for the frames to send */
    xTXFrameQ = xQueueCreate(TX_BUF_LENGTH, sizeof(frame_t));

    /* Create a Semaphore for waiting end of TX */
    vSemaphoreCreateBinary( xSendingS );
    /* Make sure the semaphore is taken */
    xSemaphoreTake( xSendingS, 0 );

    /* Create the task */
    xTaskCreate( vMacTask, (const signed char*)"MAC", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

uint16_t xSendPacket(uint16_t pktLength, uint8_t* pkt)
{
    uint8_t event = EVENT_FRAME_TO_SEND;

    if (macState == STATE_ATTACHING || pktLength > MAX_PACKET_LENGTH)
    {
        return 0;
    }

    frame_t tempFrame;

    tempFrame.length = (uint8_t)3+pktLength;
    tempFrame.srcAddr = nodeAddr;
    tempFrame.dstAddr = coordAddr;
    tempFrame.type = FRAME_TYPE_DATA;

    uint16_t i;
    for (i = 0; i<pktLength; i++)
    {
        tempFrame.payload[i] = pkt[i];
    }

    if ( !xQueueSendToBack(xEventQ, &event, 0) )
    {
        return 0;
    }

    if ( !xQueueSendToBack(xTXFrameQ, &tempFrame, 0) )
    {
        return 0;
    }

    return 1;
}


static void vMacTask(void* pvParameters)
{
    uint8_t event;

    macState = STATE_ATTACHING;

    vInitMac();

    coordAddr = 0x0;

    /* Network Attachment Procedure */
    while (coordAddr == 0x0)
    {
        vSendAttachFrame();

        vStartRx();

        uint16_t RXTimeout = 1000;
        uint16_t time;
        while (1)
        {
            time = xTaskGetTickCount();

            if ( xQueueReceive(xEventQ, &event, RXTimeout) )
            {
                if (event == EVENT_FRAME_RECEIVED)
                {
                    if ( xParseAttachFrame() )
                    {
                        break;
                    }
                }
            }

            time = xTaskGetTickCount() - time;

            if (time < RXTimeout)
            {
                RXTimeout -= time;
            }
            else
            {
                break;
            }
        }
    }

    macState = STATE_ATTACHED;


    /* Packet Sending/Receiving */
    for (;;)
    {
        vStartRx();

        while ( xQueueReceive(xEventQ, &event, portMAX_DELAY))
        {
            if (event == EVENT_FRAME_RECEIVED)
            {
                vParseFrame();
            }
            else if (event == EVENT_FRAME_TO_SEND)
            {
                //~ LED_BLUE_ON();
                if ( xQueueReceive(xTXFrameQ, &txFrame, 0) )
                {
                    vSendFrame();
                }
                //~ LED_BLUE_OFF();
            }
            vStartRx();
        }
    }
}

static void vInitMac(void)
{
    /* Initialize the unique electronic signature and read it */
    ds2411_init();
    nodeAddr = ds2411_id.serial0;

    /* Seed the random number generator */
    uint16_t seed;
    seed = ( ((uint16_t)ds2411_id.serial0) << 8) + (uint16_t)ds2411_id.serial1;
    srand(seed);

    xSemaphoreTake(xSPIM, portMAX_DELAY);
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

    uint8_t table[] = {CC1101_868MHz_TX_5dBm};

    cc1101_cfg_patable(table, 1);
    cc1101_cfg_pa_power(0);

    cc1101_cmd_calibrate();

    xSemaphoreGive(xSPIM);

}

static void vStartRx(void)
{
    xSemaphoreTake(xSPIM, portMAX_DELAY);

    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();
    cc1101_cmd_calibrate();

    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();
    cc1101_gdo0_register_callback(vRxOk_cb);

    cc1101_gdo2_int_disable();

    cc1101_cmd_rx();

    xSemaphoreGive(xSPIM);
}

static void vSendAttachFrame(void)
{
    txFrame.length = FRAME_LENGTH_ATTACH;
    txFrame.srcAddr = nodeAddr;
    txFrame.dstAddr = UNKNOWN_ADDRESS;
    txFrame.type = FRAME_TYPE_ATTACH_REQUEST;

    vSendFrame();
}

static uint16_t xParseAttachFrame(void)
{
    xSemaphoreTake(xSPIM, portMAX_DELAY);

    /* Check CRC is correct */
    if ( !(cc1101_status_crc_lqi() & 0x80) )
    {
        xSemaphoreGive(xSPIM);
        return 0;
    }

    /* Check Length is correct */
    cc1101_fifo_get( (uint8_t*) &(rxFrame.length), 1);
    if (rxFrame.length != FRAME_LENGTH_ATTACH)
    {
        xSemaphoreGive(xSPIM);
        return 0;
    }

    /* Check Addresses are correct */
    cc1101_fifo_get( (uint8_t*) &(rxFrame.srcAddr), 3);
    if ( rxFrame.dstAddr != nodeAddr )
    {
        xSemaphoreGive(xSPIM);
        return 0;
    }

    /* Check Frame Type */
    if ( rxFrame.type != FRAME_TYPE_ATTACH_REPLY )
    {
        xSemaphoreGive(xSPIM);
        return 0;
    }

    coordAddr = rxFrame.srcAddr;

    xSemaphoreGive(xSPIM);
    return 1;
}

static void vParseFrame(void)
{
    xSemaphoreTake(xSPIM, portMAX_DELAY);

    /* Check CRC is correct */
    if ( !(cc1101_status_crc_lqi() & 0x80) )
    {
        xSemaphoreGive(xSPIM);
        return;
    }

    /* Check Length is correct */
    cc1101_fifo_get( (uint8_t*) &(rxFrame.length), 1);
    if (rxFrame.length > sizeof(rxFrame)-1)
    {
        xSemaphoreGive(xSPIM);
        return;
    }

    /* Check Addresses are correct */
    cc1101_fifo_get( (uint8_t*) &(rxFrame.srcAddr), 3);
    if ( (rxFrame.srcAddr != coordAddr) ||
        ( (rxFrame.dstAddr != nodeAddr) && (rxFrame.dstAddr != 0xFF) ) )
    {
        xSemaphoreGive(xSPIM);
        return;
    }

    /* Check Frame Type */
    if ( rxFrame.type != FRAME_TYPE_DATA )
    {
        xSemaphoreGive(xSPIM);
        return;
    }

    /* Get Payload */
    cc1101_fifo_get( rxFrame.payload, rxFrame.length - 3 );

    xSemaphoreGive(xSPIM);

    /* Transfer packet to higher layer */
    vPacketReceived(rxFrame.length-3, rxFrame.payload);
}

static void vSendFrame(void)
{
    uint16_t delay;

    xSemaphoreTake(xSPIM, portMAX_DELAY);
    cc1101_gdo0_int_disable();
    cc1101_gdo2_int_disable();

    /* Wait until CCA */
    while ( !(0x10 & cc1101_status_pktstatus() ) )
    {
        cc1101_cmd_idle();
        cc1101_cmd_flush_rx();
        cc1101_cmd_rx();

        delay = (rand() & 0x7F) +1;

        xSemaphoreGive(xSPIM);
        vTaskDelay( delay );
        xSemaphoreTake(xSPIM, portMAX_DELAY);
    }

    cc1101_cmd_idle();
    cc1101_cmd_flush_rx();
    cc1101_cmd_flush_tx();

    cc1101_cfg_gdo0(CC1101_GDOx_SYNC_WORD);
    cc1101_gdo0_int_set_falling_edge();
    cc1101_gdo0_int_clear();
    cc1101_gdo0_int_enable();
    cc1101_gdo0_register_callback(vTxOk_cb);

    cc1101_gdo2_int_disable();

    cc1101_fifo_put((uint8_t*)&txFrame, txFrame.length+1);

    cc1101_cmd_tx();

    xSemaphoreGive(xSPIM);

    //~ LED_GREEN_ON();
    /* Wait until frame is sent */
    if ( !xSemaphoreTake(xSendingS, 1000) )
    {
        printf("erreur semaphore, correcting\r\n");
        xSemaphoreGive(xSendingS);
        cc1101_reinit();
        vInitMac();
        xSemaphoreTake(xSendingS, 0);
    }
    //~ LED_GREEN_OFF();

    /* Little Delay */
    vTaskDelay(5);
}

static uint16_t vRxOk_cb(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken;
    uint8_t event = EVENT_FRAME_RECEIVED;

    xQueueSendToBackFromISR(xEventQ, &event, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        vPortYield();
    }

    return 1;
}

static uint16_t vTxOk_cb(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken;

    xSemaphoreGiveFromISR(xSendingS, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        vPortYield();
    }

    return 1;
}
