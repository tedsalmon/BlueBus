/*
 * File: mid.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#ifndef MID_H
#define MID_H
#include "../lib/bt/bt_bc127.h"
#include "../lib/bt.h"
#include "../lib/config.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/log.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "menu/menu_singleline.h"

#define MID_BUTTON_PLAYBACK 0x40
#define MID_BUTTON_BACK 0x40
#define MID_BUTTON_META 0x41
#define MID_BUTTON_EDIT_SAVE 0x41
#define MID_BUTTON_PREV_VAL 0x42
#define MID_BUTTON_SETTINGS_L 0x42
#define MID_BUTTON_NEXT_VAL 0x43
#define MID_BUTTON_SETTINGS_R 0x43
#define MID_BUTTON_DEVICES_L 0x45
#define MID_BUTTON_DEVICES_R 0x46
#define MID_BUTTON_PAIR 0x48
#define MID_BUTTON_BT 0x4A
#define MID_BUTTON_MODE 0x4B

#define MID_BUTTON_BT_PRESS 0x0A
#define MID_BUTTON_BT_RELEASE 0x4B

#define MID_BUTTON_MODE_PRESS 0x0B
#define MID_BUTTON_MODE_RELEASE 0x4B

#define MID_DISPLAY_NONE 0
#define MID_DISPLAY_REWRITE 1
#define MID_DISPLAY_REWRITE_NEXT 2
#define MID_DISPLAY_SCROLL_SPEED 750
#define MID_DISPLAY_STATUS_OFF 0
#define MID_DISPLAY_STATUS_ON 1
#define MID_DISPLAY_STATUS_NEW 2
#define MID_DISPLAY_TEXT_SIZE 24
#define MID_TIMER_DISPLAY_INT 500
#define MID_TIMER_MENU_WRITE_INT 250

#define MID_MODE_OFF 0
#define MID_MODE_DISPLAY_OFF 1
#define MID_MODE_ACTIVE 2
#define MID_MODE_SETTINGS 3
#define MID_MODE_DEVICES 4
#define MID_MODE_ACTIVE_NEW 5
#define MID_MODE_SETTINGS_NEW 6
#define MID_MODE_DEVICES_NEW 7

#define MID_MODE_CHANGE_OFF 0
#define MID_MODE_CHANGE_PRESS 1
#define MID_MODE_CHANGE_RELEASE 2

#define MID_PAIRING_DEVICE_NONE -1

#define MID_SETTING_IDX_METADATA_MODE 0
#define MID_SETTING_IDX_AUTOPLAY 1
#define MID_SETTING_IDX_AUDIO_DSP 2
#define MID_SETTING_IDX_LOWER_VOL_REV 3
#define MID_SETTING_IDX_AUDIO_DAC_GAIN 4
#define MID_SETTING_IDX_TEL_HFP 5
#define MID_SETTING_IDX_TEL_MIC_GAIN 6
#define MID_SETTING_IDX_TEL_VOL_OFFSET 7
#define MID_SETTING_IDX_TEL_TCU_MODE 8
#define MID_SETTING_IDX_BLINKERS 9
#define MID_SETTING_IDX_PARK_LIGHTS 10
#define MID_SETTING_IDX_COMFORT_LOCKS 11
#define MID_SETTING_IDX_COMFORT_UNLOCK 12
#define MID_SETTING_IDX_ABOUT 13
#define MID_SETTING_IDX_PAIRINGS 14

#define MID_SETTING_MODE_SCROLL_SETTINGS 1
#define MID_SETTING_MODE_SCROLL_VALUES 2
#define MID_SETTING_METADATA_MODE_OFF 0x00
#define MID_SETTING_METADATA_MODE_PARTY 0x01
#define MID_SETTING_METADATA_MODE_CHUNK 0x02


/*
 * MIDContext_t
 *  This is a struct to hold the context of the MID UI implementation
 *  BC127_t *bt: A pointer to the Bluetooth struct
 *  IBus_t *ibus: A pointer to the IBus struct
 *  mode: Track the state of the radio to see what we should display to the user.
 *  screenUpdated: The screen has been updated by the radio
 */
typedef struct MIDContext_t {
    BT_t *bt;
    IBus_t *ibus;
    int8_t btDeviceIndex;
    uint8_t mode;
    uint8_t displayUpdate;
    uint8_t modeChangeStatus;
    char mainText[16];
    MenuSingleLineContext_t menuContext;
    UtilsAbstractDisplayValue_t mainDisplay;
    UtilsAbstractDisplayValue_t tempDisplay;
    uint8_t displayUpdateTaskId;
} MIDContext_t;
void MIDInit(BT_t *, IBus_t *);
void MIDDestroy();
void MIDDisplayUpdateText(void *, char *, int8_t, uint8_t);
void MIDBTDeviceDisconnected(void *, uint8_t *);
void MIDBTMetadataUpdate(void *, uint8_t *);
void MIDBTPlaybackStatus(void *, uint8_t *);
void MIDIBusCDChangerStatus(void *, uint8_t *);
void MIDIBusIgnitionStatus(void *, uint8_t *);
void MIDIBusMIDButtonPress(void *, uint8_t *);
void MIDIIBusRADMIDDisplayUpdate(void *, uint8_t *);
void MIDIIBusRADMIDMenuUpdate(void *, uint8_t *);
void MIDIBusMIDModeChange(void *, uint8_t *);
void MIDTimerMenuWrite(void *);
void MIDTimerDisplay(void *);
#endif /* MID_H */
