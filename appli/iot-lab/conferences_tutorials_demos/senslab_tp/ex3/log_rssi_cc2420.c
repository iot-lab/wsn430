#include "log_rssi.h"

/* For cc2420
 * rssi value == rssi
 * with 1.0 precision
 */
void print_log_packet(uint16_t rxfrom, uint16_t rssi)
{
	printf("From %.4x [%idBm]: ", rxfrom, rssi);
}
