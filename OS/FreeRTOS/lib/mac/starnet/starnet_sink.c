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
#include "starnet_sink.h"

/* Drivers Include */
#include "cc1101.h"
#include "ds2411.h"
#include "leds.h"

#define UNKNOWN_ADDRESS 0xFF

#define EVENT_FRAME_RECEIVED 0x10
#define EVENT_FRAME_TO_SEND  0x20

#define FRAME_TYPE_ATTACH_REQUEST 0x01
#define FRAME_TYPE_ATTACH_REPLY   0x02
#define FRAME_TYPE_DATA           0x03

#define FRAME_LENGTH_ATTACH 3

#define STATE_ATTACHING 0x1
#define STATE_RX        0x2
#define STATE_TX        0x3

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
static void vSendFrame(void);
static void vParseFrame(void);
static uint16_t vRxOk_cb(void);
static uint16_t vTxOk_cb(void);
static uint16_t xNodeInList(uint8_t nodeAddr);

/* Local Variables */
static xQueueHandle xEventQ;
static xSemaphoreHandle xSPIM, xSendingS;
static uint8_t coordAddr;
static frame_t txFrame, rxFrame;
static uint16_t macState;

static uint8_t nodeList[10];
static uint16_t nodeNumber;

void vCreateMacTask( xSemaphoreHandle xSPIMutex, uint16_t usPriority )
{
    /* Stores the mutex handle */
    xSPIM = xSPIMutex;

    /* Create an Event Queue */
    xEventQ = xQueueCreate(5, sizeof(uint8_t));

    /* Create a Semaphore for waiting end of TX */
    vSemaphoreCreateBinary( xSendingS );
    /* Make sure the semaphore is taken */
    xSemaphoreTake( xSendingS, 0 );

    /* Create the task */
    xTaskCreate( vMacTask, (const signed char*)"MAC", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

uint16_t xSendPacketTo(uint8_t dstAddr, uint16_t pktLength, uint8_t* pkt)
{
    uint8_t event = EVENT_FRAME_TO_SEND;

    if (pktLength > MAX_PACKET_LENGTH || macState != STATE_RX)
    {
        return 0;
    }

    txFrame.length = 3+pktLength;
    txFrame.srcAddr = coordAddr;
    txFrame.dstAddr = dstAddr;
    txFrame.type = FRAME_TYPE_DATA;

    uint16_t i;
    for (i = 0; i<pktLength; i++)
    {
        txFrame.payload[i] = pkt[i];
    }

    return xQueueSendToBack(xEventQ, &event, 0);
}


static void vMacTask(void* pvParameters)
{
    uint8_t event;

    macState = STATE_ATTACHING;

    vInitMac();


    /* Packet Sending/Receiving */
    for (;;)
    {
        vStartRx();
        macState = STATE_RX;

        if ( xQueueReceive(xEventQ, &event, portMAX_DELAY))
        {
            if (event == EVENT_FRAME_RECEIVED)
            {
                //~ LED_GREEN_ON();
                vParseFrame();
                //~ LED_GREEN_OFF();
            } else if (event == EVENT_FRAME_TO_SEND)
            {
                //~ LED_RED_ON();
                macState = STATE_TX;
                vSendFrame();
                //~ LED_RED_OFF();
            }
        }
    }
}

static void vInitMac(void)
{
    /* Reset attached node list */
    nodeNumber = 0;

    /* Initialize the unique electronic signature and read it */
    ds2411_init();
    coordAddr = ds2411_id.serial0;

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
    if ( (rxFrame.dstAddr != coordAddr) && (rxFrame.dstAddr != UNKNOWN_ADDRESS))
    {
        xSemaphoreGive(xSPIM);
        return;
    }

    xSemaphoreGive(xSPIM);

    /* Check Frame Type */
    switch ( rxFrame.type)
    {
        case FRAME_TYPE_ATTACH_REQUEST:
            if (rxFrame.length != FRAME_LENGTH_ATTACH)
            {
                break;
            }
            if ( !xNodeInList(rxFrame.srcAddr) )
            {
                if (nodeNumber == 10)
                {
                    break;
                }
                nodeList[nodeNumber] = rxFrame.srcAddr;
                nodeNumber++;
            }
            txFrame.srcAddr = coordAddr;
            txFrame.dstAddr = rxFrame.srcAddr;
            txFrame.type = FRAME_TYPE_ATTACH_REPLY;
            txFrame.length = FRAME_LENGTH_ATTACH;
            vSendFrame();
            break;

        case FRAME_TYPE_DATA:
            /* Get Payload */
            xSemaphoreTake(xSPIM, portMAX_DELAY);
            cc1101_fifo_get( rxFrame.payload, rxFrame.length - 3 );
            xSemaphoreGive(xSPIM);

            /* Transfer packet to higher layer */
            vPacketReceivedFrom(rxFrame.srcAddr, rxFrame.length-3, rxFrame.payload);
            break;

        default :
            break;
    }

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

    xSemaphoreTake(xSendingS, portMAX_DELAY);
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

static uint16_t xNodeInList(uint8_t nodeAddr)
{
    uint16_t i;
    for (i=0;i<nodeNumber;i++)
    {
        if (nodeList[i] == nodeAddr)
        {
            return 1;
        }
    }
    return 0;
}

uint16_t xGetAttachedNodes(uint8_t** nodeL)
{
    *nodeL = nodeList;
    return nodeNumber;
}
