/*
 * File: handler_common.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Shared structs, defines and functions for the Handlers
 */
#ifndef HANDLER_CONTEXT_H
#define HANDLER_CONTEXT_H
#include "../lib/bt.h"
#include "../lib/ibus.h"
#include "../lib/pcm51xx.h"


#define HANDLER_BT_BOOT_OK 0
#define HANDLER_BT_BOOT_FAIL 1
// We want to default to MFB_H as this is how the application initializes
#define HANDLER_BT_BOOT_RESET HANDLER_BT_BOOT_OK
#define HANDLER_BT_BOOT_MFB_L 1
#define HANDLER_BT_BOOT_MFB_H 2

#define HANDLER_BT_SELECTED_DEVICE_NONE -1
#define HANDLER_BT_METADATA_TIMEOUT 2000
#define HANDLER_CDC_ANOUNCE_TIMEOUT 21000
#define HANDLER_CDC_SEEK_MODE_NONE 0
#define HANDLER_CDC_SEEK_MODE_FWD 1
#define HANDLER_CDC_SEEK_MODE_REV 2
#define HANDLER_CDC_STATUS_TIMEOUT 20000
#define HANDLER_DEVICE_MAX_RECONN 15
#define HANDLER_IBUS_MODULE_PING_STATE_OFF 0
#define HANDLER_IBUS_MODULE_PING_STATE_READY 1
#define HANDLER_IBUS_MODULE_PING_STATE_IKE 2
#define HANDLER_IBUS_MODULE_PING_STATE_GT 3
#define HANDLER_IBUS_MODULE_PING_STATE_MID 4
#define HANDLER_IBUS_MODULE_PING_STATE_RAD 5
#define HANDLER_IBUS_MODULE_PING_STATE_LM 6
#define HANDLER_IBUS_MODULE_PING_STATE_TEL 7
#define HANDLER_GT_STATUS_UNCHECKED 0
#define HANDLER_GT_STATUS_CHECKED 1
#define HANDLER_INT_BC127_STATE 1000
#define HANDLER_INT_CDC_ANOUNCE 1000
#define HANDLER_INT_CDC_STATUS 500
#define HANDLER_INT_DEVICE_CONN 30000
#define HANDLER_INT_DEVICE_SCAN 5000
#define HANDLER_INT_IBUS_PINGS 250
#define HANDLER_INT_TCU_STATE_CHANGE 100
#define HANDLER_INT_LCM_IO_STATUS 15000
#define HANDLER_INT_LIGHTING_STATE 1000
#define HANDLER_INT_BT_AVRCP_UPDATER 1000
#define HANDLER_INT_BT_AVRCP_UPDATER_METADATA 250
#define HANDLER_INT_PROFILE_ERROR 2500
#define HANDLER_INT_POWEROFF 1000
#define HANDLER_INT_VOL_MGMT 500
#define HANDLER_INT_BM83_POWER_RESET 200
#define HANDLER_INT_BM83_POWER_MFB_ON 150
#define HANDLER_INT_BM83_POWER_MFB_OFF 500
#define HANDLER_INT_PDC_DISTANCE 250
#define HANDLER_LCM_STATUS_BLINKER_OFF 0
#define HANDLER_LCM_STATUS_BLINKER_ON 1
#define HANDLER_LM_BLINK_OFF 0x00
#define HANDLER_LM_BLINK_LEFT 0x01
#define HANDLER_LM_BLINK_RIGHT 0x02
#define HANDLER_LM_COMF_BLINK_OFF 0x00
#define HANDLER_LM_COMF_BLINK_LEFT 0x01
#define HANDLER_LM_COMF_BLINK_RIGHT 0x02
#define HANDLER_LM_COMF_PARKING_OFF 0x00
#define HANDLER_LM_COMF_PARKING_ON 0x01
#define HANDLER_LM_EVENT_REFRESH 0x00
#define HANDLER_LM_EVENT_ALL_OFF 0x01
#define HANDLER_LM_EVENT_BLINK_OFF 0x02
#define HANDLER_LM_EVENT_BLINK_LEFT 0x03
#define HANDLER_LM_EVENT_BLINK_RIGHT 0x04
#define HANDLER_LM_EVENT_PARKING_OFF 0x05
#define HANDLER_LM_EVENT_PARKING_ON 0x06
#define HANDLER_LCM_TRIGGER_OFF 0
#define HANDLER_LCM_TRIGGER_ON 1
#define HANDLER_MFL_STATUS_OFF 0
#define HANDLER_MFL_STATUS_SPEAK_HOLD 1
#define HANDLER_POWER_OFF 0
#define HANDLER_POWER_ON 1
#define HANDLER_POWER_TIMEOUT_MILLIS 61000
#define HANDLER_TEL_DAC_VOL 0x44
#define HANDLER_TEL_MODE_AUDIO 0
#define HANDLER_TEL_MODE_TCU 1
#define HANDLER_TEL_STATUS_SET 0
#define HANDLER_TEL_STATUS_FORCE 1
#define HANDLER_TEL_STATUS_VOL_CHANGE 0xFF
#define HANDLER_WAIT_REV_VOL 1000
#define HANDLER_MONITOR_STATUS_UNSET 0
#define HANDLER_MONITOR_STATUS_POWERED_OFF 1
#define HANDLER_MONITOR_STATUS_POWERED_ON 2

#define HANDLER_VOLUME_DIRECTION_DOWN 0
#define HANDLER_VOLUME_DIRECTION_UP 1
#define HANDLER_VOLUME_MODE_LOWERED 0
#define HANDLER_VOLUME_MODE_NORMAL 1

typedef struct HandlerBodyModuleStatus_t {
    uint8_t lowSideDoors: 1;
    uint8_t doorsLocked: 1;
} HandlerBodyModuleStatus_t;

typedef struct HandlerLightControlStatus_t {
    uint8_t blinkStatus: 2;
    uint8_t blinkCount: 8;
    uint8_t comfortBlinkerStatus: 2;
    uint8_t comfortParkingLampsStatus: 1;
} HandlerLightControlStatus_t;

typedef struct HandlerContext_t {
    BT_t *bt;
    IBus_t *ibus;
    uint8_t btDeviceConnRetries;
    int8_t btSelectedDevice: 4;
    uint8_t btStartupIsRun: 1;
    uint8_t btBootState: 2;
    uint8_t ibusModulePingState: 3;
    uint8_t mflButtonStatus: 1;
    uint8_t seekMode: 2;
    uint8_t volumeMode: 1;
    uint8_t gtStatus: 1;
    uint8_t monitorStatus: 2;
    uint8_t uiMode;
    uint8_t lmDimmerChecksum;
    uint8_t telStatus;
    HandlerBodyModuleStatus_t gmState;
    HandlerLightControlStatus_t lmState;
    uint8_t powerStatus;
    uint8_t scanIntervals;
    uint8_t tcuStateChangeTimerId;
    uint8_t lightingStateTimerId;
    uint8_t avrcpRegisterStatusNotifierTimerId;
    uint8_t bm83PowerStateTimerId;
    uint32_t cdChangerLastPoll;
    uint32_t cdChangerLastStatus;
    uint32_t gearLastStatus;
    uint32_t lmLastIOStatus;
    uint32_t lmLastStatusSet;
    uint32_t pdcLastStatus;
    uint8_t pdcActive: 1;
} HandlerContext_t;

uint8_t HandlerGetTelMode(HandlerContext_t *);
uint8_t HandlerSetIBusTELStatus(HandlerContext_t *, unsigned char);
void HandlerSetVolume(HandlerContext_t *, uint8_t);
#endif /* HANDLER_CONTEXT_H */
