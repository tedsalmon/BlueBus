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
 *         BC127_t *bt - The BC127 Object
 *         IBus_t *ibus - The IBus Object
 *     Returns:
 *         void
 */
void HandlerInit(BC127_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.cdChangerLastPoll = TimerGetMillis();
    Context.cdChangerLastStatus = TimerGetMillis();
    Context.btDeviceConnRetries = 0;
    Context.btStartupIsRun = 0;
    Context.btConnectionStatus = HANDLER_BT_CONN_OFF;
    Context.btSelectedDevice = HANDLER_BT_SELECTED_DEVICE_NONE;
    Context.uiMode = ConfigGetUIMode();
    Context.seekMode = HANDLER_CDC_SEEK_MODE_NONE;
    Context.blinkerCount = 0;
    Context.blinkerStatus = HANDLER_BLINKER_OFF;
    Context.powerStatus = HANDLER_POWER_ON;
    Context.scanIntervals = 0;
    EventRegisterCallback(
        BC127Event_Boot,
        &HandlerBC127Boot,
        &Context
    );
    EventRegisterCallback(
        BC127Event_BootStatus,
        &HandlerBC127BootStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_CallStatus,
        &HandlerBC127CallStatus,
        &Context
    );
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
        UIEvent_CloseConnection,
        &HandlerUICloseConnection,
        &Context
    );
    EventRegisterCallback(
        UIEvent_InitiateConnection,
        &HandlerUIInitiateConnection,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDPoll,
        &HandlerIBusCDCPoll,
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
        IBusEvent_LCMLightStatus,
        &HandlerIBusLCMLightStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_LCMDimmerStatus,
        &HandlerIBusLCMDimmerStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_MFLButton,
        &HandlerIBusMFLButton,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_MFLVolume,
        &HandlerIBusMFLVolume,
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
        &HandlerTimerDeviceConnection,
        &Context,
        HANDLER_INT_DEVICE_CONN
    );
    TimerRegisterScheduledTask(
        &HandlerTimerPoweroff,
        &Context,
        1000
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
    // Poll the vehicle for the ignition status so we can configure ourselves
    IBusCommandIKEGetIgnition(Context.ibus);
    BC127CommandStatus(Context.bt);
    if (Context.uiMode == IBus_UI_CD53 ||
        Context.uiMode == IBus_UI_BUSINESS_NAV
    ) {
        CD53Init(bt, ibus);
    } else if (Context.uiMode == IBus_UI_BMBT) {
        BMBTInit(bt, ibus);
    } else if (Context.uiMode == IBus_UI_MID) {
        MIDInit(bt, ibus);
    } else if (Context.uiMode == IBus_UI_MID_BMBT) {
        MIDInit(bt, ibus);
        BMBTInit(bt, ibus);
    }
    unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
    // Set the CVC Parameters
    if (micGain == 0x00) {
        micGain = 0xC0; // Default
    }
    BC127CommandSetMicGain(Context.bt, micGain);
}

/**
 * HandlerBC127Boot()
 *     Description:
 *         If the BC127 restarts, reset our internal state
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127Boot(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127ClearPairedDevices(context->bt);
    BC127CommandStatus(context->bt);
}

/**
 * HandlerBC127BootStatus()
 *     Description:
 *         If the BC127 Radios are off, meaning we rebooted and got the status
 *         back, then alter the module status to match the ignition status
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127BootStatus(void *ctx, unsigned char *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    context->btConnectionStatus = HANDLER_BT_CONN_OFF;
    BC127CommandList(context->bt);
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        // Set the BT module not connectable or discoverable and disconnect all devices
        BC127CommandBtState(context->bt, BC127_STATE_OFF, BC127_STATE_OFF);
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    } else {
        // Set the connectable and discoverable states to what they were
        BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
    }
}

void HandlerBC127CallStatus(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If we were playing before the call, try to resume playback
    if (context->bt->callStatus == BC127_CALL_INACTIVE &&
        context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING
    ) {
        BC127CommandPlay(context->bt);
    }
    if (ConfigGetSetting(CONFIG_SETTING_TCU_MODE) == CONFIG_SETTING_OFF ||
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING
    ) {
        if ((context->bt->callStatus == BC127_CALL_INCOMING ||
            context->bt->callStatus == BC127_CALL_OUTGOING) &&
            context->bt->scoStatus == BC127_CALL_SCO_OPEN
        ) {
            // Enable the amp and mute the radio
            PAM_SHDN = 1;
            TEL_MUTE = 1;
        }
        // Close the call immediately, without waiting for SCO to close
        if (context->bt->callStatus == BC127_CALL_INACTIVE) {
            // Disable the amp and unmute the radio
            PAM_SHDN = 0;
            TimerDelayMicroseconds(250);
            TEL_MUTE = 0;
        }
    }
}

/**
 * HandlerBC127DeviceLinkConnected()
 *     Description:
 *         If a device link is opened, disable connectability once all profiles
 *         are opened. Otherwise if the ignition is off, disconnect all devices
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
        // If HFP is enabled, do not disable connectability until the
        // profile opens
        if (context->bt->activeDevice.avrcpLinkId != 0 &&
            context->bt->activeDevice.a2dpLinkId != 0
        ) {
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF ||
                context->bt->activeDevice.hfpLinkId != 0
            ) {
                LogDebug(LOG_SOURCE_SYSTEM, "Handler: Disable connectability");
                BC127CommandBtState(
                    context->bt,
                    BC127_STATE_OFF,
                    context->bt->discoverable
                );
                if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON &&
                    context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
                ) {
                    BC127CommandPlay(context->bt);
                }
            } else if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON &&
                       context->bt->activeDevice.hfpLinkId == 0
            ) {
                char *macId = (char *) context->bt->activeDevice.macId;
                BC127CommandProfileOpen(context->bt, macId, "HFP");
            }
        }
    } else {
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    }
}

/**
 * HandlerBC127DeviceDisconnected()
 *     Description:
 *         If a device disconnects and our ignition is on,
 *         make the module connectable again.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBC127DeviceDisconnected(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Reset the metadata so we don't display the wrong data
    BC127ClearMetadata(context->bt);
    BC127ClearPairingErrors(context->bt);
    if (context->ibus->ignitionStatus == IBUS_IGNITION_ON) {
        BC127CommandBtState(
            context->bt,
            BC127_STATE_ON,
            context->bt->discoverable
        );
        if (context->btConnectionStatus == HANDLER_BT_CONN_CHANGE) {
            BC127PairedDevice_t *dev = &context->bt->pairedDevices[
                context->btSelectedDevice
            ];
            BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                BC127CommandProfileOpen(context->bt, dev->macId, "HFP");
            }
            context->btSelectedDevice = HANDLER_BT_SELECTED_DEVICE_NONE;
        } else {
            BC127CommandList(context->bt);
        }
    }
    context->btConnectionStatus = HANDLER_BT_CONN_OFF;
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
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: No Device -- Attempt connection");
        BC127CommandProfileOpen(context->bt, macId, "A2DP");
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            BC127CommandProfileOpen(context->bt, macId, "HFP");
        }
        context->btConnectionStatus = HANDLER_BT_CONN_ON;
    } else {
        LogDebug(
            LOG_SOURCE_SYSTEM,
            "Handler: Not connecting to new device %d %d %d",
            context->bt->activeDevice.deviceId,
            context->btConnectionStatus,
            context->ibus->ignitionStatus
        );
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
    // If this is the first status update
    if (context->btStartupIsRun == 0) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            // Request Metadata
            BC127CommandGetMetadata(context->bt);
        }
        context->btStartupIsRun = 1;
    }
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING
    ) {
        // We're playing but not in Bluetooth mode - stop playback
        BC127CommandPause(context->bt);
    }
}

/**
 * HandlerUICloseConnection()
 *     Description:
 *         Close the active connection and dissociate ourselves from it
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerUICloseConnection(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Reset the metadata so we don't display the wrong data
    BC127ClearMetadata(context->bt);
    // Clear the actively paired device
    BC127ClearActiveDevice(context->bt);
    // Enable connectivity
    BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
    BC127CommandClose(context->bt, BC127_CLOSE_ALL);

}

/**
 * HandlerUIInitiateConnection()
 *     Description:
 *         Handle the connection when a new device is selected in the UI
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerUIInitiateConnection(void *ctx, unsigned char *deviceId)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->activeDevice.deviceId != 0) {
        BC127CommandClose(context->bt, BC127_CLOSE_ALL);
    }
    context->btSelectedDevice = (int8_t) *deviceId;
    context->btConnectionStatus = HANDLER_BT_CONN_CHANGE;
}

/**
 * HandlerIBusCDCPoll()
 *     Description:
 *         Respond to the Radio's "ping" with a "pong"
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusCDCPoll(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    IBusCommandCDCPollResponse(context->ibus);
    context->cdChangerLastPoll = TimerGetMillis();
}

/**
 * HandlerIBusCDCStatus()
 *     Description:
 *         Track the current CD Changer status based on what the radio
 *         instructs us to do. We respond with exactly what the radio instructs
 *         even if we haven't done it yet. Otherwise, the radio will continue
 *         to accost us to do what it wants
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusCDCStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char curStatus = 0x00;
    unsigned char curFunction = 0x00;
    unsigned char requestedCommand = pkt[4];
    if (requestedCommand == IBUS_CDC_CMD_GET_STATUS) {
        curFunction = context->ibus->cdChangerFunction;
        if (curFunction == IBUS_CDC_FUNC_PLAYING) {
            curStatus = IBUS_CDC_STAT_PLAYING;
        } else if (curFunction == IBUS_CDC_FUNC_PAUSE) {
            curStatus = IBUS_CDC_STAT_PAUSE;
        }
    } else if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
        curStatus = IBUS_CDC_STAT_STOP;
        curFunction = IBUS_CDC_FUNC_NOT_PLAYING;
    } else if (requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK) {
        curFunction = context->ibus->cdChangerFunction;
        curStatus = IBUS_CDC_STAT_PLAYING;
        // Do not go backwards/forwards if the UI is CD53 because
        // those actions can be used to use the UI
        if (context->uiMode != IBus_UI_CD53) {
            if (pkt[5] == 0x00) {
                BC127CommandForward(context->bt);
            } else {
                BC127CommandBackward(context->bt);
            }
        }
    } else if (requestedCommand == IBUS_CDC_CMD_SEEK) {
        if (pkt[5] == 0x00) {
            context->seekMode = HANDLER_CDC_SEEK_MODE_REV;
            BC127CommandBackwardSeekPress(context->bt);
        } else {
            context->seekMode = HANDLER_CDC_SEEK_MODE_FWD;
            BC127CommandForwardSeekPress(context->bt);
        }
        curStatus = IBUS_CDC_STAT_FAST_REV;
    } else if (requestedCommand == IBUS_CDC_CMD_SCAN) {
        curStatus = 0x00;
        // The 5th octet in the packet tells the CDC if we should
        // enable or disable the given mode
        if (pkt[5] == 0x01) {
            curFunction = IBUS_CDC_FUNC_SCAN_MODE;
        } else {
            curFunction = IBUS_CDC_FUNC_PLAYING;
        }
    } else if (requestedCommand == IBUS_CDC_CMD_RANDOM_MODE) {
        curStatus = 0x00;
        // The 5th octet in the packet tells the CDC if we should
        // enable or disable the given mode
        if (pkt[5] == 0x01) {
            curFunction = IBUS_CDC_FUNC_RANDOM_MODE;
        } else {
            curFunction = IBUS_CDC_FUNC_PLAYING;
        }
    } else {
        if (requestedCommand == IBUS_CDC_CMD_PAUSE_PLAYING) {
            curStatus = IBUS_CDC_STAT_PAUSE;
            curFunction = IBUS_CDC_FUNC_PAUSE;
        } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING) {
            curStatus = IBUS_CDC_STAT_PLAYING;
            curFunction = IBUS_CDC_FUNC_PLAYING;
            if (context->seekMode == HANDLER_CDC_SEEK_MODE_FWD) {
                BC127CommandForwardSeekRelease(context->bt);
                context->seekMode = HANDLER_CDC_SEEK_MODE_NONE;
            } else if (context->seekMode == HANDLER_CDC_SEEK_MODE_REV) {
                BC127CommandBackwardSeekRelease(context->bt);
                context->seekMode = HANDLER_CDC_SEEK_MODE_NONE;
            }
        } else {
            curStatus = requestedCommand;
        }
    }
    unsigned char discCount = IBUS_CDC_DISC_COUNT_6;
    if (context->uiMode == IBus_UI_BMBT) {
        discCount = IBUS_CDC_DISC_COUNT_1;
    }
    IBusCommandCDCStatus(context->ibus, curStatus, curFunction, discCount);
    context->cdChangerLastPoll = TimerGetMillis();
    context->cdChangerLastStatus = TimerGetMillis();
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
        // Play a tone to awaken the WM8804 / PCM5122
        BC127CommandTone(Context.bt, "V 0 N C6 L 4");
        // Anounce the CDC to the network
        IBusCommandCDCAnnounce(context->ibus);
        unsigned char discCount = IBUS_CDC_DISC_COUNT_6;
        if (context->uiMode == IBus_UI_BMBT) {
            discCount = IBUS_CDC_DISC_COUNT_1;
        }
        IBusCommandCDCStatus(
            context->ibus,
            IBUS_CDC_STAT_PLAYING,
            context->ibus->cdChangerFunction,
            discCount
        );
        // Reset the metadata so we don't display the wrong data
        BC127ClearMetadata(context->bt);
        // Set the BT module connectable
        BC127CommandBtState(context->bt, BC127_STATE_ON, BC127_STATE_OFF);
        // Request BC127 state
        BC127CommandStatus(context->bt);
        BC127CommandList(context->bt);
        // Ask the LCM for the I/O Status of all lamps
        if (ConfigGetVehicleType() == IBUS_VEHICLE_TYPE_E46_Z4) {
            IBusCommandDIAGetIOStatus(context->ibus, IBUS_DEVICE_LCM);
        }
    }
}

/**
 * HandlerIBusLightStatus()
 *     Description:
 *         Track the Light Status messages in case the user has configured
 *         Three/Five One-Touch Blinkers.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLCMLightStatus(void *ctx, unsigned char *pkt)
{
    uint8_t blinkCount = ConfigGetSetting(CONFIG_SETTING_OT_BLINKERS);
    if (blinkCount != 0x00 && blinkCount != 0xFF) {
        HandlerContext_t *context = (HandlerContext_t *) ctx;
        unsigned char lightStatus = pkt[4];
        if (context->blinkerStatus == HANDLER_BLINKER_OFF) {
            context->blinkerCount = 2;
            if (CHECK_BIT(lightStatus, IBUS_LCM_DRV_SIG_BIT) != 0 &&
                CHECK_BIT(lightStatus, IBUS_LCM_PSG_SIG_BIT) == 0
            ) {
                context->blinkerStatus = HANDLER_BLINKER_DRV;
                IBusCommandLCMEnableBlinker(context->ibus, IBUS_LCM_BLINKER_DRV);
            } else if (CHECK_BIT(lightStatus, IBUS_LCM_PSG_SIG_BIT) != 0 &&
                CHECK_BIT(lightStatus, IBUS_LCM_DRV_SIG_BIT) == 0
            ) {
                context->blinkerStatus = HANDLER_BLINKER_PSG;
                IBusCommandLCMEnableBlinker(context->ibus, IBUS_LCM_BLINKER_PSG);
            }
        } else if (context->blinkerStatus == HANDLER_BLINKER_DRV) {
            if (CHECK_BIT(lightStatus, IBUS_LCM_PSG_SIG_BIT) != 0 ||
                context->blinkerCount == blinkCount
            ) {
                // Reset ourselves once the signal is off so we do not reactivate
                if (CHECK_BIT(lightStatus, IBUS_LCM_DRV_SIG_BIT) == 0) {
                    context->blinkerStatus = HANDLER_BLINKER_OFF;
                }
                IBusCommandDIATerminateDiag(context->ibus, IBUS_DEVICE_LCM);
            } else {
                context->blinkerCount++;
            }
        } else if (context->blinkerStatus == HANDLER_BLINKER_PSG) {
            if (CHECK_BIT(lightStatus, IBUS_LCM_DRV_SIG_BIT) != 0 ||
                context->blinkerCount == blinkCount
            ) {
                // Reset ourselves once the signal is off so we do not reactivate
                if (CHECK_BIT(lightStatus, IBUS_LCM_PSG_SIG_BIT) == 0) {
                    context->blinkerStatus = HANDLER_BLINKER_OFF;
                }
                IBusCommandDIATerminateDiag(context->ibus, IBUS_DEVICE_LCM);
            } else {
                context->blinkerCount++;
            }
        }
    }
}

/**
 * HandlerIBusLCMDimmerStatus()
 *     Description:
 *         Track the Dimmer Status messages so we can correctly set the
 *         dimming state when messing with the lighting
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLCMDimmerStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetVehicleType() == IBUS_VEHICLE_TYPE_E46_Z4) {
        IBusCommandDIAGetIOStatus(context->ibus, IBUS_DEVICE_LCM);
    }
}

/**
 * HandlerIBusMFLButton()
 *     Description:
 *         Act upon MFL button presses when in CD Changer mode (when BT is active)
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The packet
 *     Returns:
 *         void
 */
void HandlerIBusMFLButton(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char mflButton = pkt[4];
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
        if (mflButton == IBusMFLButtonVoiceRelease) {
            if (context->bt->callStatus == BC127_CALL_ACTIVE) {
                BC127CommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_INCOMING) {
                BC127CommandCallAnswer(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_OUTGOING) {
                BC127CommandCallEnd(context->bt);
            }
        } else if (mflButton == IBusMFLButtonVoiceHold) {
            BC127CommandToggleVR(context->bt);
        }
    }
    if ((mflButton == IBusMFLButtonRTPress ||
        mflButton == IBusMFLButtonRTRelease) &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
    ) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        } else {
            BC127CommandPlay(context->bt);
        }
    }
}

/**
 * HandlerIBusMFLButton()
 *     Description:
 *         Act upon MFL Volume commands to control call volume
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The packet
 *     Returns:
 *         void
 */
void HandlerIBusMFLVolume(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->callStatus != BC127_CALL_INACTIVE) {
        if (pkt[4] == IBusMFLVolUp) {
            BC127CommandVolume(context->bt, 13, "UP");
        } else if (pkt[4] == IBusMFLVolDown) {
            BC127CommandVolume(context->bt, 13, "DOWN");
        }
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
    if ((now - context->cdChangerLastPoll) >= HANDLER_CDC_ANOUNCE_TIMEOUT &&
        context->ibus->ignitionStatus == IBUS_IGNITION_ON
    ) {
        IBusCommandCDCAnnounce(context->ibus);
        context->cdChangerLastPoll = now;
    }
}

/**
 * HandlerTimerCDCSendStatus()
 *     Description:
 *         This periodic task will proactively send the CDC status to the BM5x
 *         radio if we don't see a status poll within the last 20000 milliseconds.
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
        context->ibus->ignitionStatus == IBUS_IGNITION_ON &&
        (context->uiMode == IBus_UI_BMBT || context->uiMode == IBus_UI_MID_BMBT)
    ) {
        unsigned char curStatus = 0x00;
        if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
            curStatus = IBUS_CDC_STAT_PLAYING;
        }
        IBusCommandCDCStatus(
            context->ibus,
            curStatus,
            context->ibus->cdChangerFunction,
            IBUS_CDC_DISC_COUNT_1
        );
        context->cdChangerLastStatus = now;
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: Send CDC status preemptively");
    }
}

/**
 * HandlerTimerDeviceConnection()
 *     Description:
 *         Monitor the BT connection and ensure it stays connected
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerDeviceConnection(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (strlen(context->bt->activeDevice.macId) > 0 &&
        context->bt->activeDevice.a2dpLinkId == 0
    ) {
        if (context->btDeviceConnRetries <= HANDLER_DEVICE_MAX_RECONN) {
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Handler: A2DP link closed -- Attempting to connect"
            );
            BC127CommandProfileOpen(
                context->bt,
                context->bt->activeDevice.macId,
                "A2DP"
            );
            context->btDeviceConnRetries += 1;
        } else {
            LogError("Handler: Giving up on BT connection");
            context->btDeviceConnRetries = 0;
            // Enable connectivity
            BC127CommandBtState(context->bt, BC127_STATE_ON, context->bt->discoverable);
            BC127ClearPairedDevices(context->bt);
            BC127CommandClose(context->bt, BC127_CLOSE_ALL);
        }
    } else if (context->btDeviceConnRetries > 0) {
        context->btDeviceConnRetries = 0;
    }
}

/**
 * HandlerTimerOpenProfileErrors()
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
                LogDebug(LOG_SOURCE_SYSTEM, "Handler: Attempting to resolve pairing error");
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
 * HandlerTimerPoweroff()
 *     Description:
 *         Track the time since the last IBus message and see if we need to
 *         power off.
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerPoweroff(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char powerTimeout = ConfigGetPoweroffTimeout();
    if (powerTimeout != 0) {
        uint8_t lastRxMinutes = (TimerGetMillis() - context->ibus->rxLastStamp) / 60000;
        if (lastRxMinutes > powerTimeout) {
            if (context->powerStatus == HANDLER_POWER_ON) {
                // Destroy the UART module for IBus
                UARTDestroy(IBUS_UART_MODULE);
                TimerDelayMicroseconds(500);
                LogInfo(LOG_SOURCE_SYSTEM, "System Power Down!");
                context->powerStatus = HANDLER_POWER_OFF;
                // Disable the TH3122
                IBUS_EN = 0;
            } else {
                // Re-enable the TH3122 EN line so we can try pulling it,
                // and the regulator low again
                IBUS_EN = 1;
                context->powerStatus = HANDLER_POWER_ON;
            }
        }
    }
}

/**
 * HandlerTimerScanDevices()
 *     Description:
 *         Rescan for devices on the PDL periodically. Scan every 5 seconds if
 *         there is no connected device, otherwise every 60 seconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (((context->bt->activeDevice.deviceId == 0 &&
        context->btConnectionStatus == HANDLER_BT_CONN_OFF) ||
        context->scanIntervals == 12) &&
        context->ibus->ignitionStatus == IBUS_IGNITION_ON
    ) {
        context->scanIntervals = 0;
        BC127ClearInactivePairedDevices(context->bt);
        BC127CommandList(context->bt);
    } else {
        context->scanIntervals += 1;
    }
}
