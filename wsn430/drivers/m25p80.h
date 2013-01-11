/*
 * Copyright  2008-2009 INRIA/SensTools
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

/**
 * \defgroup m25p80 M25P80 serial flash memory driver
 * \ingroup wsn430
 * @{
 * 
 * The M25P80 from ST is an external 8Mbit flash memory.
 * It is connected to the MSP430 via the SPI1 port.
 * This driver allows read/write access to the chip. 
 *
 */

/**
 * \file
 * \brief M25P80 flash memory driver header
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date November 08
 */

#ifndef M25P80_H
#define M25P80_H

/**
 * \name M25P80 constants
 * @{
 */
/** \brief Page size in bytes */
#define M25P80_PAGE_SIZE              256  /* bytes */
/** \brief Number of pages */
#define M25P80_PAGE_NUMBER            4096

/** \brief Sector size in pages */
#define M25P80_SECTOR_SIZE            256  /* pages */
/** \brief Number of sectors */
#define M25P80_SECTOR_NUMBER          16

/**
 * @}
 */

/**
 * \brief Configure IO pins for M25P80 and read device signature.
 * 
 * This function must be called before any other of this module.
 * \return signature read (should be 0x13)
 */
uint8_t  m25p80_init(void);

/**
 * \brief Read device electronic signature
 * \return signature, sould be 0x13
 */
uint8_t  m25p80_get_signature(void);

/** 
 * \brief Read and returns the M25P80 state register.
 * \return state register
 */
uint8_t  m25p80_get_state(void);

/**
 * \brief Wake the M25P80 from Power Down
 */
void  m25p80_wakeup      (void);

/**
 * \brief Put the M25P80 in Power Down mode.
 */
void  m25p80_power_down  (void);

/**
 * \brief Erase a memory sector. (256 pages)
 * \param sector the sector number to erase
 */
void  m25p80_erase_sector(uint8_t sector);

/**
 * \brief Erase the whole memory.
 * 
 * It takes at least 10s to complete.
 */
void  m25p80_erase_bulk  (void);

/**
 * \brief Write data to the memory.
 * \param addr the address to start writing to.
 * \param buffer a pointer to the data to write
 * \param size the number of bytes to copy
 */
void m25p80_write(uint32_t addr, uint8_t *buffer, uint16_t size);


/**
 * \brief Read data from the memory
 * \param addr the address to start reading at
 * \param buffer a pointer to the buffer
 * \param size the number of bytes to copy
 */
void m25p80_read(uint32_t addr, uint8_t *buffer, uint16_t size);

/**
 * \brief Save a whole page of data to the memory.
 * \param page the memory page number to write to.
 * \param buffer a pointer to the data to copy
 */
#define m25p80_save_page(page, buffer) m25p80_write( ((uint32_t)(page))<<8, (buffer), 256)

/**
 * \brief Read data from the memory
 * \param page the memory page number to read from.
 * \param buffer a pointer to the buffer to store the data to.
 */
#define m25p80_load_page(page, buffer) m25p80_read ( ((uint32_t)(page))<<8, (buffer), 256)

#endif

/**
 * @}
 */

