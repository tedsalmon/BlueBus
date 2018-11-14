/*
 * File: handler.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#include "handler.h"
static HandlerContext_t Context;
static char *PROFILES[4] = {
    "A2DP",
    "AVRCP",
    "",
    "HFP"
};

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
    Context.cdChangerLastStatus = TimerGetMillis();
    Context.btStartupIsRun = 0;
    Context.btConnectionStatus = HANDLER_BT_CONN_OFF;
    Context.uiMode = uiMode;
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
        BC127Event_DeviceFound,
        &HandlerBC127DeviceFound,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &HandlerBC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_Boot,
        &HandlerBC127Ready,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDKeepAlive,
        &HandlerIBusCDCKeepAlive,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDStatusRequest,
        &HandlerIBusCDCStatus,
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
    EventRegisterCallback(
        IBusEvent_RADDiagResponse,
        &HandlerIBusRADDiagnostics,
        &Context
    );
    TimerRegisterScheduledTask(
        &HandlerTimerCDCAnnounce,
        &Context,
        HANDLER_CDC_ANOUNCE_INT
    );
    TimerRegisterScheduledTask(
        &HandlerTimerCDCSendStatus,
        &Context,
        HANDLER_CDC_STATUS_INT
    );
    TimerRegisterScheduledTask(
        &HandlerTimerOpenProfileErrors,
        &Context,
        HANDLER_PROFILE_ERROR_INT
    );
    TimerRegisterScheduledTask(
        &HandlerTimerScanDevices,
        &Context,
        HANDLER_SCAN_INT
    );
    HandlerStartup(&Context);
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
 * HandlerStartup()
 *     Description:
 *         On application startup, announce that we are a CDC on the IBus. We
 *         also poll the GT, if it's on the network, for its diagnostics info
 *         and the IKE for the ignition status. We also setup the BC127's
 *         connectivity status.
 *     Params:
 *         HandlerContext_t *context - The context for the handler
 *     Returns:
 *         void
 */
void HandlerStartup(HandlerContext_t *context)
{
    IBusCommandRADGetDiagnostics(context->ibus);
    IBusCommandIKEGetIgnition(context->ibus);
    IBusCommandCDCAnnounce(context->ibus);
    BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
}

/**
 * HandlerBC127DeviceLinkConnected()
 *     Description:
 *         If a device link is opened, disable connectability once both profiles
 *         are opened. Otherwise if the iginition is off, disconnect all devices
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *data - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceLinkConnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibus->ignitionStatus == IBUS_IGNITION_ON) {
        // Once A2DP and AVRCP are connected, we can disable connectability
        if (context->bt->activeDevice.avrcpLinkId != 0 &&
            context->bt->activeDevice.a2dpLinkId != 0
        ) {
            BC127CommandBtState(context->bt, BC127_STATE_OFF, context->bt->discoverable);
        }
    } else {
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
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
    if (context->ibus->ignitionStatus == IBUS_IGNITION_ON) {
        BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
    } else {
        BC127CommandBtState(context->bt, BC127_STATE_OFF, context->bt->discoverable);
    }
    context->btConnectionStatus = HANDLER_BT_CONN_OFF;
    BC127ClearPairingErrors(context->bt);
}

/**
 * HandlerBC127DeviceFound()
 *     Description:
 *         If a device is found and we are not connected, connect to it
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceFound(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice.deviceId == 0 &&
        context->btConnectionStatus == HANDLER_BT_CONN_OFF &&
        context->ibus->ignitionStatus == IBUS_IGNITION_ON
    ) {
        char *macId = (char *) data;
        BC127CommandProfileOpen(context->bt, macId, "A2DP");
        BC127CommandProfileOpen(context->bt, macId, "AVRCP");
        context->btConnectionStatus = HANDLER_BT_CONN_ON;
    }
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
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            // Request Metadata
            BC127CommandGetMetadata(context->bt);
        }
        context->btStartupIsRun = 1;
    }
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerStatus <= IBUS_CDC_NOT_PLAYING
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
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        // Set the BT module not connectable or discoverable and disconnect all devices
        BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    } else {
        // Set the connectable and discoverable states to what they were
        BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
    }
    BC127CommandStatus(context->bt);
    BC127CommandList(context->bt);
}

/**
 * HandlerIBusCDCKeepAlive()
 *     Description:
 *         Respond to the Radio's "ping" with a "pong"
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusCDCKeepAlive(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandCDCKeepAlive(context->ibus);
    context->cdChangerLastKeepAlive = TimerGetMillis();
}

/**
 * HandlerIBusCDCStatus()
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
void HandlerIBusCDCStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char curAction = 0x00;
    unsigned char curStatus = 0x00;
    unsigned char requestedAction = pkt[4];
    if (requestedAction == IBUS_CDC_GET_STATUS) {
        curStatus = context->ibus->cdChangerStatus;
        if (curStatus == IBUS_CDC_PLAYING) {
            /*
             * The BM53 will pingback action 0x02 ("Start Playing") unless
             * we proactively report that as our "Playing" action.
             */
            curAction = IBUS_CDC_START_PLAYING;
        }
    } else if (requestedAction == IBUS_CDC_STOP_PLAYING) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
        curStatus = IBUS_CDC_NOT_PLAYING;
    } else if (requestedAction == IBUS_CDC_START_PLAYING_CD53 &&
               context->uiMode == HANDLER_UI_MODE_CD53
    ) {
        curAction = requestedAction;
        curStatus = IBUS_CDC_PLAYING;
    } else if (requestedAction == IBUS_CDC_START_PLAYING &&
               context->uiMode == HANDLER_UI_MODE_BMBT
    ) {
        curAction = IBUS_CDC_BM53_START_PLAYING;
        curStatus = IBUS_CDC_BM53_PLAYING;
    } else if (requestedAction == IBUS_CDC_CHANGE_TRACK) {
        curAction = IBUS_CDC_START_PLAYING;
    } else if (requestedAction == IBUS_CDC_SCAN_FORWARD &&
               context->uiMode == HANDLER_UI_MODE_BMBT
    ) {
        curAction = IBUS_CDC_START_PLAYING;
        curStatus = IBUS_CDC_PLAYING;
    } else {
        curAction = requestedAction;
    }
    IBusCommandCDCStatus(context->ibus, curAction, curStatus);
    context->cdChangerLastKeepAlive = TimerGetMillis();
    context->cdChangerLastStatus = TimerGetMillis();
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
 * HandlerIBusRADDiagnostics()
 *     Description:
 *         Grab the Radio diagnostics info. Here we can identifiy the vehicles
 *         radio type.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusRADDiagnostics(void *ctx, unsigned char *pkt)
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
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        // Set the BT module not connectable/discoverable. Disconnect all devices
        BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
        BC127ClearPairedDevices(context->bt);
    } else if (context->ibus->ignitionStatus == IBUS_IGNITION_ON) {
        IBusCommandCDCAnnounce(context->ibus);
        // Always assume that we're playing, that way the radio can tell us
        // if we shouldn't be, then we can deduce the system state
        context->ibus->cdChangerStatus = IBUS_CDC_PLAYING;
        // Reset the metadata so we don't display the wrong data
        BC127ClearMetadata(context->bt);
        // Set the BT module connectable
        BC127CommandBtState(context->bt, BC127_STATE_ON, BC127_STATE_OFF);
        // Request BC127 state
        BC127CommandStatus(context->bt);
        BC127CommandList(context->bt);
    }
}

/**
 * HandlerTimerCDCAnnounce()
 *     Description:
 *         This periodic task tracks how long it has been since the radio
 *         sent us (the CDC) a "ping". We should re-announce ourselves if that
 *         value reaches the timeout specified and the ignition is on.
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerCDCAnnounce(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastKeepAlive) >= HANDLER_CDC_ANOUNCE_TIMEOUT &&
        context->ibus->ignitionStatus == IBUS_IGNITION_ON
    ) {
        IBusCommandCDCAnnounce(context->ibus);
        context->cdChangerLastKeepAlive = now;
    }
}

/**
 * HandlerTimerCDCSendStatus()
 *     Description:
 *         This periodic task will proactively send the CDC status to the radio
 *         if we don't see a status poll within the last 20000 milliseconds.
 *         The CDC poll happens every 19945 milliseconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerCDCSendStatus(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastStatus) >= HANDLER_CDC_STATUS_TIMEOUT &&
        context->ibus->ignitionStatus == IBUS_IGNITION_ON
    ) {
        unsigned char curAction = 0x00;
        if (context->ibus->cdChangerStatus == IBUS_CDC_PLAYING) {
            curAction = 0x02;
        }
        IBusCommandCDCStatus(
            context->ibus,
            curAction,
            context->ibus->cdChangerStatus
        );
        context->cdChangerLastStatus = now;
        LogDebug("Handler: Send CDC status preemptively");
    }
}

/**
 * HandlerTimerMetadataRequest()
 *     Description:
 *         If there are any profile open errors, request the profile
 *         be opened again
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerOpenProfileErrors(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (strlen(context->bt->activeDevice.macId) > 0) {
        uint8_t idx;
        for (idx = 0; idx < BC127_PROFILE_COUNT; idx++) {
            if (context->bt->pairingErrors[idx] == 1) {
                BC127CommandProfileOpen(
                    context->bt,
                    context->bt->activeDevice.macId,
                    PROFILES[idx]
                );
                context->bt->pairingErrors[idx] = 0;
            }
        }
    }
}

/**
 * HandlerTimerScanDevices()
 *     Description:
 *         Rescan for devices on the PDL periodically
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127ClearInactivePairedDevices(context->bt);
    BC127CommandList(context->bt);
}
