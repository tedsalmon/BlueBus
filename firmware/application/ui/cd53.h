/*
 * File: cd53.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the CD53 UI Mode handler
 */
#ifndef CD53_H
#define CD53_H
#define _ADDED_C_LIB 1
#include <stdio.h>
#include "../lib/bc127.h"
#include "../lib/log.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#define CD53_DISPLAY_METADATA_ON 1
#define CD53_DISPLAY_METADATA_OFF 0
#define CD53_DISPLAY_SCROLL_SPEED 750
#define CD53_DISPLAY_STATUS_OFF 0
#define CD53_DISPLAY_STATUS_ON 1
#define CD53_DISPLAY_STATUS_NEW 2
#define CD53_DISPLAY_TIMER_INT 500
#define CD53_DISPLAY_TEMP_TEXT_SIZE 11
#define CD53_MODE_OFF 0
#define CD53_MODE_ACTIVE 1
#define CD53_MODE_DEVICE_SEL 2
#define CD53_MODE_SETTINGS 3
#define CD53_PAIRING_DEVICE_NONE -1
#define CD53_SEEK_MODE_NONE 0
#define CD53_SEEK_MODE_FWD 1
#define CD53_SEEK_MODE_REV 2
#define CD53_SETTING_IDX_HFP 0
#define CD53_SETTING_IDX_METADATA_MODE 1
#define CD53_SETTING_IDX_AUTOPLAY 2
#define CD53_SETTING_IDX_VEH_TYPE 3
#define CD53_SETTING_IDX_BLINKERS 4
#define CD53_SETTING_IDX_TCU_MODE 5
#define CD53_SETTING_IDX_PAIRINGS 6
#define CD53_SETTING_MODE_SCROLL_SETTINGS 1
#define CD53_SETTING_MODE_SCROLL_VALUES 2
#define CD53_METADATA_MODE_PARTY 0x01
#define CD53_METADATA_MODE_CHUNK 0x02
#define CD53_VR_TOGGLE_TIME 500

/*
 * CD53Context_t
 *  This is a struct to hold the context of the CD53 UI implementation
 *  bt: A pointer to the Bluetooth struct
 *  ibus: A pointer to the IBus struct
 *  mode: Track the state of the radio to see what we should display to the user.
 *  displayUpdateTaskId: The ID of the display update task. We use this to call
 *      the display update task immediately.
 *  btDeviceIndex: The selected Bluetooth device -- Used to change selected devices
 *  mainDisplay: The main text that should be displayed
 *  tempDisplay: The value to temporarily display on the screen. The max text
 *      length is 11 characters.
 */
typedef struct CD53Context_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t mode;
    uint8_t displayUpdateTaskId;
    int8_t btDeviceIndex;
    uint8_t seekMode;
    uint8_t displayMetadata;
    uint8_t settingIdx;
    uint8_t settingValue;
    uint8_t settingMode;
    uint8_t radioType;
    uint32_t lastTelephoneButtonPress;
    UtilsAbstractDisplayValue_t mainDisplay;
    UtilsAbstractDisplayValue_t tempDisplay;
} CD53Context_t;
void CD53Init(BC127_t *, IBus_t *);
void CD53BC127DeviceDisconnected(void *, unsigned char *);
void CD53BC127DeviceReady(void *, unsigned char *);
void CD53BC127Metadata(CD53Context_t *, unsigned char *);
void CD53BC127PlaybackStatus(void *, unsigned char *);
void CD53IBusClearScreen(void *, unsigned char *);
void CD53IBusCDChangerStatus(void *, unsigned char *);
void CD53TimerDisplay(void *);
#endif /* CD53_H */
