/**
 *  \file   tsl2550.h
 *  \brief  TSL2550 light sensor driver (available on wsn430v1.3b et .4 boards)
 *  \author Colin Chaballier
 *  \date   2008
 **/


#ifndef _TSL2550_H_
#define _TSL2550_H_

void tsl2550_init(void);
void tsl2550_powerdown(void);
unsigned char tsl2550_powerup(void);
void tsl2550_set_extended();
void tsl2550_set_standard();
unsigned char tsl2550_read_adc0(void);
unsigned char tsl2550_read_adc1(void);

#endif

