#include <io.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "net.h"
#include "mac.h"
#include "sensor.h"
#include "blinker.h"

static uint8_t packet[64];

#define SENSOR_PORT_NUMBER 0x0

void vSendSensorData(uint16_t dataLength, uint8_t* data)
{
    packet[0] = SENSOR_PORT_NUMBER;

    uint16_t i;
    for (i=0;i<dataLength;i++)
    {
        packet[i+1] = data[i];
    }

    xSendPacket(dataLength+1, packet);
}

void vPacketReceived(uint16_t pktLength, uint8_t* pkt)
{
    if (pktLength != 2)
    {
        return;
    }

    switch (pkt[0])
    {
        case '1': //0x31:
            if (pktLength == 2)
            {
                vSensorRequest(pkt[1]);
            }
            break;
        case '0': //0x30:
            if (pktLength == 2)
            {
                vBlinkerRequest(pkt[1]);
            }
            break;
    }
}
