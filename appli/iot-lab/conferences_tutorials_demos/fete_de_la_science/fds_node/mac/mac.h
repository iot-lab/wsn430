#ifndef _MAC_H
#define _MAC_H

/**
 * This node's address
 */
extern uint8_t node_addr;

/**
 * Initialize and create the MAC task.
 * \param usPriority priority of the task
 */
void vCreateMacTask(uint16_t usPriority);

#endif
