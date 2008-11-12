#ifndef M25P80_H
#define M25P80_H

#define M25P80_PAGE_SIZE              256  /* bytes */
#define M25P80_PAGE_NUMBER            4096

#define M25P80_SECTOR_SIZE            256  /* pages */
#define M25P80_SECTOR_NUMBER          16

/**
 * Configure IO pins for M25P80
 * and read device signature.
 * \return signature read (should be 0x13)
 */
uint8_t  m25p80_init(void);

/**
 *  Read device electronic signature
 * \return signature, sould be 0x13
 */
uint8_t  m25p80_get_signature(void);

/** 
 * Read and returns the M25P80 state register.
 * \return state register
 */
uint8_t  m25p80_get_state(void);

/**
 * Wake the M25P80 from Power Down
 */
void  m25p80_wakeup      (void);

/**
 * Put the M25P80 in Power Down mode.
 */
void  m25p80_power_down  (void);

/**
 * Erase a memory sector. (256 pages)
 * \param sector the sector number to erase
 */
void  m25p80_erase_sector(uint8_t sector);

/**
 * Erase the whole memory.
 * It takes at least 10s to complete.
 */
void  m25p80_erase_bulk  (void);

/**
 * Write a complete page to the memory. (256 bytes)
 * \param page the number of the page
 * \param buffer a pointer to a 256byte array
 */
void  m25p80_save_page(uint16_t page, const uint8_t *buffer);


/**
 * Read a complete page from the memory. (256 bytes)
 * \param page the number of the page
 * \param buffer a pointer to a 256byte array
 */
void  m25p80_load_page(uint16_t page, uint8_t *buffer);

#endif
