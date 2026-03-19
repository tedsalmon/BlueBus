/*
 * File: handler_bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and I/K-Bus communicate
 */
#ifndef HANDLER_BT_H
#define HANDLER_BT_H
#include "handler_common.h"

void HandlerBTInit(HandlerContext_t *);
void HandlerBTCallStatus(void *, uint8_t *);
void HandlerBTCallerID(void *, uint8_t *);
void HandlerBTDeviceDisconnected(void *, uint8_t *);
void HandlerBTDeviceLinkConnected(void *, uint8_t *);
void HandlerBTDeviceLinkDisconnected(void *, uint8_t *);
void HandlerBTPairingStatus(void *, uint8_t *);
void HandlerBTPairingsLoaded(void *, uint8_t *);
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

void HandlerTimerBTBC127AVRCPPoll(void *);
void HandlerTimerBTBC127RequestDateTime(void *);
void HandlerTimerBTBC127OpenProfileErrors(void *);
void HandlerTimerBTBC127State(void *);
void HandlerTimerBC127VolumeManagement(void *);

void HandlerTimerBTBM83AVRCPManager(void *);
void HandlerTimerBTBM83ManagePowerState(void *);
#endif /* HANDLER_BT_H */
