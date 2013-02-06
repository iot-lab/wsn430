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
#include "starnet_node.h"

/* Hardware initialization */
static void prvSetupHardware( void );

/* Task that sends packets */
static void vSendingTask(void* pvParameters);

/* Global Variables */
static xSemaphoreHandle xSPIMutex;


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

    printf("FreeRTOS Star Network, network device\r\n");

    /* Enable Interrupts */
    eint();
}

int putchar(int c)
{
    return uart0_putchar(c);
}

void vApplicationIdleHook( void );
void vApplicationIdleHook( void )
{
    _BIS_SR(LPM0_bits);
}

void vPacketReceived(uint16_t pktLength, uint8_t* pkt)
{
    printf("Frame received (length=%u)\r\n", pktLength);
}

static void vSendingTask(void* pvParameters)
{
    while (1)
    {
        xSendPacket(sizeof("Test"), (uint8_t*)"Test");
        vTaskDelay(2000);
    }
}
