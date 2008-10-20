#ifndef SPI1_H
#define SPI1_H

static uint8_t spi1_tx_return_value;

/* Local Macros */
/**
 * wait until a byte has been received on spi port
 */
#define SPI1_WAIT_EORX() while ( (IFG2 & URXIFG1) == 0){}

/**
 * wait until a byte has been sent on spi port
 */
#define SPI1_WAIT_EOTX() while ( (IFG2 & UTXIFG1) == 0){}

#define CC1101_CS_PIN (1<<2)
#define DS1722_CS_PIN (1<<3)
#define M25P80_CS_PIN (1<<4)

#define CC1101_ENABLE()  P4OUT &= ~CC1101_CS_PIN
#define CC1101_DISABLE() P4OUT |=  CC1101_CS_PIN

#define DS1722_ENABLE()  P4OUT |=  DS1722_CS_PIN
#define DS1722_DISABLE() P4OUT &= ~DS1722_CS_PIN

#define M25P80_ENABLE()  P4OUT &= ~M25P80_CS_PIN
#define M25P80_DISABLE() P4OUT |=  M25P80_CS_PIN

/**
 * SPI on USART1 initialization procedure
 **/
#define SPI1_INIT() do \
{ \
  P5DIR  |=   (1<<1) | (1<<3); /* output for CLK and SIMO */ \
  P5DIR  &=  ~(1<<2);   /* input for SOMI */ \
  P5SEL  |=   (1<<1) | (1<<2) | (1<<3); /* SPI for all three */ \
\
  U1CTL = SWRST; /* SPI 1 software reset */ \
  U1CTL = CHAR | SYNC | MM | SWRST;  /* 8bit SPI master */ \
\
  U1TCTL = CKPH | SSEL_2 | STC;    /* clock delay, SMCLK */ \
\
  U1RCTL = 0; /* clear errors */ \
\
  U1BR0 = 0x2; /* baudrate = SMCLK/2 */ \
  U1BR1 = 0x0; \
\
  ME2 |= USPIE1; /* enable SPI module */ \
\
  IE2 &= ~(UTXIE1 | URXIE1); /* disable SPI interrupt */ \
\
  U1CTL &= ~(SWRST); /* clear reset */ \
 \
 /* CS IO pins configuration */ \
  P4SEL &= ~(CC1101_CS_PIN | DS1722_CS_PIN | M25P80_CS_PIN); \
  P4DIR |=  (CC1101_CS_PIN | DS1722_CS_PIN | M25P80_CS_PIN); \
 \
 /* disable peripherals */ \
  M25P80_DISABLE(); \
  CC1101_DISABLE(); \
  DS1722_DISABLE(); \
} while(0)

/* enable/disable macros for SPI peripherals */

/**
 * Select CC1100 on SPI bus.
 **/
#define SPI1_CC1100_ENABLE() do \
{ \
  M25P80_DISABLE(); \
  DS1722_DISABLE(); \
  CC1101_ENABLE(); \
} while (0)

/**
 * Deselect CC1100.
 **/
#define SPI1_CC1100_DISABLE() do \
{ \
  CC1101_DISABLE(); \
} while (0)


/**
 * select DS1722
 */
#define SPI1_DS1722_ENABLE() do \
{ \
  M25P80_DISABLE(); \
  CC1101_DISABLE(); \
  DS1722_DISABLE(); \
  U1CTL |= SWRST; \
  U1TCTL &= ~(CKPH); \
  U1CTL &= ~(SWRST); \
  DS1722_ENABLE(); \
} while (0)

/**
 * deselect DS1722
 */
#define SPI1_DS1722_DISABLE() do \
{ \
  DS1722_DISABLE(); \
  U1CTL |= SWRST; \
  U1TCTL |= CKPH; \
  U1CTL &= ~(SWRST); \
} while (0)


/**
 * select M25P80
 */
#define SPI1_M25P80_ENABLE() do \
{ \
    CC1101_DISABLE(); \
    DS1722_DISABLE(); \
    M25P80_ENABLE(); \
} while (0)
/**
 * deselect M25P80
 */
#define SPI1_M25P80_DISABLE() do \
{ \
  M25P80_DISABLE(); \
} while (0)

// tx/rx procedures
/**
 * send one byte on SPI port 1
 */
#define SPI1_TX(value) do  \
{ \
  U1TXBUF = (value); \
  SPI1_WAIT_EORX(); \
  spi1_tx_return_value = U1RXBUF; \
} while (0)

/**
 * receive one byte on SPI port 1
 */
#define SPI1_RX(value) do \
{ \
  U1TXBUF = 0; \
  SPI1_WAIT_EORX(); \
  (value) = U1RXBUF; \
} while (0)

/**
 * read SOMI line
 */
#define SPI1_READ_SOMI() (P5IN & (1<<2))

#endif
