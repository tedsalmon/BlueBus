/*
 * File: mid.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#ifndef MID_H
#define MID_H
#include "../lib/bc127.h"
#include "../lib/config.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/log.h"
#include "../lib/timer.h"
#include "../lib/utils.h"

#define MID_BUTTON_PLAYBACK 0
#define MID_BUTTON_BACK 0
#define MID_BUTTON_META 1
#define MID_BUTTON_EDIT_SAVE 1
#define MID_BUTTON_PREV_VAL 2
#define MID_BUTTON_SETTINGS_L 2
#define MID_BUTTON_NEXT_VAL 3
#define MID_BUTTON_SETTINGS_R 3
#define MID_BUTTON_DEVICES_L 4
#define MID_BUTTON_DEVICES_R 5
#define MID_BUTTON_PAIR 8

#define MID_DISPLAY_NONE 0
#define MID_DISPLAY_REWRITE 1
#define MID_DISPLAY_REWRITE_NEXT 2
#define MID_DISPLAY_SCROLL_SPEED 750
#define MID_DISPLAY_STATUS_OFF 0
#define MID_DISPLAY_STATUS_ON 1
#define MID_DISPLAY_STATUS_NEW 2
#define MID_DISPLAY_TEXT_SIZE 20
#define MID_DISPLAY_TIMER_INT 500

#define MID_METADATA_MODE_PARTY 0x01
#define MID_METADATA_MODE_CHUNK 0x02

#define MID_MODE_OFF 0
#define MID_MODE_ACTIVE 1
#define MID_MODE_SETTINGS 2
#define MID_MODE_DEVICES 3


/*
 * MIDContext_t
 *  This is a struct to hold the context of the MID UI implementation
 *  BC127_t *bt: A pointer to the Bluetooth struct
 *  IBus_t *ibus: A pointer to the IBus struct
 *  mode: Track the state of the radio to see what we should display to the user.
 *  screenUpdated: The screen has been updated by the radio
 */
typedef struct MIDContext_t {
    IBus_t *ibus;
    BC127_t *bt;
    int8_t btDeviceIndex;
    uint8_t mode;
    uint8_t displayUpdate;
    UtilsAbstractDisplayValue_t mainDisplay;
    UtilsAbstractDisplayValue_t tempDisplay;
    uint8_t displayUpdateTaskId;
} MIDContext_t;
void MIDBC127MetadataUpdate(void *, unsigned char *);
void MIDBC127PlaybackStatus(void *, unsigned char *);
void MIDInit(BC127_t *, IBus_t *);
void MIDIBusCDChangerStatus(void *, unsigned char *);
void MIDIBusMIDButtonPress(void *, unsigned char *);
void MIDIIBusRADMIDDisplayUpdate(void *, unsigned char *);
void MIDIIBusRADMIDMenuUpdate(void *, unsigned char *);
void MIDTimerDisplay(void *);
#endif /* MID_H */
