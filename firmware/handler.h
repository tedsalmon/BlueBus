/*
 * File: handler.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#ifndef HANDLER_H
#define HANDLER_H
#include "lib/bc127.h"
#include "lib/ibus.h"
#define HANDLER_DISPLAY_TEXT_SIZE 256
#define HANDLER_DISPLAY_CHANGE_INT 1500
#define HANDLER_CDC_ANOUNCE_INT 15000
typedef struct ApplicationContext_t {
    struct BC127_t *bt;
    struct IBus_t *ibus;
    uint8_t announceSelf;
    char displayText[HANDLER_DISPLAY_TEXT_SIZE];
    uint8_t displayTextIdx;
    uint32_t displayTextLastStamp;
    uint32_t cdChangeLastKeepAlive;
} ApplicationContext_t;
void HandlerRegister(struct BC127_t *, struct IBus_t *);
void HandlerBC127Startup(void *, unsigned char *);
void HandlerBC127Metadata(void *, unsigned char *);
void HandlerBC127PlaybackStatus(void *, unsigned char *);
void HandlerIBusStartup(void *, unsigned char *);
void HandlerIBusCDChangerKeepAlive(void *, unsigned char *);
void HandlerIBusCDChangerStatus(void *, unsigned char *);
void HandlerTimerCD53StatusScroll(void *);
void HandlerTimerCDChangerAnnounce(void *);
#endif /* HANDLER_H */
