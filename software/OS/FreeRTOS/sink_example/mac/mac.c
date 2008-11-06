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

#define FRAME_TYPE_DATA 0xAA
#define FRAME_ADDR_BROADCAST 0xFF

typedef struct {
    uint8_t src;
    uint8_t dst;
    uint8_t datalen;
    uint8_t data[2];
} txframe_t;

typedef struct {
    uint8_t src;
    uint8_t dst;
    uint8_t datalen;
    uint8_t data[71];
} rxframe_t;

/* Function Prototypes */
static void vMacTask(void* pvParameters);
static void frame_received(uint8_t frame[], uint16_t length);
static void frame_sent(void);
static void vPassFrame(void);
static void vSendFrame(void);

/* Local Variables */
static xQueueHandle xTXDataQueue;
static xQueueHandle xRXDataQueue;
static xQueueHandle xRXFrameQueue;
static xSemaphoreHandle xSendingSem;

static rxframe_t rx_frame;
static txframe_t tx_frame;

static rxdata_t rx_data;
static txdata_t tx_data;

static uint8_t local_addr;

void vCreateMacTask( xQueueHandle xTXQueue, xQueueHandle xRXQueue, uint16_t usPriority)
{
    /* Store the queue handles */
    xTXDataQueue = xTXQueue;
    xRXDataQueue = xRXQueue;
    
    /* Create a Semaphore for waiting end of TX */
    vSemaphoreCreateBinary( xSendingSem );
    /* Make sure the semaphore is taken */
    xSemaphoreTake( xSendingSem, 0 );
    
    /* Create a Queue for Received Frames */
    xRXFrameQueue = xQueueCreate(1, sizeof(rxframe_t));
    
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
        if (xQueueReceive(xRXFrameQueue, (void*) &rx_frame, 200))
        {
            /* Enable RX again */
            phy_start_rx();
            
            /* Pass the received frame */
            vPassFrame();
        }
        
        /* Check if there is a frame to send on the network */
        if (xQueueReceive(xTXDataQueue, (void*) &tx_data, 200))
        {
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
 * Function to pass a received frame to the upper layer.
 */
static void vPassFrame(void)
{
    uint16_t i;
    
    for (i=0;i<rx_frame.datalen;i++)
    {
        ((uint8_t*)&rx_data)[i] = rx_frame.data[i];
    }
    
    xQueueSendToBack(xRXDataQueue, &rx_data,0);
}

/**
 * Function to add a header to a payload and send the created frame.
 */
static void vSendFrame(void)
{
    uint16_t i;
    
    /* Fill the frame header */
    tx_frame.src = local_addr;
    tx_frame.dst = 0xFF;
    tx_frame.datalen = 2;
    
    for (i=0;i<tx_frame.datalen;i++)
    {
        tx_frame.data[i] = ((uint8_t*)&tx_data)[i];
    }
    
    /* Send the frame. */
    phy_send_frame((uint8_t*)&tx_frame, tx_frame.datalen + 3);
}
