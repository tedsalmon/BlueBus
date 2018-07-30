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
#define CD53_DISPLAY_TEXT_SIZE 256
typedef struct CD53Context_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t ibusAnounce;
    char displayText[CD53_DISPLAY_TEXT_SIZE];
    uint8_t displayTextIdx;
    uint8_t displayTextTicks;
    uint8_t displayTextStatus;
    uint8_t displayUpdateTaskId;
} CD53Context_t;
void CD53Init(BC127_t *, IBus_t *);
void CD53BC127DeviceReady(void *, unsigned char *);
void CD53BC127Metadata(void *, unsigned char *);
void CD53BC127PlaybackStatus(void *, unsigned char *);
void CD53IBusClearScreen(void *, unsigned char *);
void CD53IBusCDChangerStatus(void *, unsigned char *);
void CD53TimerDisplay(void *);
#endif /* CD53_H */
