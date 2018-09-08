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
#define BMBT_DISPLAY_OFF 0
#define BMBT_DISPLAY_ON 1
#define BMBT_HEADER_BT 1
#define BMBT_HEADER_PB_STAT 2
#define BMBT_HEADER_DEV_NAME 6
#define BMBT_MENU_NONE 0
#define BMBT_MENU_MAIN 1
#define BMBT_MENU_DASHBOARD 2
#define BMBT_MENU_DEVICE_SELECTION 3
#define BMBT_MENU_SETTINGS 4
#define BMBT_MENU_IDX_DASHBOARD 0
#define BMBT_MENU_IDX_DEVICE_SELECTION 1
#define BMBT_MENU_IDX_SETTINGS 2
#define BMBT_MENU_IDX_SETTINGS_SCROLL_META 0
#define BMBT_MENU_IDX_SETTINGS_RESET_BT 1
#define BMBT_MENU_IDX_SETTINGS_BACK 2
#define BMBT_MENU_WRITE_DELAY 300
#define BMBT_MODE_OFF 0
#define BMBT_MODE_ACTIVE 1
#define BMBT_PAIRING_DEVICE_NONE -1
typedef struct BMBTContext_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t menu;
    uint8_t mode;
    uint8_t displayMode;
    uint8_t writtenIndices;
    int8_t selectedPairingDevice;
    int8_t activelyPairedDevice;
} BMBTContext_t;
void BMBTInit(BC127_t *, IBus_t *);
void BMBTBC127DeviceConnected(void *, unsigned char *);
void BMBTBC127DeviceDisconnected(void *, unsigned char *);
void BMBTBC127Metadata(void *, unsigned char *);
void BMBTBC127PlaybackStatus(void *, unsigned char *);
void BMBTBC127Ready(void *, unsigned char *);
void BMBTIBusBMBTButtonPress(void *, unsigned char *);
void BMBTIBusCDChangerStatus(void *, unsigned char *);
void BMBTIBusMenuSelect(void *, unsigned char *);
void BMBTRADUpdateMainArea(void *, unsigned char *);
void BMBTScreenModeUpdate(void *, unsigned char *);
void BMBTScreenModeSet(void *, unsigned char *);
#endif /* BMBT_H */
