#include <io.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Project Includes */
#include "interface.h"
#include "queue_types.h"

/* Drivers Includes */
#include "uart0.h"

#define SAMPLE_DATA_TYPE 0x32
#define BLINKER_CMD_TYPE 0X30
#define SENSOR_CMD_TYPE  0x31

/* Function Prototypes */
static void vInterfaceTask(void* pvParameters);
static uint16_t char_received(uint8_t c);
static void vPrintData(void);

/* Local Variables */
static xQueueHandle xTXDataQueue, xRXDataQueue;

static rxdata_t rx_data;

void vCreateInterfaceTask( xQueueHandle xTXQueue, xQueueHandle xRXQueue, uint16_t usPriority)
{
    /* Store the queue handled */
    xTXDataQueue = xTXQueue;
    xRXDataQueue = xRXQueue;

    /* Create the task */
    xTaskCreate( vInterfaceTask, (const signed char*) "interface", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vInterfaceTask(void* pvParameters)
{

    uart0_init(UART0_CONFIG_8MHZ_115200);
    uart0_register_callback(char_received);

    for (;;)
    {
        if ( xQueueReceive(xRXDataQueue, &rx_data, portMAX_DELAY) )
        {
            vPrintData();
        }
    }
}

static void vPrintData(void)
{
    uint16_t i;
    if (rx_data.type != SAMPLE_DATA_TYPE)
    {
        return;
    }
    printf("Length=%u\t", rx_data.length);
    for (i=0;i<rx_data.length; i++)
    {
        printf("%u:",rx_data.data[i]);
    }
    printf("\r\n");
}

static uint16_t char_received(uint8_t c)
{
    portBASE_TYPE xHighPriorityTaskWoken;
    static uint8_t cmd[2] = {0, 0};
    static uint16_t first = 1;
    if (first)
    {
        if (c == BLINKER_CMD_TYPE || c == SENSOR_CMD_TYPE)
        {
            cmd[0] = c;
            first = 0;
        }
    }
    else
    {
        cmd[1] = c;
        first = 1;

        /* Send the command to the MAC TX Queue */
        xQueueSendToBackFromISR( xTXDataQueue, cmd, &xHighPriorityTaskWoken);
    }

    return 1;
}


int putchar(int c)
{
    return uart0_putchar(c);
}
