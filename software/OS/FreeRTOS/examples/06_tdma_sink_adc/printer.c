#include <io.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Project Includes */
#include "printer.h"
#include "tdma_coord.h"
#include "tdma_common.h"
#include "leds.h"

typedef struct {
	uint16_t seq;
	uint16_t length;
	uint16_t measures[4][6];
	uint16_t node;
} adc_data_t;

/* Function Prototypes */
static void vPrinterTask(void* pvParameters);
static void new_node(uint16_t node);
static void new_data(uint16_t node, uint8_t* data, uint16_t length);
static uint32_t inline adc_to_mv(uint16_t raw);

/* Local Variables */
static xQueueHandle xDataQueue;

void vCreatePrinterTask(uint16_t usPriority) {
	/* Create a Pkt data Queue */
	xDataQueue = xQueueCreate(6, sizeof(adc_data_t));

	/* Create the task */
	xTaskCreate(vPrinterTask, (const signed char * const ) "printer",
			configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vPrinterTask(void* pvParameters) {
	static adc_data_t rx_data;

	// Start MAC layer
	mac_set_node_associated_handler(new_node);
	mac_set_data_received_handler(new_data);

	LED_GREEN_OFF();
	for (;;) {
		if (xQueueReceive(xDataQueue, &rx_data, portMAX_DELAY) == pdTRUE) {
			//			for (i = 0; i < rx_data.length; i++) {
			printf("%.4x: %u: %u", rx_data.node, rx_data.seq, rx_data.length);
			//				printf("%.4u | %.4u | %.4u | %.4u", adc_to_mv(
			//						rx_data.measures[i][0]), adc_to_mv(
			//						rx_data.measures[i][1]), adc_to_mv(
			//						rx_data.measures[i][2]), adc_to_mv(
			//						rx_data.measures[i][3]));
			printf("\n");
			//			}
		}
	}
}

static void new_node(uint16_t node) {
	printf("new node: %4x\n", node);
}

static uint32_t inline adc_to_mv(uint16_t raw) {
	uint32_t temp;
	temp = raw;
	temp *= 2500;
	temp >>= 12;
	return temp;
}

static void new_data(uint16_t node, uint8_t* data, uint16_t length) {
	adc_data_t *adc = (adc_data_t*)data;
	adc->node = node; // Huge Hack :)
	xQueueSendToBack(xDataQueue, adc, 0);
}
