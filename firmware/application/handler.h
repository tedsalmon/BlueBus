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
#define HANDLER_LCM_STATUS_BLINKER_OFF 0
#define HANDLER_LCM_STATUS_BLINKER_ON 1
#define HANDLER_LCM_TRIGGER_OFF 0
#define HANDLER_LCM_TRIGGER_ON 1
#define HANDLER_BT_CONN_OFF 0
#define HANDLER_BT_CONN_ON 1
#define HANDLER_BT_CONN_CHANGE 2
#define HANDLER_BT_SELECTED_DEVICE_NONE -1
#define HANDLER_CDC_ANOUNCE_TIMEOUT 21000
#define HANDLER_CDC_SEEK_MODE_NONE 0
#define HANDLER_CDC_SEEK_MODE_FWD 1
#define HANDLER_CDC_SEEK_MODE_REV 2
#define HANDLER_CDC_STATUS_TIMEOUT 20000
#define HANDLER_DEVICE_MAX_RECONN 15
#define HANDLER_INT_CDC_ANOUNCE 1000
#define HANDLER_INT_CDC_STATUS 500
#define HANDLER_INT_DEVICE_CONN 30000
#define HANDLER_INT_DEVICE_SCAN 10000
#define HANDLER_INT_LCM_IO_STATUS 20000
#define HANDLER_INT_PROFILE_ERROR 2500
#define HANDLER_INT_POWEROFF 1000
#define HANDLER_MFL_STATUS_OFF 0
#define HANDLER_MFL_STATUS_SPEAK_HOLD 1
#define HANDLER_POWER_OFF 0
#define HANDLER_POWER_ON 1
#define HANDLER_POWER_TIMEOUT_MILLIS 61000
#define HANDLER_TEL_DAC_VOL 0x44
#define HANDLER_TEL_STATUS_SET 0
#define HANDLER_TEL_STATUS_FORCE 1
#define HANDLER_TEL_STATUS_VOL_CHANGE 0xFF

typedef struct HandlerBodyModuleStatus_t {
    uint8_t lowSideDoors: 1;
    uint8_t doorsLocked: 1;
} HandlerBodyModuleStatus_t;

typedef struct HandlerLightControlStatus_t {
    uint8_t lightStatus: 1;
    uint8_t triggerStatus: 1;
    uint8_t leftBlinker: 1;
    uint8_t rightBlinker: 1;
    uint8_t blinkCount: 4;
} HandlerLightControlStatus_t;

typedef struct HandlerModuleStatus_t {
    uint8_t BMBT: 1;
    uint8_t DSP: 1;
    uint8_t GT: 1;
    uint8_t IKE: 1;
    uint8_t LCM: 1;
    uint8_t MID: 1;
    uint8_t RAD: 1;
} HandlerModuleStatus_t;

typedef struct HandlerContext_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t btDeviceConnRetries;
    int8_t btSelectedDevice;
    uint8_t btStartupIsRun;
    uint8_t btConnectionStatus;
    uint8_t uiMode;
    uint8_t seekMode;
    uint8_t lmDimmerChecksum;
    uint8_t mflButtonStatus;
    uint8_t telStatus;
    HandlerBodyModuleStatus_t bodyModuleStatus;
    HandlerLightControlStatus_t lightControlStatus;
    HandlerModuleStatus_t ibusModuleStatus;
    uint8_t powerStatus;
    uint8_t scanIntervals;
    uint32_t cdChangerLastPoll;
    uint32_t cdChangerLastStatus;
    uint32_t lmLastIOStatus;
} HandlerContext_t;

void HandlerInit(BC127_t *, IBus_t *);
void HandlerBC127Boot(void *, unsigned char *);
void HandlerBC127BootStatus(void *, unsigned char *);
void HandlerBC127CallStatus(void *, unsigned char *);
void HandlerBC127CallerID(void *, unsigned char *);
void HandlerBC127DeviceLinkConnected(void *, unsigned char *);
void HandlerBC127DeviceDisconnected(void *, unsigned char *);
void HandlerBC127DeviceFound(void *, unsigned char *);
void HandlerBC127PlaybackStatus(void *, unsigned char *);
void HandlerUICloseConnection(void *, unsigned char *);
void HandlerUIInitiateConnection(void *, unsigned char *);
void HandlerIBusCDCStatus(void *, unsigned char *);
void HandlerIBusDSPConfigSet(void *, unsigned char *);
void HandlerIBusFirstMessageReceived(void *, unsigned char *);
void HandlerIBusGMDoorsFlapsStatusResponse(void *, unsigned char *);
void HandlerIBusGTDIAIdentityResponse(void *, unsigned char *);
void HandlerIBusGTDIAOSIdentityResponse(void *, unsigned char *);
void HandlerIBusIKEIgnitionStatus(void *, unsigned char *);
void HandlerIBusIKESpeedRPMUpdate(void *, unsigned char *);
void HandlerIBusIKEVehicleType(void *, unsigned char *);
void HandlerIBusLMLightStatus(void *, unsigned char *);
void HandlerIBusLMDiagnosticsAcknowledge(void *, unsigned char *);
void HandlerIBusLMDimmerStatus(void *, unsigned char *);
void HandlerIBusLMIdentResponse(void *, unsigned char *);
void HandlerIBusLMRedundantData(void *, unsigned char *);
void HandlerIBusMFLButton(void *, unsigned char *);
void HandlerIBusModuleStatusResponse(void *, unsigned char *);
void HandlerIBusModuleStatusRequest(void *, unsigned char *);
void HandlerIBusRADVolumeChange(void *, unsigned char *);
void HandlerIBusTELVolumeChange(void *, unsigned char *);
void HandlerIBusBroadcastCDCStatus(HandlerContext_t *);
uint8_t HandlerIBusBroadcastTELStatus(HandlerContext_t *, unsigned char);
void HandlerTimerCDCAnnounce(void *);
void HandlerTimerCDCSendStatus(void *);
void HandlerTimerBTStatus(void *);
void HandlerTimerDeviceConnection(void *);
void HandlerTimerLCMIOStatus(void *);
void HandlerTimerOpenProfileErrors(void *);
void HandlerTimerPoweroff(void *);
void HandlerTimerScanDevices(void *);
#endif /* HANDLER_H */
