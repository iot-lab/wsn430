#include <io.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "mac.h"
#include "queue_types.h"

/* Drivers Include */
#include "ds2411.h"
#include "timerB.h"
#include "simplephy.h"
#include "leds.h"

typedef struct {
    uint8_t src;
    uint8_t dst;
    uint8_t datalen;
    uint8_t data[2];
} rxframe_t;

typedef struct {
    uint8_t src;
    uint8_t dst;
    uint8_t datalen;
    uint8_t data[71];
} txframe_t;

/* Function Prototypes */
static void vMacTask(void* pvParameters);
static void frame_received(uint8_t frame[], uint16_t length);
static void frame_sent(void);
static void vParseFrame(void);
static void vSendFrame(void);

/* Local Variables */
static xQueueHandle xBlinkerQueue;
static xQueueHandle xSensorQueue;
static xQueueHandle xTXDataQueue;
static xQueueHandle xRXFrameQueue;
static xSemaphoreHandle xSendingSem;

static rxframe_t rx_frame;
static txframe_t tx_frame;
static rxdata_t rx_data;
static txdata_t tx_data;

static uint8_t local_addr;

void vCreateMacTask( xQueueHandle xBlinkerCmdQueue, xQueueHandle xSensorCmdQueue, xQueueHandle xMacDataQueue, uint16_t usPriority)
{
    /* Store the queue handles */
    xBlinkerQueue = xBlinkerCmdQueue;
    xSensorQueue = xSensorCmdQueue;
    xTXDataQueue = xMacDataQueue;
    
    /* Create a Semaphore for waiting end of TX */
    vSemaphoreCreateBinary( xSendingSem );
    /* Make sure the semaphore is taken */
    xSemaphoreTake( xSendingSem, 0 );
    
    /* Create a Queue for Received Frames */
    xRXFrameQueue = xQueueCreate(3, sizeof(rxframe_t));
    
    /* Create the task */
    xTaskCreate( vMacTask, "MAC", configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vMacTask(void* pvParameters)
{
    /* Initialize the unique electronic signature and read it */
    ds2411_init();
    local_addr = ds2411_id.serial0;
    
    /* Seed the random number generator */
    uint16_t seed;
    seed = ( ((uint16_t)ds2411_id.serial0) << 8) + (uint16_t)ds2411_id.serial1;
    srand(seed);
    
    /* Init the PHY layer module */ 
    phy_init();
    phy_register_frame_received_handler(frame_received);
    phy_register_frame_sent_notifier(frame_sent);
    phy_start_rx();
    
    for (;;)
    {
        /* Check if a frame has been received from the network */
        if (pdTRUE == xQueueReceive(xRXFrameQueue, (void*) &rx_frame, 20))
        {
            /* Enable RX again */
            phy_start_rx();
            
            /* Parse the received frame */
            vParseFrame();
        }
        
        /* Check if there is data to send on the network */
        if (pdTRUE == xQueueReceive(xTXDataQueue, (void*) &tx_data, 20))
        {
            //~ LED_RED_TOGGLE();
            
            /* Send the frame */
            vSendFrame();
            
            /* Wait until the frame is sent */
            xSemaphoreTake( xSendingSem, portMAX_DELAY);
            
            /* Enable RX again */
            phy_start_rx();
            
        }
    }
}

/**
 * This function is a callback passed to the PHY layer.
 * It's called when a frame is received.
 * \param frame pointer to the received frame
 * \param length number of bytes received
 */
static void frame_received(uint8_t frame[], uint16_t length)
{
    portBASE_TYPE xHighPriorityTaskWoken;
    
    /* Check if received length is not bigger than expected */
    if (length <= sizeof(rxframe_t))
    {
        /* Queue the received frame in the received frame queue */
        xQueueSendToBackFromISR(xRXFrameQueue, (void*)frame, &xHighPriorityTaskWoken);
        
        /* If adding an element to the queue enabled a higher priority to execute,
         * force a context switch */
        if (xHighPriorityTaskWoken)
        {
            vPortYield();
        }
    }
}

/**
 * This is a function callback, passed to the PHY layer.
 * It is called when a frame has been sent.
 */
static void frame_sent(void)
{
    portBASE_TYPE xHighPriorityTaskWoken;
    
    /* Give the semaphore to indicate to the task the frame has been sent */
    xSemaphoreGiveFromISR( xSendingSem, &xHighPriorityTaskWoken);
    
    
    /* If this enabled a higher priority to execute,
     * force a context switch */
    if (xHighPriorityTaskWoken)
    {
        vPortYield();
    }
}

/**
 * Function to parse a received frame, and to take action accordingly.
 */
static void vParseFrame(void)
{
    /* Check the data length is 2 */
    if (rx_frame.datalen != 2)
    {
        return;
    }
    
    /* Switch on the frame type , and pass the payload to the corresponding
     * Task via a queue. */
    switch (rx_frame.data[0])
    {
        case BLINKER_CMD_TYPE:
            xQueueSendToBack(xBlinkerQueue, (void*)rx_frame.data, 0);
            break;
        case SENSOR_CMD_TYPE:
            xQueueSendToBack(xSensorQueue, (void*)rx_frame.data, 0);
            break;
    }
}

/**
 * Function to add a header to a payload and send the created frame.
 */
static void vSendFrame(void)
{
    uint16_t frame_length;
    
    /* Fill the frame header */
    tx_frame.src = local_addr;
    tx_frame.dst = 0xFF;
    tx_frame.datalen = tx_data.length +2;
    
    /* Recopy the body of the message */
    uint16_t i;
    for (i=0; i<tx_frame.datalen; i++)
    {
        tx_frame.data[i] = ((uint8_t*)(&tx_data))[i];
    }
    
    /* Compute the new length */
    frame_length = tx_frame.datalen + 3;
    
    /* Send the frame. */
    phy_send_frame(&tx_frame, frame_length);
}
