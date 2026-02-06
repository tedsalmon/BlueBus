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
#include "../lib/timer.h"
#include "../lib/utils.h"
#include "../ui/bmbt.h"
#include "../ui/cd53.h"
#include "../ui/mid.h"
#include <stdint.h>

void HandlerIBusInit(HandlerContext_t *);
void HandlerIBusBlueBusTELStatusUpdate(void *, uint8_t *);
void HandlerIBusBMBTButtonPress(void *, uint8_t *);
void HandlerIBusCDCStatus(void *, uint8_t *);
void HandlerIBusDSPConfigSet(void *, uint8_t *);
void HandlerIBusFirstMessageReceived(void *, uint8_t *);
void HandlerIBusGMDoorsFlapsStatusResponse(void *, uint8_t *);
void HandlerIBusGMIdentResponse(void *, uint8_t *);
void HandlerIBusGTDIAIdentityResponse(void *, uint8_t *);
void HandlerIBusGTDIAOSIdentityResponse(void *, uint8_t *);
void HandlerIBusIKEIgnitionStatus(void *, uint8_t *);
void HandlerIBusIKESpeedRPMUpdate(void *, uint8_t *);
void HandlerIBusIKEVehicleConfig(void *, uint8_t *);
void HandlerIBusLMLightStatus(void *, uint8_t *);
void HandlerIBusLMDimmerStatus(void *, uint8_t *);
void HandlerIBusLMIdentResponse(void *, uint8_t *);
void HandlerIBusLMRedundantData(void *, uint8_t *);
void HandlerIBusMFLButton(void *, uint8_t *);
void HandlerIBusModuleStatusResponse(void *, uint8_t *);
void HandlerIBusModuleStatusRequest(void *, uint8_t *);
void HandlerIBusNavGPSDateTimeUpdate(void *, uint8_t *);
void HandlerIBusPDCSensorUpdate(void *, uint8_t *);
void HandlerIBusPDCStatus(void *, uint8_t *);
void HandlerIBusRADMessageReceived(void *, uint8_t *);
void HandlerIBusVMDIAIdentityResponse(void *, uint8_t *);
void HandlerIBusVolumeChange(void *, uint8_t *);
void HandlerIBusSensorValueUpdate(void *, uint8_t *);
void HandlerIBusTELVolumeChange(void *, uint8_t *);
void HandlerTimerIBusCDCAnnounce(void *);
void HandlerTimerIBusCDCSendStatus(void *);
void HandlerTimerIBusLCMIOStatus(void *);
void HandlerTimerIBusIdent(void *);
void HandlerTimerIBusLightingState(void *);
void HandlerTimerIBusPDCDistance(void *);
void HandlerTimerIBusPings(void *);
#endif /* HANDLER_IBUS_H */
