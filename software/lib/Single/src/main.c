/*
 * Copyright  2008-2009 INRIA/SensTools
 *
 * <dev-team@sentools.info>
 *
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/**
 * @file main.c
 * @brief main file for SINGLE
 * @author Nathalie Mitton, Tony Ducrocq and David Simplot-Ryl
 * @version 0.3
 * @date July 25, 2010
 *
 * @todo improve idle mode
 * @todo improve tree building
 * @todo fix desynchronization bug in the MAC layer
 */

/**
 * Standard header files
 */
#include <io.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * WSN430 drivers
 */
#include "clock.h"
#include "uart0.h"
#include "leds.h"
#include "timerA.h"
#include "ds1722.h"
#include "tsl2550.h"
#include "m25p80.h"

/**
 * MAC layer: CSMA is used
 */
#include "mac.h"

/**
 * SINGLE header file
 */
#include "single.h"

/**********************************************************************/

uint8_t my_state;  /**< the state of the node */
uint8_t cpt_blank; /**< the number of cycles without packet reception
		      (has been introduced because of the
		      desynchronization misbehaviour in the MAC
		      layer) */

uint16_t num_slot;
uint16_t num_frame;
uint8_t my_radio_state;

uint8_t buffer_state;

// c'est mal
uint16_t cpt_last_beacon;

typedef struct {
  uint8_t index, next;
  uint8_t pkt[PKT_BUFFER_SIZE][PACKET_LENGTH_MAX];
  uint16_t length[PKT_BUFFER_SIZE];
  uint16_t dst_addr[PKT_BUFFER_SIZE];
} pkt_buffer_t;

pkt_buffer_t pkt_buffer;

/**********************************************************************/

void io_init(void);

/**********************************************************************/

int putchar(int c)
{
  return uart0_putchar(c);
}

/**********************************************************************/

int main(void)
{
    /* start initialization phase */
    my_state = STATE_INIT;

    WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer

    /* Configure the IO ports */
    io_init();

    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    set_aclk_div(1);

    LEDS_INIT();
    LEDS_OFF();
    LEDS_ON();

    // shutdown unused chips
    // temp sensor
    ds1722_init();
    ds1722_stop();

    // light sensor
    tsl2550_init();
    tsl2550_powerdown();

    // flash mem
    m25p80_init();
    m25p80_power_down();

    radio_on();

    uart0_init(UART0_CONFIG_1MHZ_115200);
    uart0_register_callback(char_rx);
    eint();

    printf("# I'm %.4x - channel=%u\r\n", node_addr, CHANNEL);

    my_state = STATE_DEFAULT;
    num_slot = 0;
    num_frame = 0;

    init_buffer(BUFFER_OFF);
    init_app();

    timerA_init();
    timerA_start_ACLK_div(TIMERA_DIV_8);
    timerA_register_cb(TIMERA_ALARM_CCR0, send_packet);
    timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, PERIOD, PERIOD);

    LEDS_OFF();
    /* end of initialization phase */

    /* infinite loop with low consumption mode */
    while(1)
      LPM3;
    return 0;
}

/**********************************************************************/

uint16_t char_rx(uint8_t c) {
    if (c == ' ') timerA_set_alarm_from_now(TIMERA_ALARM_CCR0, 12, 0);
    return 0;
}

/**********************************************************************/

void synchronize_mac(void)
{
  cpt_blank++;
  if ( cpt_blank >= BLANK_THRESHOLD )
    {
      mac_init(CHANNEL);
      mac_set_rx_cb(packet_received);
      mac_set_error_cb(packet_error);
      mac_set_sent_cb(packet_sent);
    }
}

/**********************************************************************/

void notice_packet_received_mac(void)
{
  cpt_blank = 0;
}

/**********************************************************************/

void io_init(void) {
  P1SEL = 0;
  P2SEL = 0;
  P3SEL = 0;
  P4SEL = 0;
  P5SEL = 0;
  P6SEL = 0;

  P1DIR = BV(0)+BV(1)+BV(2)+BV(5)+BV(6)+BV(7);
  P1OUT = 0;

  P2DIR = BV(0)+BV(1)+BV(2)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
  P2OUT = 0;

  P3DIR = BV(0)+BV(2)+BV(4)+BV(6)+BV(7);
  P3OUT = BV(2)+BV(4);

  P4DIR = BV(2)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
  P4OUT = BV(2)+BV(4);

  P5DIR = BV(0)+BV(1)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
  P5OUT = 0;

  P6DIR = BV(0)+BV(1)+BV(3)+BV(4)+BV(5)+BV(6)+BV(7);
  P6OUT = 0;
}

/**********************************************************************/

void init_buffer(uint8_t mode)
{
  if ( mode == BUFFER_ON )
    {
      buffer_state = BUFFER_ON;
      pkt_buffer.next = NO_PACKET;
      pkt_buffer.index = 0;
    }
  else
    buffer_state = BUFFER_OFF;
}

/**********************************************************************/

uint16_t my_send(uint8_t packet[], uint16_t length, uint16_t dst_addr)
{
  uint16_t tmp;

  /* first, test radio activity */
  if ( !my_radio_state )
    return 1;

  /* then, try to send it directly */
  if ( (tmp=mac_send(packet, length, dst_addr)) != 1 )
    /* success or too large packet => exit without action */
    return tmp;

  if ( buffer_state == BUFFER_OFF )
    return 1;

  /* packet needs to be put in packet buffer */
  if ( pkt_buffer.next == pkt_buffer.index )
    /* full buffer exit with error */
    return 1;

  memcpy(pkt_buffer.pkt[pkt_buffer.index], packet, length);
  pkt_buffer.length[pkt_buffer.index] = length;
  pkt_buffer.dst_addr[pkt_buffer.index] = dst_addr;
  if ( pkt_buffer.next == NO_PACKET )
    pkt_buffer.next = pkt_buffer.index;
  pkt_buffer.index++;
  if ( pkt_buffer.index >= PKT_BUFFER_SIZE )
    pkt_buffer.index= 0;

  return 0;
}

/**********************************************************************/

uint16_t try_next_in_buffer(void)
{
  uint16_t tmp;

  if ( buffer_state == BUFFER_OFF || pkt_buffer.next == NO_PACKET ||
       !my_radio_state )
    return 0;

  if ( (tmp=mac_send(pkt_buffer.pkt[pkt_buffer.next],
		     pkt_buffer.length[pkt_buffer.next],
		     pkt_buffer.dst_addr[pkt_buffer.next])) != 0 )
    /* not a success => keep trying... exit without action */
    return tmp;

  pkt_buffer.next = (pkt_buffer.next+1) % PKT_BUFFER_SIZE;
  if ( pkt_buffer.next == pkt_buffer.index )
    pkt_buffer.next = NO_PACKET;

  return 0;
}

/**********************************************************************/

void flush_buffer(void)
{
  pkt_buffer.next = NO_PACKET;
}

/**********************************************************************/

uint16_t packet_sent(void) {
  PRINTF("#> packet sent\r\n");
  return try_next_in_buffer();
}

/**********************************************************************/

uint16_t packet_error(void) {
  PRINTF("#> packet error\r\n");
  return try_next_in_buffer();
}

/**********************************************************************/

void radio_on()
{
  flush_buffer(); /* remove the packets stored in buffer since they are out of date */
  mac_init(CHANNEL);
  mac_set_rx_cb(packet_received);
  mac_set_error_cb(packet_error);
  mac_set_sent_cb(packet_sent);
  notice_packet_received_mac();
  my_radio_state = 1;
}

/**********************************************************************/

void radio_off()
{
  my_radio_state = 0;
  mac_stop();
}

/**********************************************************************/

uint16_t activity_schedule()
{
  num_slot++;
  if ( num_slot >= FRAME_SIZE )
    {
      num_slot = 0;
      if ( my_state == STATE_ACTIVE )
	{
	  cpt_last_beacon++;
	  if ( cpt_last_beacon >= THRESHOLD_BEACON_ANCHOR )
	    my_state = STATE_DEFAULT;
	}
      else
	{
	  // STATE_DEFAULT
	  num_frame++;
	  if ( num_frame >= FRAME_PERIOD )
	    num_frame = 0;
	}
    }

  PRINTF("#> frame=%d, slot=%d, mode=%d\n", num_frame, num_slot, my_state);
  if ( num_slot == 0 ){
    if ( my_state == STATE_ACTIVE || num_frame == 0 )
      {
	radio_on();
	PRINTF("#> mac on\n");
      }
    else
      if ( my_state == STATE_DEFAULT && num_frame == 1 )
	{
	  radio_off();
	  PRINTF("#> default mode: mac off\n");
	}
  }

  if ( my_state == STATE_ACTIVE && num_slot == ACTIVE_SLOTS )
    {
      radio_off();
      PRINTF("#> active mode: mac off\n");
    }

  return 0;
}

/**********************************************************************/
