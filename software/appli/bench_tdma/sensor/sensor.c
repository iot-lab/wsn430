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
#include "tdma_node.h"
#include "tdma_common.h"
#include "sensor.h"
#include "dataSensor.h"
#include "timerB.h"
#include "leds.h"

enum event {
	ASSOCIATED, MEASURE, TXREADY
};


/* Function Prototypes */
static void vSensorTask(void* pvParameters);
static void vSensorInit(void);
static void associated(void);
static void lost(void);
static void tx_ready(void);
static uint16_t measure_time(void);

/* Local Variables */
static xQueueHandle xDataQueue;
static sensor_data_t data_frame;

void vCreateSensorTask(uint16_t usPriority) {
	/* Create a Sensor data Queue */
	xDataQueue = xQueueCreate(2, sizeof(uint16_t));

	/* Create the task */
	xTaskCreate(vSensorTask, (const signed char * const ) "sensor",
			configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}

static void vSensorTask(void* pvParameters) {
	uint16_t event;
	uint16_t iv;

	vSensorInit();

	// Start MAC layer
	mac_send_command(MAC_ASSOCIATE);
	mac_set_event_handler(MAC_ASSOCIATED, associated);
	mac_set_event_handler(MAC_LOST, lost);
	mac_set_event_handler(MAC_TX_READY, tx_ready);

	LED_GREEN_OFF();
	for (;;) {
		if (xQueueReceive(xDataQueue, &event, portMAX_DELAY) == pdTRUE) {
			switch (event) {
			case ASSOCIATED:
				data_frame.length = 0;
				break;
			case MEASURE:
			  if (data_frame.length < SIZEDATABUF) {
			    for (iv = 0; iv < SIZEDATA; iv++) {
				data_frame.measures[data_frame.length][iv] = data_frame.seq+iv;
			    }
			    data_frame.length++;
			    data_frame.seq++;
			  } else {
			    data_frame.seq++;
			  }
				break;
			case TXREADY:
				if (data_frame.length > 0) {
					mac_send((uint8_t*) &data_frame, SIZEDATA + 2*SIZEDATA*data_frame.length);
					/*printf("%.4u \r\n",data_frame.measures[0][0]);*/

					data_frame.length = 0;
				}
				break;
			}
		}
	}
}

static void associated(void) {
	uint16_t event = ASSOCIATED;
	portBASE_TYPE woken = pdFALSE;

	printf("asso\n");

	xQueueSendToBackFromISR(xDataQueue, &event, &woken);

	if (woken == pdTRUE) {
		taskYIELD();
	}
}

static void lost(void) {
	printf("lost\n");
}

static void tx_ready(void) {
	uint16_t event = TXREADY;
	portBASE_TYPE woken = pdFALSE;
	xQueueSendToBackFromISR(xDataQueue, &event, &woken);

	if (woken == pdTRUE) {
		taskYIELD();
	}
}

static uint16_t measure_time(void) {
	uint16_t event = MEASURE;
	portBASE_TYPE woken = pdFALSE;
	xQueueSendToBackFromISR(xDataQueue, &event, &woken);

	if (woken == pdTRUE) {
		taskYIELD();
	}

	return 0;
}

static void vSensorInit(void) {
	uint16_t period;

	// Configure GPIO1 (P2.3) as output and high, as a voltage reference
	P2SEL &= ~BV(3);
	P2DIR |= BV(3);
	P2OUT |= BV(3);

	// Reset frame
	data_frame.length = 0;
	data_frame.seq = 0;

	// Start the timer
	period = MS_TO_TICKS(PERIOD_MS);
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR6, period, period);
	timerB_register_cb(TIMERB_ALARM_CCR6, measure_time);

}

