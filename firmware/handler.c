/*
 * File: handler.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#include "handler.h"
static HandlerContext_t Context;
const static uint16_t HANDLER_CDC_ANOUNCE_INT = 30000;

void HandlerInit(BC127_t *bt, IBus_t *ibus, uint8_t uiMode)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.cdChangerLastKeepAlive = TimerGetMillis();
    Context.btStartupIsRun = 0;
    Context.ignitionStatus = 0;
    EventRegisterCallback(
        BC127Event_DeviceConnected,
        &HandlerBC127DeviceConnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceLinkConnected,
        &HandlerBC127DeviceLinkConnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &HandlerBC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_Startup,
        &HandlerBC127Startup,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_Startup,
        &HandlerIBusStartup,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDKeepAlive,
        &HandlerIBusCDChangerKeepAlive,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDStatusRequest,
        &HandlerIBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_IgnitionStatus,
        &HandlerIBusIgnitionStatus,
        &Context
    );
    //EventRegisterCallback(
    //    IBusEvent_NavDiagResponse,
    //    0, // Implement
    //    &Context
    //);
    TimerRegisterScheduledTask(
        &HandlerTimerCDChangerAnnounce,
        &Context,
        HANDLER_CDC_ANOUNCE_INT
    );
    switch (uiMode) {
        case HANDLER_UI_MODE_CD53:
            CD53Init(bt, ibus);
            break;
        case HANDLER_UI_MODE_BMBT:
            BMBTInit(bt, ibus);
            break;
    }
}

void HandlerBC127DeviceConnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t deviceId = (uint8_t) *data;
    if (context->bt->activeDevice != 0) {
        if (deviceId != context->bt->activeDevice->deviceId) {
            BC127CommandClose(context->bt, deviceId);
        }
    }
}

void HandlerBC127DeviceLinkConnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice != 0) {
        uint8_t linkId = strToInt((char *) data);
        uint8_t deviceId = BC127GetDeviceId((char *) data);
        if (deviceId != context->bt->activeDevice->deviceId) {
            BC127CommandClose(context->bt, linkId);
        }
    }
}

void HandlerBC127PlaybackStatus(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If this is the first Status update
    if (context->btStartupIsRun == 0) {
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
            // Request Metadata
            BC127CommandGetMetadata(context->bt);
        }
        context->btStartupIsRun = 1;
    }
    if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerStatus <= 0x01
    ){
        // We're playing but not in Bluetooth mode - stop playback
        BC127CommandPause(context->bt);
    }
}

void HandlerBC127Startup(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127CommandStatus(context->bt);
}

void HandlerIBusStartup(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandSendCdChangerAnnounce(context->ibus);
    // Always assume that we're playing, that way the radio can tell us
    // if we shouldn't be, then we can deduce the system state
    context->ibus->cdChangerStatus = 0x09;
}

void HandlerIBusCDChangerKeepAlive(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandSendCdChangerKeepAlive(context->ibus);
    context->cdChangerLastKeepAlive = TimerGetMillis();
}

void HandlerIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char curAction = 0x00;
    unsigned char curStatus = 0x02;
    unsigned char changerStatus = pkt[4];
    if (changerStatus == 0x00) {
        // Asking for status
        curStatus = context->ibus->cdChangerStatus;
    } else if (changerStatus == 0x03) {
        // Start Playing
        curAction = 0x02;
        curStatus = 0x09;
    } else if (changerStatus == 0x07 || changerStatus == 0x08) {
        curStatus = changerStatus;
    }
    IBusCommandSendCdChangerStatus(context->ibus, &curStatus, &curAction);
    context->cdChangerLastKeepAlive = TimerGetMillis();
}

void HandlerIBusIgnitionStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If the first bit is set, the key is in position 1 at least, otherwise
    // the ignition is off
    if (pkt[4] == 0x00){
        context->ignitionStatus = 0;
        // Set the BT module connectable and discoverable
        BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    } else {
        if (context->ignitionStatus == 0) {
            HandlerIBusStartup(ctx, 0);
            // Set the BT module connectable but not discoverable
            BC127CommandBtState(context->bt, BC127_STATE_ON, BC127_STATE_OFF);
        }
        context->ignitionStatus = 1;
    }
}

void HandlerTimerCDChangerAnnounce(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastKeepAlive) >= HANDLER_CDC_ANOUNCE_INT) {
        IBusCommandSendCdChangerAnnounce(context->ibus);
        context->cdChangerLastKeepAlive = now;
    }

}
