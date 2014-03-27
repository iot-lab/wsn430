#include "common-config.h"

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

unsigned char receiver_node_rime_addr[] =  { 0x01, 0xcb }; // trailing 0x00 are useless


