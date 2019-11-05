/*
 * File: handler.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#ifndef HANDLER_H
#define HANDLER_H
#define _ADDED_C_LIB 1
#include <stdio.h>
#include "lib/bc127.h"
#include "lib/log.h"
#include "lib/event.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"
#include "ui/bmbt.h"
#include "ui/cd53.h"
#include "ui/mid.h"
#define HANDLER_BLINKER_OFF 0
#define HANDLER_BLINKER_DRV 1
#define HANDLER_BLINKER_PSG 2
#define HANDLER_BT_CONN_OFF 0
#define HANDLER_BT_CONN_ON 1
#define HANDLER_BT_CONN_CHANGE 2
#define HANDLER_BT_SELECTED_DEVICE_NONE -1
#define HANDLER_CDC_ANOUNCE_INT 1000
#define HANDLER_CDC_ANOUNCE_TIMEOUT 21000
#define HANDLER_CDC_SEEK_MODE_NONE 0
#define HANDLER_CDC_SEEK_MODE_FWD 1
#define HANDLER_CDC_SEEK_MODE_REV 2
#define HANDLER_CDC_STATUS_INT 500
#define HANDLER_CDC_STATUS_TIMEOUT 20000
#define HANDLER_DEVICE_MAX_RECONN 10
#define HANDLER_INT_DEVICE_CONN 30000
#define HANDLER_POWER_OFF 0
#define HANDLER_POWER_ON 1
#define HANDLER_PROFILE_ERROR_INT 2500
#define HANDLER_SCAN_INT 10000

typedef struct HandlerContext_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t btDeviceConnRetries;
    int8_t btSelectedDevice;
    uint8_t btStatus;
    uint8_t btStatusCount;
    uint8_t btStartupIsRun;
    uint8_t btConnectionStatus;
    uint8_t uiMode;
    uint8_t seekMode;
    uint8_t blinkerStatus;
    uint8_t blinkerCount;
    uint8_t powerStatus;
    uint8_t scanIntervals;
    uint32_t cdChangerLastPoll;
    uint32_t cdChangerLastStatus;
} HandlerContext_t;
void HandlerInit(BC127_t *, IBus_t *);
void HandlerBC127Boot(void *, unsigned char *);
void HandlerBC127BootStatus(void *, unsigned char *);
void HandlerBC127CallStatus(void *, unsigned char *);
void HandlerBC127DeviceLinkConnected(void *, unsigned char *);
void HandlerBC127DeviceDisconnected(void *, unsigned char *);
void HandlerBC127DeviceFound(void *, unsigned char *);
void HandlerBC127PlaybackStatus(void *, unsigned char *);
void HandlerUICloseConnection(void *, unsigned char *);
void HandlerUIInitiateConnection(void *, unsigned char *);
void HandlerIBusCDCPoll(void *, unsigned char *);
void HandlerIBusCDCStatus(void *, unsigned char *);
void HandlerIBusIgnitionStatus(void *, unsigned char *);
void HandlerIBusLCMLightStatus(void *, unsigned char *);
void HandlerIBusLCMDimmerStatus(void *, unsigned char *);
void HandlerIBusMFLButton(void *, unsigned char *);
void HandlerIBusMFLVolume(void *, unsigned char *);
void HandlerTimerCDCAnnounce(void *);
void HandlerTimerCDCSendStatus(void *);
void HandlerTimerBTStatus(void *);
void HandlerTimerDeviceConnection(void *);
void HandlerTimerPoweroff(void *);
void HandlerTimerOpenProfileErrors(void *);
void HandlerTimerScanDevices(void *);
#endif /* HANDLER_H */
