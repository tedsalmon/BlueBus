/*
 * File: handler.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#include "handler.h"
static HandlerContext_t Context;
const static uint16_t HANDLER_CDC_ANOUNCE_INT = 30000;

/**
 * HandlerInit()
 *     Description:
 *         Initialize our context and register all event listeners and
 *         scheduled tasks.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerInit(BC127_t *bt, IBus_t *ibus, uint8_t uiMode)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.cdChangerLastKeepAlive = TimerGetMillis();
    Context.btStartupIsRun = 0;
    Context.ignitionStatus = 0;
    EventRegisterCallback(
        BC127Event_DeviceLinkConnected,
        &HandlerBC127DeviceLinkConnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceDisconnected,
        &HandlerBC127DeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &HandlerBC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceReady,
        &HandlerBC127Ready,
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
    EventRegisterCallback(
        IBusEvent_GTDiagResponse,
        &HandlerIBusGTDiagnostics,
        &Context
    );
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

/**
 * HandlerBC127DeviceLinkConnected()
 *     Description:
 *         If a device link is opened, make sure that the other links open too.
 *         Sometimes a device won't open all profiles. Once AVRCP and A2DP
 *         are open, make the BT device unconnectable
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceLinkConnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If the AVRCP link is being opened and the A2DP link closed, open it
    if (context->bt->activeDevice.avrcpLinkId != 0 &&
        context->bt->activeDevice.a2dpLinkId == 0
    ) {
        BC127CommandProfileOpen(
            context->bt,
            context->bt->activeDevice.macId,
            "A2DP"
        );
    }
    // If the HFP link is being opened and the A2DP link closed, open it
    if (context->bt->activeDevice.hfpLinkId != 0 &&
        context->bt->activeDevice.avrcpLinkId == 0
    ) {
        BC127CommandProfileOpen(
            context->bt,
            context->bt->activeDevice.macId,
            "AVRCP"
        );
    }
    // Once A2DP and AVRCP are connected, we can disable connectability
    if (context->bt->activeDevice.avrcpLinkId != 0 &&
        context->bt->activeDevice.a2dpLinkId != 0
    ) {
        BC127CommandConnectable(context->bt, BC127_STATE_OFF);
        context->bt->connectable = BC127_STATE_OFF;
    }
}

/**
 * HandlerBC127DeviceDisconnected()
 *     Description:
 *         If a device disconnects, make the module connectable again.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceDisconnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127CommandConnectable(context->bt, BC127_STATE_ON);
    context->bt->connectable = BC127_STATE_ON;
}

/**
 * HandlerBC127PlaybackStatus()
 *     Description:
 *         If the application is starting, request the BC127 AVRCP Metadata
 *         if it is playing. If the CD Change status is not set to "playing"
 *         then we pause playback.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127PlaybackStatus(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If this is the first Status update
    if (context->btStartupIsRun == 0) {
        if (context->bt->activeDevice.playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            // Request Metadata
            BC127CommandGetMetadata(context->bt);
        }
        context->btStartupIsRun = 1;
    }
    if (context->bt->activeDevice.playbackStatus == BC127_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerStatus <= 0x01
    ){
        // We're playing but not in Bluetooth mode - stop playback
        BC127CommandPause(context->bt);
    }
}

/**
 * HandlerBC127Ready()
 *     Description:
 *         If the BC127 restarts, make sure that it is in the right
 *         connectable and discoverable states
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127Ready(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ignitionStatus == 0) {
        // Set the BT module not connectable or discoverable and disconnect all devices
        BC127CommandDiscoverable(context->bt, BC127_STATE_OFF);
        BC127CommandConnectable(context->bt, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    } else {
        // Set the connectable and discoverable states to what they were
        BC127CommandDiscoverable(context->bt, context->bt->discoverable);
        BC127CommandConnectable(context->bt, BC127_STATE_ON);
    }
    BC127CommandStatus(context->bt);
    BC127CommandList(context->bt);
}

/**
 * HandlerBC127Startup()
 *     Description:
 *         Ask for the BC127 device on application startup
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127Startup(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Set it connectable until a connection is found
    BC127CommandConnectable(context->bt, BC127_STATE_ON);
    BC127CommandStatus(context->bt);
    BC127CommandList(context->bt);
}

/**
 * HandlerIBusStartup()
 *     Description:
 *         On application startup, announce that we are a CDC on the IBus. We
 *         also poll the GT, if it's on the network, for its diagnostics info
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusStartup(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandGTGetDiagnostics(context->ibus);
    IBusCommandSendCdChangerAnnounce(context->ibus);
    // Always assume that we're playing, that way the radio can tell us
    // if we shouldn't be, then we can deduce the system state
    context->ibus->cdChangerStatus = 0x09;
}

/**
 * HandlerIBusCDChangerKeepAlive()
 *     Description:
 *         Respond to the Radio's "ping" with a "pong"
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusCDChangerKeepAlive(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandSendCdChangerKeepAlive(context->ibus);
    context->cdChangerLastKeepAlive = TimerGetMillis();
}

/**
 * HandlerIBusCDChangerStatus()
 *     Description:
 *         Track the current CD Changer status based on what the radio
 *         instructs us to do. We respond with exactly what the radio instructs
 *         even if we haven't done it yet.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char curAction = 0x00;
    unsigned char curStatus = 0x02;
    unsigned char changerStatus = pkt[4];
    if (changerStatus == 0x00) {
        // Asking for status
        curStatus = context->ibus->cdChangerStatus;
    } else if (changerStatus == 0x01) {
        // Stop Playing
        if (context->bt->activeDevice.playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
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

/**
 * HandlerIBusGTDiagnostics()
 *     Description:
 *         Track the GT diagnostics info. Here we can define what device versions
 *         are installed in the vehicle.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusGTDiagnostics(void *ctx, unsigned char *pkt)
{

}

/**
 * HandlerIBusIgnitionStatus()
 *     Description:
 *         Track the Ignition state and update the BC127 accordingly. We set
 *         the BT device "off" when the key is set to position 0 and on
 *         as soon as it goes to a position >= 1
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusIgnitionStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If the first bit is set, the key is in position 1 at least, otherwise
    // the ignition is off
    if (pkt[4] == 0x00){
        context->ignitionStatus = 0;
        // Set the BT module not connectable or discoverable and disconnect all devices
        BC127CommandDiscoverable(context->bt, BC127_STATE_OFF);
        BC127CommandConnectable(context->bt, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    } else {
        if (context->ignitionStatus == 0) {
            // Ignore the ignition states while developing
            // HandlerIBusStartup(ctx, 0);
            // Set the BT module connectable and discoverable
            // BC127CommandDiscoverable(context->bt, BC127_STATE_ON);
            // BC127CommandConnectable(context->bt, BC127_STATE_ON);
        }
        context->ignitionStatus = 1;
    }
}

/**
 * HandlerTimerCDChangerAnnounce()
 *     Description:
 *         This periodic task tracks how long it has been since the radio
 *         sent us (the CDC) a "ping". We should re-announce ourselves if that
 *         value reaches the timeout specified and the ignition is on.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerTimerCDChangerAnnounce(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastKeepAlive) >= HANDLER_CDC_ANOUNCE_INT &&
        context->ignitionStatus == 1
    ) {
        IBusCommandSendCdChangerAnnounce(context->ibus);
        context->cdChangerLastKeepAlive = now;
    }

}
