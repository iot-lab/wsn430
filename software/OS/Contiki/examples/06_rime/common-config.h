#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

/*
 * Some Rime examples require the receiver node address to be set.
 * It is based on the UID of the node given by the DS2411 chip
 *
 * To configure the Rime address of your receiver node:
 *  + Compile the firmware with an arbitrary value
 *  + Connect to the receiver node serial link
 *  + Load it on the receiver node
 *  + Read the value
 *  + Update this file
 *  + Compile and update all the nodes
 *
 */

char receiver_node_rime_addr[] =  { 0x01, 0xcb }; // trailing 0x00 are useless

#endif /* __COMMON_CONFIG_H__ */
