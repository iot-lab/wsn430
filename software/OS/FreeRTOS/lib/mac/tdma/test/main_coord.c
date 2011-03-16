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
#include "leds.h"

/* MAC task include */
#include "tdma_coord.h"

/* Hardware initialization */
static void prvSetupHardware(void);

static void vPrinterTask(void* pvParameters);
static void new_node(uint16_t node);
static void new_data(uint16_t node, uint8_t* data, uint16_t length);
static void beacon_tx(uint8_t id, uint16_t timestamp);

/* Global Variables */
static xSemaphoreHandle xSPIMutex;
static xQueueHandle rx_queue;
static uint8_t rx_frame[128];

/**
 * The main function.
 */
int main(void) {
	/* Setup the hardware. */
	prvSetupHardware();

	/* Create the SPI mutex */
	xSPIMutex = xSemaphoreCreateMutex();

	/* Create the task of the application */
	mac_create_task(xSPIMutex);

	/* Create a Pkt data Queue */
	rx_queue = xQueueCreate(6, sizeof(rx_frame));

	/* Create the task */
	xTaskCreate(vPrinterTask, (const signed char * const ) "printer",
			configMINIMAL_STACK_SIZE, NULL, 1, NULL );

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* As the scheduler has been started we should never get here! */
	return 0;
}

/**
 * Initialize the main hardware parameters.
 */
static void prvSetupHardware(void) {
	/* Stop the watchdog timer. */
	WDTCTL = WDTPW + WDTHOLD;

	/* Setup MCLK 8MHz and SMCLK 1MHz */
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();

	LEDS_INIT();
	LEDS_OFF();

	uart0_init(UART0_CONFIG_1MHZ_115200);
	printf("FreeRTOS TDMA coordinator test program\r\n");

	/* Enable Interrupts */
	eint();
}
uint16_t rx_node;
static void vPrinterTask(void* pvParameters) {
	// Start MAC layer
	mac_set_node_associated_handler(new_node);
	mac_set_data_received_handler(new_data);
	mac_set_beacon_handler(beacon_tx);

	uint8_t last = 0;

	while (1) {
		if (xQueueReceive(rx_queue, &rx_frame, portMAX_DELAY) == pdTRUE) {
			printf("r%u\n", rx_frame[0]);
		}
	}
}

int putchar(int c) {
	uart0_putchar(c);
	return c;
}

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) {
	_BIS_SR(LPM3_bits);
}

static void new_node(uint16_t node) {
	printf("#new node: %4x\n", node);
}

static void new_data(uint16_t node, uint8_t* data, uint16_t length) {
	rx_node = node;
	xQueueSendToBack(rx_queue, data, 0);
}
static void beacon_tx(uint8_t id, uint16_t timestamp) {
}
