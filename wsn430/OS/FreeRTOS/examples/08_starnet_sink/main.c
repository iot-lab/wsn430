#include <io.h>
#include <signal.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project includes */
#include "clock.h"
#include "uart0.h"

/* MAC task include */
#include "starnet_sink.h"

/* Hardware initialization */
static void prvSetupHardware( void );

/* Idle Hook prototype */
void vApplicationIdleHook(void);

/* Global Variables */
static xSemaphoreHandle xSPIMutex;

/* Task that sends packets */
static void vSendingTask(void* pvParameters);

/**
 * The main function.
 */
int main( void )
{
    /* Setup the hardware. */
    prvSetupHardware();
    
    /* Create the SPI mutex */
    xSPIMutex = xSemaphoreCreateMutex();
    
    /* Create the task of the application */
    vCreateMacTask(xSPIMutex, configMAX_PRIORITIES-1);

    /* Add the local task */
    xTaskCreate( vSendingTask, (const signed char*)"sender", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

    /* Start the scheduler. */
    vTaskStartScheduler();
    
    /* As the scheduler has been started we should never get here! */
    return 0;
}

/**
 * Initialize the main hardware parameters.
 */
static void prvSetupHardware( void )
{
    /* Stop the watchdog timer. */
    WDTCTL = WDTPW + WDTHOLD;
    
    /* Setup MCLK 8MHz and SMCLK 1MHz */
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    
    uart0_init(UART0_CONFIG_1MHZ_115200);
    
    printf("FreeRTOS Star Network, coordinator device\r\n");
    
    /* Enable Interrupts */
    eint();
}

int putchar(int c)
{
    return uart0_putchar(c);
}

void vApplicationIdleHook( void )
{
    _BIS_SR(LPM3_bits);
}

void vPacketReceivedFrom(uint8_t srcAddr, uint16_t pktLength, uint8_t* pkt)
{
    printf("Data received from node 0x%x (length = %d)\r\n", srcAddr, pktLength);
}

static void vSendingTask(void* pvParameters)
{
    while (1)
    {
        uint8_t *nodeList;
        uint16_t nodeNum;
        
        nodeNum = xGetAttachedNodes(&nodeList);
        uint16_t i;
        for (i=0; i<nodeNum; i++)
        {
            xSendPacketTo(nodeList[i], 4, (uint8_t*) "123");
            printf("frame sent to node 0x%x\r\n", nodeList[i]);
            vTaskDelay(1000);
        }
        vTaskDelay(1000);
    }
}
