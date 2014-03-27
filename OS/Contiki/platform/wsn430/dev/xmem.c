/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id: xmem.c,v 1.7 2008/07/03 23:12:10 adamdunkels Exp $
 */

/**
 * \file
 *         Device driver for the ST M25P80 40MHz 1Mbyte external memory.
 * \author
 *         Björn Grönvall <bg@sics.se>
 *
 *         Data is written bit inverted (~-operator) to flash so that
 *         unwritten data will read as zeros (UNIX style).
 */

#include <stdio.h>
#include <string.h>

#include <io.h>
#include <signal.h>

#include "contiki.h"

#include "dev/xmem.h"
#include "dev/watchdog.h"

#include "m25p80.h"

#if 0
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

/*---------------------------------------------------------------------------*/
/*
 * Initialize external flash *and* SPI bus!
 */
void xmem_init(void)
{
  m25p80_init();
}
/*---------------------------------------------------------------------------*/
int xmem_pread(void *_p, int size, unsigned long offset)
{
    int s;
    s = splhigh();

    m25p80_read(offset, _p, size);

    splx(s);
    return size;
}
/*---------------------------------------------------------------------------*/
int xmem_pwrite(const void *_buf, int size, unsigned long addr)
{
    int s;
    s = splhigh();

    m25p80_write(addr, (uint8_t*)_buf, (uint16_t) size);

    splx(s);
    return size;
}
/*---------------------------------------------------------------------------*/
int xmem_erase(long size, unsigned long addr)
{
    unsigned long end = addr + size;

    if(size % XMEM_ERASE_UNIT_SIZE != 0)
    {
        PRINTF("xmem_erase: bad size\n");
        return -1;
    }

    if(addr % XMEM_ERASE_UNIT_SIZE != 0)
    {
        PRINTF("xmem_erase: bad offset\n");
        return -1;
    }

    watchdog_stop();

    for (; addr < end; addr += XMEM_ERASE_UNIT_SIZE)
    {
        m25p80_erase_sector((uint8_t)(addr>>16));
    }

    watchdog_start();

    return size;
}
/*---------------------------------------------------------------------------*/
