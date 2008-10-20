/* Copyright (c) 2006 by ARES Inria.  All Rights Reserved */

/***
   NAME
     mcp73861
   PURPOSE
     
   NOTES
     
   HISTORY
     efleury - Sep 17, 2006: Created.
     $Log: mcp73861.h,v $
     Revision 1.1  2006-09-21 11:58:24  efleury
     testing the LI-Polymer Charge Managment Controller

***/
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
