/**
 * \file queue_types.h
 * This header file contains the definitions of the different queue item
 * types used in the application.
 */

#ifndef _QUEUE_TYPES_H
#define _QUEUE_TYPES_H

#define MAX_DATA_LENGTH 70

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t data[MAX_DATA_LENGTH];
} rxdata_t;

typedef struct {
    uint8_t type;
    uint8_t cmd;
} txdata_t;

#endif
