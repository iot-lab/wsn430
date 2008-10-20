
/**
 *  \file   ds1722.h
 *  \brief  DS1722 temperature sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/

#ifndef _DS1722_H_
#define _DS1722_H_

void ds1722_set_res(uint16_t res);
void ds1722_sample_1shot(void);
void ds1722_sample_cont(void);
void ds1722_stop(void);
uint8_t ds1722_read_MSB(void);
uint8_t ds1722_read_LSB(void);

uint8_t ds1722_read_cfg();
void ds1722_write_cfg(uint8_t c);

#endif

