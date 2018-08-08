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
#define BMBT_MENU_MAIN 0
#define BMBT_MENU_NOW_PLAYING 1
#define BMBT_MENU_DEVICE_SELECTION 2
#define BMBT_MENU_ABOUT 3
#define BMBT_MODE_OFF 0
#define BMBT_MODE_ACTIVE 1
typedef struct BMBTContext_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t menu;
    uint8_t mode;
} BMBTContext_t;
void BMBTInit(BC127_t *, IBus_t *);
void BMBTBC127Metadata(void *, unsigned char *);
void BMBTBC127PlaybackStatus(void *, unsigned char *);
void BMBTIBusBMBTButtonPress(void *, unsigned char *);
void BMBTIBusCDChangerStatus(void *, unsigned char *);
void BMBTIBusMenuSelect(void *, unsigned char *);
#endif /* BMBT_H */
