
#include "log_rssi.h"

/* For cc1101
 * rssi value == rssi/2
 * with 0.5 precision
 */
void print_log_packet(uint16_t rxfrom, uint16_t rssi)
{
	printf("From %.4x [%i.%cdBm]: ", rxfrom, rssi/2, ((rssi & 1) ? '5' : '0'));
}
