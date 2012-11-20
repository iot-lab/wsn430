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
#include "m25p80.h"

/* Define putchar for printf */
int putchar (int c)
{
    return uart0_putchar(c);
}


uint8_t page[256];

static void read_page(uint16_t p) {
    printf("Reading page #%i\r\n", p);
    m25p80_load_page(p, page);
    int i;
    for (i=0;i<256;i++)
    {
        printf("%.2x ",page[i]);
        if ((1+i)%32==0) printf("\r\n");
    }
}

int main(void)
{
    int16_t i;
    WDTCTL = WDTPW | WDTHOLD;
    set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz();
    uart0_init(UART0_CONFIG_1MHZ_115200);

    printf("M25P80 external flash memory test program\r\n");

    m25p80_init();

    uint8_t sig;

    sig = m25p80_get_signature();

    printf("signature is %x\r\n", sig);

    read_page(0);
    read_page(1);

    printf("Erasing sector #0\r\n");
    m25p80_erase_sector(0);

    read_page(0);
    read_page(1);

    printf("Writing 0->255 in page #0-1\r\n");
    for (i=0;i<256;i++)
    {
        page[i] = i;
    }

    m25p80_write(0x80, page, 256);

    for (i=0;i<256;i++) page[i] = 0xAA;


    read_page(0);
    read_page(1);

    while (1)
    {}
}
