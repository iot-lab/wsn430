#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

/*
 * Some Rime examples require the receiver node address to be set.
 * It is based on the UID of the node given by the DS2411 chip
 *
 * To configure the Rime address of your receiver node:
 *  + Compile and run the firmware with an arbitrary value on the receiver node
 *  + Read the RimeAddress via serial link
 *
 *  + Modify the address and compile
 *  + Update all the nodes
 *
 */

extern unsigned char receiver_node_rime_addr[]; // set in the 'common-config.c'

#endif /* __COMMON_CONFIG_H__ */
