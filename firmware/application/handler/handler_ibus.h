/*
 * File: handler_ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic around I/K-Bus events
 */
#ifndef HANDLER_IBUS_H
#define HANDLER_IBUS_H
#include "handler_common.h"
#include "handler_common.h"
#include "../lib/bt.h"
#include "../lib/log.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/pcm51xx.h"
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "../ui/bmbt.h"
#include "../ui/cd53.h"
#include "../ui/mid.h"

void HandlerIBusInit(HandlerContext_t *);
void HandlerIBusCDCStatus(void *, unsigned char *);
void HandlerIBusDSPConfigSet(void *, unsigned char *);
void HandlerIBusFirstMessageReceived(void *, unsigned char *);
void HandlerIBusGMDoorsFlapsStatusResponse(void *, unsigned char *);
void HandlerIBusGTDIAIdentityResponse(void *, unsigned char *);
void HandlerIBusGTDIAOSIdentityResponse(void *, unsigned char *);
void HandlerIBusIKEIgnitionStatus(void *, unsigned char *);
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
void HandlerIBusSensorValueUpdate(void *, unsigned char *);
void HandlerIBusTELVolumeChange(void *, unsigned char *);
void HandlerTimerIBusCDCAnnounce(void *);
void HandlerTimerIBusCDCSendStatus(void *);
void HandlerTimerIBusLCMIOStatus(void *);
void HandlerTimerIBusLightingState(void *);
void HandlerTimerIBusPings(void *);
#endif /* HANDLER_IBUS_H */
