#include <io.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "sensor.h"
#include "net.h"

/* Drivers Includes */
#include "ds1722.h"
#include "tsl2550.h"

#define SENSOR_MASK       (0x3 << 6)
#define SENSOR_NONE       (0x0 << 6)
#define SENSOR_LIGHT      (0x1 << 6)
#define SENSOR_TEMP       (0x2 << 6)
#define SENSOR_BOTH       (0x3 << 6)

#define RATE_MASK         (0x3 << 4)
#define RATE_2HZ          (0x0 << 4)
#define RATE_1HZ          (0x1 << 4)
#define RATE_0_5HZ        (0x2 << 4)
#define RATE_0_25HZ       (0x3 << 4)

/* Function Prototypes */
static void vSensorTask(void* pvParameters);
static void vSampleSensors(void);

/* Local Variables */
static xSemaphoreHandle xSPIMutex;
static uint16_t xSamplePeriod, activeSensors;

static struct
{
    uint16_t sequence;
    uint8_t  description;
    uint8_t  samples[64];
} packet;
uint16_t packet_length;

void vCreateSensorTask(xSemaphoreHandle xSPIMutexHandle, uint16_t usPriority)
{
    /* Store the command and data queue handle */
    xSPIMutex = xSPIMutexHandle;

    /* Create the task */
    xTaskCreate( vSensorTask, "sensor", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vSensorTask(void* pvParameters)
{
    portTickType xLastSampleTime = xTaskGetTickCount();

    /* Setup the ds1722 temperature sensor */
    xSemaphoreTake(xSPIMutex, portMAX_DELAY);
    ds1722_init();
    ds1722_set_res(12);
    ds1722_sample_cont();
    xSemaphoreGive(xSPIMutex);

    /* Setup the TSL2550 light sensor */
    tsl2550_init();
    tsl2550_powerup();

    activeSensors = SENSOR_NONE;
    packet.sequence = 0;
    vSensorRequest(0);

    for (;;)
    {
        vTaskDelayUntil(&xLastSampleTime, xSamplePeriod);
        vSampleSensors();
    }
}

void vSensorRequest(uint8_t cmd)
{
    activeSensors = (cmd & SENSOR_MASK);

    uint16_t rate;
    rate = ( (cmd & RATE_MASK) >> 4);
    xSamplePeriod = (0x1 << rate);
    xSamplePeriod *= 500;
}

static void vSampleSensors(void)
{
    switch (activeSensors)
    {
        case SENSOR_NONE:
            return;

        case SENSOR_LIGHT:
            tsl2550_init();
            packet.samples[0] = tsl2550_read_adc0();
            packet.samples[1] = tsl2550_read_adc1();
            packet_length = 5;
            break;

        case SENSOR_TEMP:
            packet.samples[0] = ds1722_read_MSB();
            packet.samples[1] = ds1722_read_LSB();
            packet_length = 5;
            break;

        case SENSOR_BOTH:
            packet.samples[0] = tsl2550_read_adc0();
            packet.samples[1] = tsl2550_read_adc1();
            packet.samples[2] = ds1722_read_MSB();
            packet.samples[3] = ds1722_read_LSB();
            packet_length = 7;
            break;
    }

    /* Send Packet */
    packet.description = activeSensors;
    packet.sequence ++;
    vSendSensorData(packet_length, (uint8_t*) &packet);
}
