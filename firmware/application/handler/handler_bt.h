/*
 * File: handler_bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and I/K-Bus communicate
 */
#ifndef HANDLER_BT_H
#define HANDLER_BT_H
#include <stdio.h>
#include "../lib/bt/bt_bc127.h"
#include "../lib/bt/bt_bm83.h"
#include "../lib/bt/bt_common.h"
#include "../lib/log.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "../ui/bmbt.h"
#include "../ui/cd53.h"
#include "../ui/mid.h"
#include "handler_common.h"

void HandlerBTInit(HandlerContext_t *);
void HandlerBTCallStatus(void *, uint8_t *);
void HandlerBTCallerID(void *, uint8_t *);
void HandlerBTDeviceDisconnected(void *, uint8_t *);
void HandlerBTDeviceLinkConnected(void *, uint8_t *);
void HandlerBTPlaybackStatus(void *, uint8_t *);
void HandlerBTTimeUpdate(void *, uint8_t *);
void HandlerUICloseConnection(void *, uint8_t *);
void HandlerUIInitiateConnection(void *, uint8_t *);

void HandlerBTBC127Boot(void *, uint8_t *);
void HandlerBTBC127BootStatus(void *, uint8_t *);

void HandlerBTBM83AVRCPUpdates(void *, uint8_t *);
void HandlerBTBM83Boot(void *, uint8_t *);
void HandlerBTBM83BootStatus(void *, uint8_t *);
void HandlerBTBM83DSPStatus(void *, uint8_t *);

void HandlerTimerBTScanDevices(void *);
void HandlerTimerBTTCUStateChange(void *);
void HandlerTimerBTVolumeManagement(void *);

void HandlerTimerBTBC127AVRCPPoll(void *);
void HandlerTimerBTBC127OpenProfileErrors(void *);
void HandlerTimerBTBC127RequestDateTime(void *);
void HandlerTimerBTBC127State(void *);

void HandlerTimerBTBM83AVRCPManager(void *);
void HandlerTimerBTBM83ManagePowerState(void *);
#endif /* HANDLER_BT_H */
