#ifndef _LOG_RSSI_H_
#define _LOG_RSSI_H_

#include <io.h>
#include <signal.h>
#include <stdio.h>


/* RSSI value must be treated differently for cc1101 and cc2420, so this function differ in both files */
void print_log_packet(uint16_t rxfrom, uint16_t rssi);


#endif // _LOG_RSSI_H_
