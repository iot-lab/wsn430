#include <io.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "interface.h"
#include "starnet_sink.h"

/* Drivers Includes */
#include "uart0.h"
#include "leds.h"

#define BLINKER_CMD '0'
#define SENSOR_CMD  '1'
#define LIST_CMD    '2'

/* Function Prototypes */
static void vInterfaceTask(void* pvParameters);
static uint16_t char_received(uint8_t c);
static void vPrintData(void);

xQueueHandle xCmdQueue, xRXQueue;

uint16_t packet_length;
uint16_t packet_from;
uint8_t packet[64];
uint8_t cmd[3];

void vCreateInterfaceTask( uint16_t usPriority )
{
    xCmdQueue = xQueueCreate(2, sizeof(cmd));
    xRXQueue = xQueueCreate(1, sizeof(packet));

    /* Create the task */
    xTaskCreate( vInterfaceTask, "interface", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vInterfaceTask(void* pvParameters)
{
    uart0_init(UART0_CONFIG_1MHZ_115200);
    uart0_register_callback(char_received);
    printf("Radio Monitor, Sink\r\n");

    for (;;)
    {
        if ( xQueueReceive(xRXQueue, packet, 50) )
        {
            vPrintData();
        }

        if ( xQueueReceive(xCmdQueue, cmd, 50) )
        {
            if (cmd[0] == BLINKER_CMD || cmd[0] == SENSOR_CMD)
            {
                xSendPacketTo(cmd[2], 2, cmd);
            }
            else
            {
                uint8_t* nodeList;
                uint16_t nodeNum = xGetAttachedNodes(&nodeList);
                printf("nodelist=");
                uint16_t i;
                for (i=0;i<nodeNum;i++)
                {
                    printf("%x:", nodeList[i]);
                }
                printf("\r\n");
            }

        }
    }
}

static void vPrintData(void)
{
    uint16_t i;

    #define SENSOR_PORT_NUMBER 0x0
    if ( packet[0]!=SENSOR_PORT_NUMBER)
    {
        return;
    }

    printf("From=%x\tLength=%u\t", packet_from, packet_length);
    for (i=0;i<packet_length; i++)
    {
        printf("%u:",packet[i+1]);
    }
    printf("\r\n");
}

static uint16_t char_received(uint8_t c)
{
    portBASE_TYPE xHighPriorityTaskWoken;
    static uint8_t dat[3] = {0, 0, 0};
    static uint16_t idx = 0;
    if (idx == 0)
    {
        if (c == BLINKER_CMD || c == SENSOR_CMD)
        {
            dat[0] = c;
            idx = 1;
        }
        else if (c == LIST_CMD)
        {
            dat[0] = c;

            /* Send the command to the MAC TX Queue */
            xQueueSendToBackFromISR( xCmdQueue, dat, &xHighPriorityTaskWoken);

            LED_GREEN_TOGGLE();

            /* If this enabled a higher priority to execute,
             * force a context switch */
            if (xHighPriorityTaskWoken)
            {
                vPortYield();
            }
        }
    }
    else
    {
        dat[idx] = c;
        idx ++;

        if (idx == 3)
        {
            idx = 0;

            /* Send the command to the MAC TX Queue */
            xQueueSendToBackFromISR( xCmdQueue, dat, &xHighPriorityTaskWoken);

            LED_GREEN_TOGGLE();

            /* If this enabled a higher priority to execute,
             * force a context switch */
            if (xHighPriorityTaskWoken)
            {
                vPortYield();
            }
        }
    }
    return 0;
}


int putchar(int c)
{
    return uart0_putchar(c);
}

void vPacketReceivedFrom(uint8_t srcAddr, uint16_t pktLength, uint8_t* pkt)
{
    packet_length = pktLength;
    packet_from = srcAddr;
    xQueueSendToBack(xRXQueue, pkt, 0);
}
