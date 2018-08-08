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
#include "../lib/debug.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
#define CD53_DISPLAY_SCROLL_SPEED 500
#define CD53_DISPLAY_TEXT_SIZE 255
#define CD53_DISPLAY_TEMP_TEXT_SIZE 11
#define CD53_MODE_OFF 0
#define CD53_MODE_ACTIVE 1
#define CD53_MODE_DEVICE_SEL 2
#define CD53_PAIRING_DEVICE_NONE -1
/*
 * CD53DisplayValue_t
 *  This is a struct to hold text values to be displayed
 *  text: The text to display
 *  index: A variable to track what the last displayed index of text was
 *  length: The length of the text
 *  status: 0 for inactive and 1 for active
 *  timeout: The amount of iterations to display the text for. -1 is indefinite
 */
typedef struct CD53DisplayValue_t {
    char text[CD53_DISPLAY_TEXT_SIZE];
    uint8_t index;
    uint8_t length;
    uint8_t status;
    int8_t timeout;
} CD53DisplayValue_t;
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
    CD53DisplayValue_t mainDisplay;
    CD53DisplayValue_t tempDisplay;
} CD53Context_t;
void CD53Init(BC127_t *, IBus_t *);
CD53DisplayValue_t CD53DisplayValueInit(char *);
void CD53BC127DeviceDisconnected(void *, unsigned char *);
void CD53BC127DeviceReady(void *, unsigned char *);
void CD53BC127Metadata(void *, unsigned char *);
void CD53BC127PlaybackStatus(void *, unsigned char *);
void CD53IBusClearScreen(void *, unsigned char *);
void CD53IBusCDChangerStatus(void *, unsigned char *);
void CD53TimerDisplay(void *);
#endif /* CD53_H */
