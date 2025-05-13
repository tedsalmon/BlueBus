/*
 * File: bmbt.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the BoardMonitor UI Mode handler
 */
#ifndef BMBT_H
#define BMBT_H
#include <stdio.h>
#include "../lib/bt/bt_bc127.h"
#include "../lib/bt/bt_bm83.h"
#include "../lib/bt.h"
#include "../lib/config.h"
#include "../lib/event.h"
#include "../lib/i2c.h"
#include "../lib/ibus.h"
#include "../lib/locale.h"
#include "../lib/pcm51xx.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "../lib/wm88xx.h"
#define BMBT_DISPLAY_OFF 0x00
#define BMBT_DISPLAY_TONE_SEL_INFO 0x01
#define BMBT_DISPLAY_EXTERNAL_INIT 0x02
#define BMBT_DISPLAY_EXTERNAL 0x03
#define BMBT_DISPLAY_ON 0x04
#define BMBT_DISPLAY_TEXT_LEN 9
#define BMBT_HEADER_BT 1
#define BMBT_HEADER_PB_STAT 2
#define BMBT_HEADER_TEMPS 5
#define BMBT_HEADER_DEV_NAME 6
#define BMBT_MENU_NONE 0
#define BMBT_MENU_MAIN 1
#define BMBT_MENU_DASHBOARD 2
#define BMBT_MENU_DASHBOARD_FRESH 255
#define BMBT_MENU_DEVICE_SELECTION 3
#define BMBT_MENU_SETTINGS 4
#define BMBT_MENU_SETTINGS_ABOUT 5
#define BMBT_MENU_SETTINGS_AUDIO 6
#define BMBT_MENU_SETTINGS_COMFORT 7
#define BMBT_MENU_SETTINGS_COMFORT_TIME 8
#define BMBT_MENU_SETTINGS_COMFORT_NAVI 9
#define BMBT_MENU_SETTINGS_CALLING 10
#define BMBT_MENU_SETTINGS_UI 11
#define BMBT_MENU_IDX_BACK 7
#define BMBT_MENU_IDX_DASHBOARD 0
#define BMBT_MENU_IDX_DEVICE_SELECTION 1
#define BMBT_MENU_IDX_SETTINGS 2
#define BMBT_MENU_IDX_SETTINGS_ABOUT 0
#define BMBT_MENU_IDX_SETTINGS_AUDIO 1
#define BMBT_MENU_IDX_SETTINGS_CALLING 2
#define BMBT_MENU_IDX_SETTINGS_COMFORT 3
#define BMBT_MENU_IDX_SETTINGS_UI 4
/* About Menu */
#define BMBT_MENU_IDX_SETTINGS_ABOUT_FW_VERSION 0
#define BMBT_MENU_IDX_SETTINGS_ABOUT_BUILD_DATE 1
#define BMBT_MENU_IDX_SETTINGS_ABOUT_SERIAL 2
#define BMBT_MENU_IDX_SETTINGS_ABOUT_URL 3
/* Audio Settings */
#define BMBT_MENU_IDX_SETTINGS_AUDIO_AUTOPLAY 0
#define BMBT_MENU_IDX_SETTINGS_AUDIO_DAC_GAIN 1
#define BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT 2
#define BMBT_MENU_IDX_SETTINGS_AUDIO_MANAGE_VOL 3
#define BMBT_MENU_IDX_SETTINGS_AUDIO_REV_VOL 4
/* Call Settings */
#define BMBT_MENU_IDX_SETTINGS_CALLING_HFP 0
#define BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN 1
#define BMBT_MENU_IDX_SETTINGS_CALLING_VOL_OFFSET 2
#define BMBT_MENU_IDX_SETTINGS_CALLING_MODE 3
/* Comfort Settings */
#define BMBT_MENU_IDX_SETTINGS_COMFORT_LOCK 0
#define BMBT_MENU_IDX_SETTINGS_COMFORT_UNLOCK 1
#define BMBT_MENU_IDX_SETTINGS_COMFORT_BLINKERS 2
#define BMBT_MENU_IDX_SETTINGS_COMFORT_PARKING_LAMPS 3
#define BMBT_MENU_IDX_SETTINGS_COMFORT_NAVI 4
#define BMBT_MENU_IDX_SETTINGS_COMFORT_TIME 5
#define BMBT_MENU_IDX_SETTINGS_COMFORT_PDC 6
/* Comfort Settings -> Navi */
#define BMBT_MENU_IDX_SETTINGS_COMFORT_NAVI_AUTOZOOM 0
#define BMBT_MENU_IDX_SETTINGS_COMFORT_NAVI_MAP 1
#define BMBT_MENU_IDX_SETTINGS_COMFORT_NAVI_SILENT 2
#define BMBT_MENU_IDX_SETTINGS_COMFORT_NAVI_RANGE 3
/* Comfort Settings -> Time */
#define BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_SOURCE 0
#define BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_DST 1
#define BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_OFFSET 2
#define BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_GPSDATE 4
#define BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_GPSTIME 5
/* UI Settings */
#define BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU 0
#define BMBT_MENU_IDX_SETTINGS_UI_METADATA_MODE 1
#define BMBT_MENU_IDX_SETTINGS_UI_TEMPS 2
#define BMBT_MENU_IDX_SETTINGS_UI_DASH_OBC 3
#define BMBT_MENU_IDX_SETTINGS_UI_MONITOR_OFF 4
#define BMBT_MENU_IDX_SETTINGS_UI_LANGUAGE 5

#define BMBT_MENU_BUFFER_OK 0
#define BMBT_MENU_BUFFER_FLUSH 1

#define BMBT_MAIN_AREA_LEN 9
#define BMBT_MENU_IDX_CARPLAY 0
#define BMBT_MENU_IDX_PAIRING_MODE 1
#define BMBT_MENU_IDX_CLEAR_PAIRING 2
#define BMBT_MENU_IDX_FIRST_DEVICE 3
#define BMBT_MENU_WRITE_DELAY 300
#define BMBT_MENU_TIMER_WRITE_INT 100
#define BMBT_MENU_TIMER_WRITE_TIMEOUT 500
#define BMBT_HEADER_TIMER_WRITE_INT 100
#define BMBT_HEADER_TIMER_WRITE_TIMEOUT 500
#define BMBT_MENU_HEADER_TIMER_OFF 255
/* 23 + 1 for null terminator */
#define BMBT_MENU_STRING_MAX_SIZE 24
#define BMBT_METADATA_MODE_OFF 0x00
#define BMBT_METADATA_MODE_PARTY 0x01
#define BMBT_METADATA_MODE_CHUNK 0x02
#define BMBT_MODE_INACTIVE 0
#define BMBT_MODE_ACTIVE 1
#define BMBT_NAV_BOOT 0x10
#define BMBT_NAV_STATE_BOOT 0x00
#define BMBT_NAV_STATE_ON 0x01
#define BMBT_SCROLL_TEXT_SIZE 255
#define BMBT_SCROLL_TEXT_SPEED 750
#define BMBT_SCROLL_TEXT_TIMER 500
#define BMBT_TV_STATUS_OFF 0
#define BMBT_TV_STATUS_ON 1

#define BMBT_AUTOZOOM_TOLERANCE 4
#define BMBT_AUTOZOOM_DELAY 10000

typedef struct BMBTStatus_t {
    uint8_t playerMode: 1;
    uint8_t displayMode: 3;
    uint8_t navState: 1;
    uint8_t radType: 4;
    uint8_t tvStatus: 1;
    uint8_t menuBufferStatus: 1;
    uint8_t navIndexType;
} BMBTStatus_t;

typedef struct BMBTContext_t {
    BT_t *bt;
    IBus_t *ibus;
    uint8_t menu;
    BMBTStatus_t status;
    uint8_t timerHeaderIntervals;
    uint8_t timerMenuIntervals;
    uint8_t displayUpdateTaskId;
    uint8_t headerWriteTaskId;
    uint8_t menuWriteTaskId;
    uint8_t dspMode;
    UtilsAbstractDisplayValue_t mainDisplay;
    uint8_t navZoom: 4;
    uint32_t navZoomTime;
    uint8_t mapShown: 1;
    uint8_t naviSilenced: 1;
    uint8_t rangeNavi: 1;
} BMBTContext_t;

void BMBTInit(BT_t *, IBus_t *);
void BMBTDestroy();
void BMBTBTDeviceConnected(void *, uint8_t *);
void BMBTBTDeviceDisconnected(void *, uint8_t *);
void BMBTBTMetadata(void *, uint8_t *);
void BMBTBTPlaybackStatus(void *, uint8_t *);
void BMBTBTReady(void *, uint8_t *);
void BMBTIBusBMBTButtonPress(void *, uint8_t *);
void BMBTIBusCDChangerStatus(void *, uint8_t *);
void BMBTIBusGTChangeUIRequest(void *, uint8_t *);
void BMBTIKESpeedRPMUpdate(void *, uint8_t *);
void BMBTIBusMonitorStatus(void *, uint8_t *);
void BMBTRangeUpdate(void *, uint8_t *);
void BMBTIBusGTMenuBufferUpdate(void *, uint8_t *);
void BMBTIBusMenuSelect(void *, uint8_t *);
void BMBTIBusScreenBufferFlush(void *, uint8_t *);
void BMBTIBusSensorValueUpdate(void *, uint8_t *);
void BMBTRADDisplayMenu(void *, uint8_t *);
void BMBTRADUpdateMainArea(void *, uint8_t *);
void BMBTRADScreenModeRequest(void *, uint8_t *);
void BMBTGTScreenModeSet(void *, uint8_t *);
void BMBTTVStatusUpdate(void *, uint8_t *);
void BMBTMonitorControl(void *, uint8_t *);
void BMBTIBusVehicleConfig(void *, uint8_t *);
void BMBTTimerHeaderWrite(void *);
void BMBTTimerMenuWrite(void *);
void BMBTTimerScrollDisplay(void *);
void BMBTLogStatus(void *);
#endif /* BMBT_H */
