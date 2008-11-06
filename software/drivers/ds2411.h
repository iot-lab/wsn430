#ifndef DS2411_H
#define DS2411_H

/*
 *   MSB                                  LSB
 *   CRC : S5 : S4 : S3 : S2 : S1 : S0 : FAMILY
 */

typedef union
{
  uint8_t raw[8];
  struct
  {
    uint8_t crc;      /* MSB */
    uint8_t serial5;
    uint8_t serial4;
    uint8_t serial3;
    uint8_t serial2;
    uint8_t serial1;
    uint8_t serial0;
    uint8_t family;   /* LSB */
  };
} ds2411_serial_number_t;

/**
 * Variable where the serial number is stored after call
 * to ds2411_init() is made.
 */
extern ds2411_serial_number_t ds2411_id;

/**
 * Initialize ds2411 component and read its value.
 * The serial number is stored in ds2411_id.
 * \return 1 if ok, 0 if error
 */
uint16_t ds2411_init(void);

#endif
