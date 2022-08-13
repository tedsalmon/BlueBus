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
#include "handler/handler_bt.h"
#include "handler/handler_common.h"
#include "lib/bt/bt_bc127.h"
#include "lib/bt/bt_bm83.h"
#include "lib/bt.h"
#include "lib/log.h"
#include "lib/event.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"
#include "ui/bmbt.h"
#include "ui/cd53.h"
#include "ui/mid.h"

void HandlerInit(BT_t *, IBus_t *);
void HandlerUICloseConnection(void *, unsigned char *);
void HandlerUIInitiateConnection(void *, unsigned char *);
void HandlerIBusCDCStatus(void *, unsigned char *);
void HandlerIBusDSPConfigSet(void *, unsigned char *);
void HandlerIBusFirstMessageReceived(void *, unsigned char *);
void HandlerIBusGMDoorsFlapsStatusResponse(void *, unsigned char *);
void HandlerIBusGTDIAIdentityResponse(void *, unsigned char *);
void HandlerIBusGTDIAOSIdentityResponse(void *, unsigned char *);
void HandlerIBusIKEIgnitionStatus(void *, unsigned char *);
void HandlerIBusSensorValueUpdate(void *, unsigned char *);
void HandlerIBusIKESpeedRPMUpdate(void *, unsigned char *);
void HandlerIBusIKEVehicleConfig(void *, unsigned char *);
void HandlerIBusLMLightStatus(void *, unsigned char *);
void HandlerIBusLMDimmerStatus(void *, unsigned char *);
void HandlerIBusLMIdentResponse(void *, unsigned char *);
void HandlerIBusLMRedundantData(void *, unsigned char *);
void HandlerIBusMFLButton(void *, unsigned char *);
void HandlerIBusModuleStatusResponse(void *, unsigned char *);
void HandlerIBusModuleStatusRequest(void *, unsigned char *);
void HandlerIBusPDCStatus(void *, unsigned char *);
void HandlerIBusRADVolumeChange(void *, unsigned char *);
void HandlerIBusTELVolumeChange(void *, unsigned char *);
void HandlerIBusBroadcastCDCStatus(HandlerContext_t *);
void HandlerTimerBC127State(void *);
void HandlerTimerCDCAnnounce(void *);
void HandlerTimerCDCSendStatus(void *);
void HandlerTimerIBusPings(void *);
void HandlerTimerLCMIOStatus(void *);
void HandlerTimerLightingState(void *);
void HandlerTimerPoweroff(void *);
void HandlerLMActivateBulbs(HandlerContext_t *, unsigned char);
#endif /* HANDLER_H */
