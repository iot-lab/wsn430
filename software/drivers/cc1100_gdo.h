/**
 * \file cc1100gdo.h
 * \brief CC1100 GDO PIN hardware abstraction
 * \author Clément Burin des Roziers
 * \date October 08
 */
#ifndef _CC1100_GDO_H
#define _CC1100_GDO_H

#define GDO0_PIN (1<<3)
#define GDO2_PIN (1<<4)

/**
 * Initialize IO PORT for GDO connectivity
 **/
#define GDO_INIT() do \
{ \
  P1SEL &= ~(GDO0_PIN | GDO2_PIN); \
  P1DIR &= ~(GDO0_PIN | GDO2_PIN); \
  P1IE  &= ~(GDO0_PIN | GDO2_PIN); \
} while (0)

/**
 * Enable Interrupt for GDO0 pin
 **/
#define GDO0_INT_ENABLE() P1IE |= GDO0_PIN

/**
 * Enable Interrupt for GDO2 pin
 **/
#define GDO2_INT_ENABLE() P1IE |= GDO2_PIN

/**
 * Disable Interrupt for GDO0 pin
 **/
#define GDO0_INT_DISABLE() P1IE &= ~GDO0_PIN

/**
 * Disable Interrupt for GDO2 pin
 **/
#define GDO2_INT_DISABLE() P1IE &= ~GDO2_PIN

/**
 * Clear interrupt flag for GDO0 pin
 **/
#define GDO0_INT_CLEAR() P1IFG &= ~GDO0_PIN
/**
 * Clear interrupt flag for GDO2 pin
 **/
#define GDO2_INT_CLEAR() P1IFG &= ~GDO2_PIN

/**
 * Set interrupt on rising edge for GDO0 pin
 **/
#define GDO0_INT_SET_RISING()  P1IES &= ~GDO0_PIN
/**
 * Set interrupt on falling edge for GDO0 pin
 **/
#define GDO0_INT_SET_FALLING() P1IES |=  GDO0_PIN
/**
 * Set interrupt on rising edge for GDO2 pin
 **/
#define GDO2_INT_SET_RISING()  P1IES &= ~GDO2_PIN
/**
 * Set interrupt on falling edge for GDO2 pin
 **/
#define GDO2_INT_SET_FALLING() P1IES |=  GDO2_PIN

/**
 * Read GDO0 pin value
 **/
#define GDO0_READ() P1IN & GDO0_PIN
/**
 * Read GDO2 pin value
 **/
#define GDO2_READ() P1IN & GDO2_PIN

#endif
