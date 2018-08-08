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
#include "lib/debug.h"
#include "lib/event.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"
#include "ui/cd53.h"
#include "ui/bmbt.h"
#define HANDLER_UI_MODE_CD53 0
#define HANDLER_UI_MODE_BMBT 1
typedef struct HandlerContext_t {
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t btStartupIsRun;
    uint8_t ignitionStatus;
    uint32_t cdChangerLastKeepAlive;
} HandlerContext_t;
void HandlerInit(BC127_t *, IBus_t *, uint8_t);
void HandlerBC127DeviceConnected(void *, unsigned char *);
void HandlerBC127DeviceLinkConnected(void *, unsigned char *);
void HandlerBC127PlaybackStatus(void *, unsigned char *);
void HandlerBC127Ready(void *, unsigned char *);
void HandlerBC127Startup(void *, unsigned char *);
void HandlerIBusStartup(void *, unsigned char *);
void HandlerIBusCDChangerKeepAlive(void *, unsigned char *);
void HandlerIBusCDChangerStatus(void *, unsigned char *);
void HandlerIBusGTDiagnostics(void *, unsigned char *);
void HandlerIBusIgnitionStatus(void *, unsigned char *);
void HandlerTimerCDChangerAnnounce(void *);
#endif /* HANDLER_H */
