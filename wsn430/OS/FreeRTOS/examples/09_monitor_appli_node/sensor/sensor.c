#include <io.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project Includes */
#include "sensor.h"
#include "queue_types.h"

/* Drivers Includes */
#include "ds1722.h"
#include "tsl2550.h"
#include "leds.h"

#define SENSOR_MASK       (0x3 << 6)
#define SENSOR_NONE       (0x0 << 6)
#define SENSOR_LIGHT      (0x1 << 6)
#define SENSOR_TEMP       (0x2 << 6)
#define SENSOR_BOTH       (0x3 << 6)

#define RATE_MASK         (0x3 << 4)
#define RATE_4HZ          (0x0 << 4)
#define RATE_8HZ         (0x1 << 4)
#define RATE_16HZ         (0x2 << 4)
#define RATE_32HZ         (0x3 << 4)

#define SEND_EVERY_MASK   (0x0F)


/* Function Prototypes */
static void vSensorTask(void* pvParameters);
static void vParseAndExecute(uint8_t cmd);
static void vSampleSensors(void);
static void vSendData(void);

/* Local Variables */
static xQueueHandle xCmdQueue, xDataQueue;
static portTickType xSamplePeriod;
static uint16_t blockOnCmd, dataReady, samplePerPacket, activeSensors;
static uint16_t filling_index;

static struct
{
    uint16_t sequence;
    uint8_t  description;
    uint8_t  samples[64];
} packet;

static txdata_t tx_data;

void vCreateSensorTask(xQueueHandle xSensorCmdQueue, xQueueHandle xMacDataQueue, uint16_t usPriority)
{
    /* Store the command and data queue handle */
    xCmdQueue = xSensorCmdQueue;
    xDataQueue = xMacDataQueue;
    
    /* Create the task */
    xTaskCreate( vSensorTask, (const signed char*) "sensor", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vSensorTask(void* pvParameters)
{
    portTickType xLastSampleTime;
    rxdata_t rx_data;
    
    /* Setup the ds1722 temperature sensor */
    ds1722_init();
    ds1722_set_res(12);
    ds1722_sample_cont();

    /* Setup the TSL2550 light sensor */
    tsl2550_init();
    tsl2550_powerup();
    
    blockOnCmd = 1;
    activeSensors = SENSOR_NONE;
    packet.sequence = 0;
    filling_index = 0;
    
    vParseAndExecute(0);
    
    for (;;)
    {
        if (blockOnCmd)
        {
            if (xQueueReceive(xCmdQueue, (void*)&rx_data, portMAX_DELAY))
            {
                vParseAndExecute(rx_data.cmd);
                xLastSampleTime = xTaskGetTickCount();
            }
        }
        else
        {
            vTaskDelayUntil(&xLastSampleTime, xSamplePeriod);
            vSampleSensors();
            if (dataReady)
            {
                //~ LED_GREEN_TOGGLE();
                vSendData();
            }
            
            if (xQueueReceive(xCmdQueue, (void*)&rx_data, 0))
            {
                vParseAndExecute(rx_data.cmd);
            }
        }
    }
}

static void vParseAndExecute(uint8_t cmd)
{
    blockOnCmd = 0;
    
    activeSensors = (cmd & SENSOR_MASK);
    
    if ( (cmd & SENSOR_MASK) == SENSOR_NONE)
    {
        blockOnCmd = 1;
    }
    
    uint16_t rate;
    rate = ( (cmd & RATE_MASK) >> 4) + 2;
    
    xSamplePeriod = (1000 >> rate);
    
    samplePerPacket = (cmd & SEND_EVERY_MASK) + 1;
    
    dataReady = 0;
    filling_index = 0;
}

static void vSampleSensors(void)
{
    switch (activeSensors)
    {
        case SENSOR_LIGHT:
            packet.samples[filling_index<<1]     = tsl2550_read_adc0();
            packet.samples[(filling_index<<1)+1] = tsl2550_read_adc1();
            filling_index ++;
            break;
        
        case SENSOR_TEMP:
            packet.samples[filling_index<<1]     = ds1722_read_MSB();
            packet.samples[(filling_index<<1)+1] = ds1722_read_LSB();
            filling_index ++;
            break;
        
        case SENSOR_BOTH:
            packet.samples[(filling_index<<2)]   = tsl2550_read_adc0();
            packet.samples[(filling_index<<2)+1] = tsl2550_read_adc1();
            packet.samples[(filling_index<<2)+2] = ds1722_read_MSB();
            packet.samples[(filling_index<<1)+3] = ds1722_read_LSB();
            filling_index ++;
            break;
    }
    
    if (filling_index == samplePerPacket)
    {
        filling_index = 0;
        dataReady = 1;
    }
}

static void vSendData(void)
{
    packet.description = activeSensors | (samplePerPacket-1);
    if (activeSensors == SENSOR_BOTH)
    {
        tx_data.length = 3 + (samplePerPacket << 2);
    }
    else
    {
        tx_data.length = 3 + (samplePerPacket << 1);
    }
    
    uint16_t i;
    for (i=0; i<tx_data.length; i++)
    {
        tx_data.data[i] = ((uint8_t*) (&packet))[i];
    }
    
    tx_data.type = 0x32;
    
    xQueueSendToBack(xDataQueue, &tx_data, 0);
    
    packet.sequence ++;
    dataReady = 0;
}
