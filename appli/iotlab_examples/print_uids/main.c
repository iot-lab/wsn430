/*
 * Copyright  2008-2009 SensTools, INRIA
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


#include <io.h>
#include <stdio.h>

#include "leds.h"
#include "clock.h"
#include "uart0.h"
#include "ds2411.h"

/* Define putchar for printf */
int putchar (int c)
{
	return uart0_putchar(c);
}


int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;
	set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
	uart0_init(UART0_CONFIG_1MHZ_115200);

	ds2411_init();

    printf("ds2411_uid == %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
        ds2411_id.raw[0], ds2411_id.raw[1], ds2411_id.raw[2], ds2411_id.raw[3],
        ds2411_id.raw[4], ds2411_id.raw[5], ds2411_id.raw[6], ds2411_id.raw[7]);
    printf("iotlab_uid == %02x%02x\n",
        ds2411_id.raw[5], ds2411_id.raw[6]);

    while (1) {
            nop();
    }
}
