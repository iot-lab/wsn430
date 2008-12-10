/**
 *  \file   clock.h
 *  \brief  msp430 system clock 
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#ifndef CLOCK_H
#define CLOCK_H

void set_mcu_speed_dco_mclk_4MHz_smclk_1MHz (void);

void set_mcu_speed_xt2_mclk_2MHz_smclk_1MHz (void);
void set_mcu_speed_xt2_mclk_4MHz_smclk_1MHz (void);
void set_mcu_speed_xt2_mclk_8MHz_smclk_1MHz (void);
void set_mcu_speed_xt2_mclk_8MHz_smclk_8MHz (void);

/**
 * Set the ACLK clock divider from the 32768Hz external quartz.
 * \param div the divider, should be either 1/2/4/8
 */
void set_aclk_div                           (uint16_t div);

#endif
