/*
 * File: cd53.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the CD53 UI Mode handler
 */
#ifndef CD53_H
#define CD53_H
#include <stdio.h>
#include "../lib/bt/bt_bc127.h"
#include "../lib/bt.h"
#include "../lib/log.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "menu/menu_singleline.h"

#define CD53_DISPLAY_METADATA_ON 1
#define CD53_DISPLAY_METADATA_OFF 0
#define CD53_DISPLAY_SCROLL_SPEED 750
#define CD53_DISPLAY_STATUS_OFF 0
#define CD53_DISPLAY_STATUS_ON 1
#define CD53_DISPLAY_STATUS_NEW 2
#define CD53_DISPLAY_TIMER_INT 500
#define CD53_DISPLAY_TEXT_LEN 11
#define CD53_MEDIA_STATE_OK 0
#define CD53_MEDIA_STATE_CHANGE 1
#define CD53_MEDIA_STATE_METADATA_OK 2
#define CD53_MODE_OFF 0
#define CD53_MODE_ACTIVE_DISPLAY_OFF 1
#define CD53_MODE_ACTIVE 2
#define CD53_MODE_CALL 3
#define CD53_MODE_DEVICE_SEL 4
#define CD53_MODE_SETTINGS 5
#define CD53_PAIRING_DEVICE_NONE -1
#define CD53_SEEK_MODE_NONE 0
#define CD53_SEEK_MODE_FWD 1
#define CD53_SEEK_MODE_REV 2
#define CD53_TIMEOUT_SCROLL_STOP -1
#define CD53_TIMEOUT_SCROLL_STOP_NEXT_ITR -2
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
    BT_t *bt;
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
    uint8_t mediaChangeState;
    uint32_t lastTelephoneButtonPress;
    UtilsAbstractDisplayValue_t mainDisplay;
    UtilsAbstractDisplayValue_t tempDisplay;
    MenuSingleLineContext_t menuContext;
} CD53Context_t;
void CD53Init(BT_t *, IBus_t *);
void CD53Destroy();
void CD53DisplayUpdateText(void *, char *, int8_t, uint8_t);
void CD53BTCallerID(void *, uint8_t *);
void CD53BTCallStatus(void *, uint8_t *);
void CD53BTDeviceDisconnected(void *, uint8_t *);
void CD53BTDeviceReady(void *, uint8_t *);
void CD53BTMetadata(CD53Context_t *, uint8_t *);
void CD53BTPlaybackStatus(void *, uint8_t *);
void CD53IBusBMBTButtonPress(void *, uint8_t *);
void CD53IBusCDChangerStatus(void *, uint8_t *);
void CD53IBusIgnitionStatus(void *, unsigned char *);
void CD53IBusMFLButton(void *, uint8_t *);
void CD53IBusRADWriteDisplay(void *, uint8_t *);
void CD53GTScreenModeSet(void *, uint8_t *);
void CD53TimerDisplay(void *);
#endif /* CD53_H */
