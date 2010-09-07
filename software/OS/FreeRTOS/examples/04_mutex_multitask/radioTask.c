#include <stdio.h>
#include <io.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "radioTask.h"

#include "cc1100.h"
#include "leds.h"


static uint16_t vEndOfTx_cb(void);
static void vRadioTask(void* pvParameters);

/* File variables */
static xSemaphoreHandle xSPIMutex, xTxSem;
static xQueueHandle xDataQueue;


void vCreateRadioTask(xQueueHandle xQueue, xSemaphoreHandle xMutex)
{
    /* Stores the handles */
    xDataQueue = xQueue;
    xSPIMutex = xMutex;
    
    /* Create a Semaphore for synchronization with radio interrupts */
    vSemaphoreCreateBinary(xTxSem);
    xSemaphoreTake(xTxSem, 0);
    
    /* Add the radio task to the scheduler */
    xTaskCreate(vRadioTask, (const signed char*) "Radio", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    
}

/**
 * The radio task.
 * \param pvParameters NULL is passed at start-up
 */
static void vRadioTask(void* pvParameters)
{
    /* Create the TX packet buffer, and associated variables */
    uint8_t tx_buffer[4];
    uint16_t tx_buffer_index = 0;
    uint8_t tx_length = 4;
    
    /* Take the mutex */
    xSemaphoreTake(xSPIMutex, portMAX_DELAY);
    LED_BLUE_ON();
    printf("Radio takes mutex\r\n");
    
    /* Initialize the radio chip driver, and configure the radio */
    
    cc1100_init();
    cc1100_cmd_idle();
    
    cc1100_cfg_append_status(CC1100_APPEND_STATUS_DISABLE);
    cc1100_cfg_crc_autoflush(CC1100_CRC_AUTOFLUSH_DISABLE);
    cc1100_cfg_white_data(CC1100_DATA_WHITENING_ENABLE);
    cc1100_cfg_crc_en(CC1100_CRC_CALCULATION_ENABLE);
    cc1100_cfg_freq_if(0x0C);
    cc1100_cfg_fs_autocal(CC1100_AUTOCAL_NEVER);

    cc1100_cfg_mod_format(CC1100_MODULATION_MSK);

    cc1100_cfg_sync_mode(CC1100_SYNCMODE_30_32);

    cc1100_cfg_manchester_en(CC1100_MANCHESTER_DISABLE);

    // set channel bandwidth (560 kHz)
    cc1100_cfg_chanbw_e(0);
    cc1100_cfg_chanbw_m(2);

    // set data rate (0xD/0x2F is 250kbps)
    cc1100_cfg_drate_e(0x0D);
    cc1100_cfg_drate_m(0x2F);

    uint8_t table[1];
    table[0] = 0xC2; // 10dBm
    cc1100_cfg_patable(table, 1);
    cc1100_cfg_pa_power(0);
    
    /* Give the mutex back */
    xSemaphoreGive(xSPIMutex);
    LED_BLUE_OFF();
    printf("Radio gives mutex\r\n");
    
    uint8_t data_item;
    
    /* Enter the infinite loop */
    while (1)
    {
        /* Wait for an item to be put in the queue */
        if ( xQueueReceive(xDataQueue, &data_item, portMAX_DELAY) )
        {
            /* Copy the item in the packet buffer */
            tx_buffer[tx_buffer_index] = data_item;
            tx_buffer_index ++;
            
            /* Check if the buffer is full */
            if (tx_buffer_index == 4)
            {
                tx_buffer_index = 0;
                
                /* Take the semaphore and send the packet */
                xSemaphoreTake(xSPIMutex, portMAX_DELAY);
                LED_BLUE_ON();
                printf("Radio takes mutex\r\n");
                
                cc1100_cmd_idle();
                cc1100_cmd_flush_tx();
                cc1100_cmd_calibrate();

                cc1100_cfg_gdo0(CC1100_GDOx_SYNC_WORD);
                cc1100_gdo0_int_set_falling_edge();
                cc1100_gdo0_int_clear();
                cc1100_gdo0_int_enable();
                cc1100_gdo0_register_callback(vEndOfTx_cb);
                
                cc1100_gdo2_int_disable();
                
                cc1100_fifo_put((&tx_length), 1);
                cc1100_fifo_put(tx_buffer, tx_length);
                
                cc1100_cmd_tx();
                
                printf("Radio gives mutex\r\n");
                LED_BLUE_OFF();
                
                /* Give the semaphore back */
                xSemaphoreGive(xSPIMutex);
                
                /* Wait until the packet has been successfully sent */
                while (! xSemaphoreTake(xTxSem, portMAX_DELAY));
            }
        }
    }
}

/**
 * Callback function called when a packet has been sent.
 */
static uint16_t vEndOfTx_cb(void)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken; 
    
    /* Give the semaphore, indicating the task the packer is sent */
    xSemaphoreGiveFromISR(xTxSem, &xHigherPriorityTaskWoken);
    
    return 1;
}
