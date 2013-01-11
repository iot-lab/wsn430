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
 * \file
 * \brief MCP73861 Battery Charger driver
 * \author Eric Fleury
 * \author Antoine Fraboulet
 * \date September 06
 */

#ifndef __MCP73861
#define __MCP73861


enum mcp73861_stat_t {
  MCP73861_OFF      = 0x00,
  MCP73861_ON       = 0xFF,
  MCP73861_FLASHING = 0xAA
};

#define MCP73861_STAT1  0xF0
#define MCP73861_STAT2  0x0F

#define MCP73861_STAT(stat1, stat2)\
  (MCP73861_STAT1 & stat1) |  (MCP73861_STAT2 & stat2)


enum mcp73861_result_t {
  MCP73861_QUALIFICATION              = MCP73861_STAT(MCP73861_OFF, MCP73861_OFF),
  MCP73861_PRECONDITIONING            = MCP73861_STAT(MCP73861_ON, MCP73861_OFF),
  MCP73861_CTE_CURRENT_FAST_CHARGE    = MCP73861_STAT(MCP73861_ON, MCP73861_OFF),
  MCP73861_CTE_VOLTAGE                = MCP73861_STAT(MCP73861_ON, MCP73861_OFF),
  MCP73861_CHARGE_COMPLETE            = MCP73861_STAT(MCP73861_FLASHING, MCP73861_OFF),
  MCP73861_FAULT                      = MCP73861_STAT(MCP73861_OFF, MCP73861_ON),
  MCP73861_THERM_INVALID              = MCP73861_STAT(MCP73861_OFF, MCP73861_FLASHING),
  MCP73861_DISABLE_SLEEP_MODE         = MCP73861_STAT(MCP73861_OFF, MCP73861_OFF),
  MCP73861_INPUT_VOLTAGE_DISCONNECTED = MCP73861_STAT(MCP73861_OFF, MCP73861_OFF)
};


extern void mcp73861_init(void);
extern enum mcp73861_result_t mcp73861_get_status(void);

#endif /* __MCP73861 */
