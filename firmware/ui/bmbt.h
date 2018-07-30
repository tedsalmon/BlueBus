/*
 * File: bmbt.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the BoardMonitor UI Mode handler
 */
#ifndef BMBT_H
#define BMBT_H
#define _ADDED_C_LIB 1
#include <stdio.h>
#include "../lib/bc127.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
typedef struct BMBTContext_t {
    BC127_t *bt;
    IBus_t *ibus;
} BMBTContext_t;
void BMBTInit(BC127_t *, IBus_t *);
#endif /* BMBT_H */
