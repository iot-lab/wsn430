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
#include "adc.h"
#include "timerB.h"
#include "leds.h"

enum event {
	ASSOCIATED, MEASURE, SEND
};

#define SAMPLE_MAX 8

typedef struct {
	uint16_t seq;
	uint16_t length;
	uint16_t measures[SAMPLE_MAX][6];
} adc_data_t;

/* Function Prototypes */
static void vADCTask(void* pvParameters);
static void vADCInit(void);
static void associated(void);
static void lost(void);
/* static void tx_ready(void); */ /* unused */
static uint16_t measure_time(void);

static void beacon_node(uint8_t id, uint16_t time);


/* Local Variables */
static xQueueHandle xDataQueue;
static uint16_t *ADC12MEMx = (uint16_t*) ADC12MEM;
static adc_data_t data_frame;

void vCreateADCTask(uint16_t usPriority) {
	/* Create a ADC data Queue */
	xDataQueue = xQueueCreate(2, sizeof(uint16_t));

	/* Create the task */
	xTaskCreate(vADCTask, (const signed char * const ) "adc",
			configMINIMAL_STACK_SIZE, NULL, usPriority, NULL );
}
static uint16_t inline adc_to_mv(uint16_t raw) {
	uint32_t temp;
	temp = raw;
	temp *= 2500;
	temp >>= 12;
	return temp;
}
static void vADCTask(void* pvParameters) {
	uint16_t event;
	vADCInit();

	// Start MAC layer
	mac_send_command(MAC_ASSOCIATE);
	mac_set_event_handler(MAC_ASSOCIATED, associated);
	mac_set_event_handler(MAC_LOST, lost);
	mac_set_beacon_handler(beacon_node);

	LED_GREEN_OFF();
	for (;;) {
		if (xQueueReceive(xDataQueue, &event, portMAX_DELAY) == pdTRUE) {
			switch (event) {
			case ASSOCIATED:
				vADCInit();
				data_frame.length = 0;
				break;
			case MEASURE:
				ADC12CTL0 |= ADC12SC;
				break;
			case SEND:
				if (data_frame.length > 0) {
					if (mac_send((uint8_t*) &data_frame, 4 + 12
							* data_frame.length)) {
						printf("%.4x: %u: %u\n", mac_addr, data_frame.seq,
								data_frame.length);
					}

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

static uint16_t measure_time(void) {
	uint16_t event = MEASURE;
	portBASE_TYPE woken = pdFALSE;
	xQueueSendToBackFromISR(xDataQueue, &event, &woken);

	if (woken == pdTRUE) {
		taskYIELD();
	}

	return 0;
}

static void vADCInit(void) {
	uint16_t period;

	// Configure GPIO1 (P2.3) as output and high, as a voltage reference
	P2SEL &= ~BV(3);
	P2DIR |= BV(3);
	P2OUT |= BV(3);

	// Reset frame
	data_frame.length = 0;
	data_frame.seq = 0;

	// Start the timer
	period = MS_TO_TICKS(10);
	timerB_set_alarm_from_now(TIMERB_ALARM_CCR6, period, period);
	timerB_register_cb(TIMERB_ALARM_CCR6, measure_time);

	/* Configure ADC pins for analog input */
	P6SEL |= BV(0) | BV(1) | BV(3) | BV(4) | BV(6) | BV(7);
	P6DIR &= ~(BV(0) | BV(1) | BV(3) | BV(4) | BV(6) | BV(7));

	/* 38us sampling, internal reference 1.5V */
	ADC12CTL0 = SHT0_7 | MSC | REFON | REF2_5V | ADC12ON;

	/* Start@ADD0, ADC12SC bit source, seq. of chan. */
	ADC12CTL1 = SHP | CONSEQ_1;

	/* Channel A0, vREF */
	ADC12MCTL0 = INCH_0 | SREF_1;

	/* Channel A1, vREF */
	ADC12MCTL1 = INCH_1 | SREF_1;

	/* Channel A4, vREF */
	ADC12MCTL2 = INCH_4 | SREF_1;

	/* Channel A3, vREF */
	ADC12MCTL3 = INCH_3 | SREF_1;

	/* Channel A6, vREF */
	ADC12MCTL4 = INCH_6 | SREF_1;

	/* Channel A7, vREF, End Of Sequence */
	ADC12MCTL5 = EOS | INCH_7 | SREF_1;

	/* Interrupt on end of sequence */
	ADC12IE = BV(5);

	/* Start ADC conversions */
	ADC12CTL0 |= ENC;
}

void adc12irq(void);
/**
 * ADC12 Interrupt service routine
 * Execute once the 6 ADCs have been sampled
 */
interrupt(ADC12_VECTOR) adc12irq(void) {
	uint16_t iv;
	iv = ADC12IV;

	if (iv == 16) {
		if (data_frame.length < SAMPLE_MAX) {
			for (iv = 0; iv < 6; iv++) {
				data_frame.measures[data_frame.length][iv] = ADC12MEMx[iv];
			}
			data_frame.length++;
			data_frame.seq++;

			if (data_frame.length == SAMPLE_MAX) {
				uint16_t event = SEND;
				portBASE_TYPE yield;
				xQueueSendToBackFromISR(xDataQueue, &event, &yield);
				if (yield) {
					taskYIELD();
				}
			}
		} else {
			data_frame.seq++;
			ADC12IFG = 0;
		}
	} else {
		ADC12IFG = 0;
	}

}



static void beacon_node(uint8_t id, uint16_t time) {
}

