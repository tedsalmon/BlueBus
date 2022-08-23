/*
 * File: handler_ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic around I/K-Bus events
 */
#include "handler_ibus.h"

void HandlerIBusInit(HandlerContext_t *context)
{
    EventRegisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &HandlerIBusCDCStatus,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_DSPConfigSet,
        &HandlerIBusDSPConfigSet,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_FirstMessageReceived,
        &HandlerIBusFirstMessageReceived,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_DoorsFlapsStatusResponse,
        &HandlerIBusGMDoorsFlapsStatusResponse,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_GTDIAIdentityResponse,
        &HandlerIBusGTDIAIdentityResponse,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_GTDIAOSIdentityResponse,
        &HandlerIBusGTDIAOSIdentityResponse,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKEIgnitionStatus,
        &HandlerIBusIKEIgnitionStatus,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKESpeedRPMUpdate,
        &HandlerIBusIKESpeedRPMUpdate,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKE_VEHICLE_CONFIG,
        &HandlerIBusIKEVehicleConfig,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_LCMLightStatus,
        &HandlerIBusLMLightStatus,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_LCMDimmerStatus,
        &HandlerIBusLMDimmerStatus,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_LCMRedundantData,
        &HandlerIBusLMRedundantData,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_LMIdentResponse,
        &HandlerIBusLMIdentResponse,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_MFLButton,
        &HandlerIBusMFLButton,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_ModuleStatusRequest,
        &HandlerIBusModuleStatusRequest,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_ModuleStatusResponse,
        &HandlerIBusModuleStatusResponse,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_PDC_STATUS,
        &HandlerIBusPDCStatus,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_RADVolumeChange,
        &HandlerIBusRADVolumeChange,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_SENSOR_VALUE_UPDATE,
        &HandlerIBusSensorValueUpdate,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_TELVolumeChange,
        &HandlerIBusTELVolumeChange,
        context
    );
    TimerRegisterScheduledTask(
        &HandlerTimerIBusCDCAnnounce,
        context,
        HANDLER_INT_CDC_ANOUNCE
    );
    TimerRegisterScheduledTask(
        &HandlerTimerIBusCDCSendStatus,
        context,
        HANDLER_INT_CDC_STATUS
    );
    TimerRegisterScheduledTask(
        &HandlerTimerIBusPings,
        context,
        HANDLER_INT_IBUS_PINGS
    );
    TimerRegisterScheduledTask(
        &HandlerTimerIBusLCMIOStatus,
        context,
        HANDLER_INT_LCM_IO_STATUS
    );
    context->lightingStateTimerId = TimerRegisterScheduledTask(
        &HandlerTimerIBusLightingState,
        context,
        HANDLER_INT_LIGHTING_STATE
    );
    if (ConfigGetSetting(CONFIG_SETTING_IGN_ALWAYS_ON) == CONFIG_SETTING_ON) {
        IBusSetInternalIgnitionStatus(context->ibus, IBUS_IGNITION_KL15);
    }
}

/**
 * HandlerIBusBroadcastCDCStatus()
 *     Description:
 *         Wrapper to send the CDC Status
 *     Params:
 *         HandlerContext_t *context - The handler context
 *     Returns:
 *         void
 */
static void HandlerIBusBroadcastCDCStatus(HandlerContext_t *context)
{
    unsigned char curStatus = IBUS_CDC_STAT_STOP;
    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PAUSE) {
        curStatus = IBUS_CDC_FUNC_PAUSE;
    } else if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
        curStatus = IBUS_CDC_STAT_PLAYING;
    }
    unsigned char discCount = IBUS_CDC_DISC_COUNT_6;
    // Report disc 7 loaded so any button press causes a CD Changer command
    // to be sent by the RAD (since there is no 7th disc)
    unsigned char discNumber = 0x07;
    if (context->uiMode == CONFIG_UI_BMBT) {
        discCount = IBUS_CDC_DISC_COUNT_1;
        discNumber = 0x01;
    }
    IBusCommandCDCStatus(
        context->ibus,
        curStatus,
        context->ibus->cdChangerFunction,
        discCount,
        discNumber
    );
    context->cdChangerLastStatus = TimerGetMillis();
}

static uint8_t HandlerIBusGetIsIgnitionStatusOn(HandlerContext_t *context)
{
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF &&
        (context->ibus->ignitionStatus != IBUS_IGNITION_KL99 ||
         context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING)
    ) {
        return 1;
    }
    return 0;
}

/**
 * HandlerIBusLMActivateBulbs()
 *     Description:
 *         Abstract function to set the LM Bulb I/O status and
 *         reset the lighting timer
 *     Params:
 *         HandlerContext_t *context - The handler context
 *         uint8_t event - The event that occurred
 *     Returns:
 *         void
 */
static void HandlerIBusLMActivateBulbs(
    HandlerContext_t *context,
    unsigned char event
) {
    context->lmLastStatusSet = TimerGetMillis();
    TimerResetScheduledTask(context->lightingStateTimerId);
    unsigned char blinkers = context->lmState.comfortBlinkerStatus;
    unsigned char parkingLamps = context->lmState.comfortParkingLampsStatus;
    switch (event) {
        case HANDLER_LM_EVENT_ALL_OFF:
            parkingLamps = HANDLER_LM_COMF_PARKING_OFF;
            context->lmState.comfortParkingLampsStatus = HANDLER_LM_COMF_PARKING_OFF;
            blinkers = HANDLER_LM_COMF_BLINK_OFF;
            context->lmState.comfortBlinkerStatus = HANDLER_LM_COMF_BLINK_OFF;
            break;
        case HANDLER_LM_EVENT_BLINK_OFF:
            blinkers = HANDLER_LM_COMF_BLINK_OFF;
            context->lmState.comfortBlinkerStatus = HANDLER_LM_COMF_BLINK_OFF;
            break;
        case HANDLER_LM_EVENT_BLINK_LEFT:
            blinkers = HANDLER_LM_COMF_BLINK_LEFT;
            context->lmState.comfortBlinkerStatus = HANDLER_LM_COMF_BLINK_LEFT;
            break;
        case HANDLER_LM_EVENT_BLINK_RIGHT:
            blinkers = HANDLER_LM_COMF_BLINK_RIGHT;
            context->lmState.comfortBlinkerStatus = HANDLER_LM_COMF_BLINK_RIGHT;
            break;
        case HANDLER_LM_EVENT_PARKING_OFF:
            parkingLamps = HANDLER_LM_COMF_PARKING_OFF;
            context->lmState.comfortParkingLampsStatus = HANDLER_LM_COMF_PARKING_OFF;
            break;
        case HANDLER_LM_EVENT_PARKING_ON:
            parkingLamps = HANDLER_LM_COMF_PARKING_ON;
            context->lmState.comfortParkingLampsStatus = HANDLER_LM_COMF_PARKING_ON;
            break;
    }
    IBusCommandLMActivateBulbs(context->ibus, blinkers, parkingLamps);
}

static void HandlerIBusSwitchUI(HandlerContext_t *context, unsigned char newUi)
{
    // Unregister the previous UI
    if (context->uiMode != newUi) {
        // Unregister the previous UI
        if (context->uiMode == CONFIG_UI_CD53 ||
            context->uiMode == CONFIG_UI_BUSINESS_NAV
        ) {
            CD53Destroy();
        } else if (context->uiMode == CONFIG_UI_BMBT) {
            BMBTDestroy();
        } else if (context->uiMode == CONFIG_UI_MID) {
            MIDDestroy();
        } else if (context->uiMode == CONFIG_UI_MID_BMBT) {
            MIDDestroy();
            BMBTDestroy();
        }
        if (newUi == CONFIG_UI_CD53 || newUi == CONFIG_UI_BUSINESS_NAV) {
            CD53Init(context->bt, context->ibus);
        } else if (newUi == CONFIG_UI_BMBT) {
            BMBTInit(context->bt, context->ibus);
        } else if (newUi == CONFIG_UI_MID) {
            MIDInit(context->bt, context->ibus);
        } else if (newUi == CONFIG_UI_MID_BMBT) {
            MIDInit(context->bt, context->ibus);
            BMBTInit(context->bt, context->ibus);
        }
        ConfigSetUIMode(newUi);
        context->uiMode = newUi;
     }
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
    unsigned char curStatus = IBUS_CDC_STAT_STOP;
    unsigned char curFunction = IBUS_CDC_FUNC_NOT_PLAYING;
    unsigned char requestedCommand = pkt[4];
    if (requestedCommand == IBUS_CDC_CMD_GET_STATUS) {
        curFunction = context->ibus->cdChangerFunction;
        if (curFunction == IBUS_CDC_FUNC_PLAYING) {
            curStatus = IBUS_CDC_STAT_PLAYING;
        } else if (curFunction == IBUS_CDC_FUNC_PAUSE) {
            curStatus = IBUS_CDC_STAT_PAUSE;
        }
        if (TimerGetMillis() > 30000) {
            if (context->ibusModuleStatus.MID == 0 &&
                context->ibusModuleStatus.GT == 0 &&
                context->ibusModuleStatus.BMBT == 0 &&
                context->ibusModuleStatus.VM == 0 &&
                context->uiMode != CONFIG_UI_CD53
            ) {
                // Fallback for vehicle UI Identification
                // If no UI has been detected and we have been
                // running at least 30s, default to CD53 UI
                LogInfo(LOG_SOURCE_SYSTEM, "Fallback to CD53 UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_CD53);
            } else if (context->ibusModuleStatus.GT == 1 &&
                       context->gtStatus == HANDLER_GT_STATUS_UNCHECKED
            ) {
                // Request the Navigation Identity
                IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_GT);
                context->gtStatus = HANDLER_GT_STATUS_CHECKED;
            }
        }
    } else if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
            BTCommandPause(context->bt);
        }
        curStatus = IBUS_CDC_STAT_STOP;
        curFunction = IBUS_CDC_FUNC_NOT_PLAYING;
        // Return to non-S/PDIF input once told to stop playback, if enabled
        if (ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC) == CONFIG_SETTING_DSP_INPUT_SPDIF) {
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
        }
        if (context->ibus->ignitionStatus == IBUS_IGNITION_KL99) {
            IBusSetInternalIgnitionStatus(context->ibus, IBUS_IGNITION_OFF);
        }
    } else if (requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK ||
               requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK_BLAUPUNKT
    ) {
        curFunction = context->ibus->cdChangerFunction;
        curStatus = IBUS_CDC_STAT_PLAYING;
        // Do not go backwards/forwards if the UI is CD53 because
        // those actions can be used to use the UI
        if (context->uiMode != CONFIG_UI_CD53) {
            if (pkt[5] == 0x00) {
                BTCommandPlaybackTrackNext(context->bt);
            } else {
                BTCommandPlaybackTrackPrevious(context->bt);
            }
        }
    } else if (requestedCommand == IBUS_CDC_CMD_SEEK) {
        if (pkt[5] == 0x00) {
            context->seekMode = HANDLER_CDC_SEEK_MODE_REV;
            BTCommandPlaybackTrackRewindStart(context->bt);
        } else {
            context->seekMode = HANDLER_CDC_SEEK_MODE_FWD;
            BTCommandPlaybackTrackFastforwardStart(context->bt);
        }
        curStatus = IBUS_CDC_STAT_FAST_REV;
    } else if (requestedCommand == IBUS_CDC_CMD_CD_CHANGE) {
        curStatus = IBUS_CDC_STAT_PLAYING;
        curFunction = IBUS_CDC_FUNC_PLAYING;
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
                BTCommandPlaybackTrackFastforwardStop(context->bt);
                context->seekMode = HANDLER_CDC_SEEK_MODE_NONE;
            } else if (context->seekMode == HANDLER_CDC_SEEK_MODE_REV) {
                BTCommandPlaybackTrackRewindStop(context->bt);
                context->seekMode = HANDLER_CDC_SEEK_MODE_NONE;
            }
            if (context->ibusModuleStatus.DSP == 1) {
                unsigned char dspInput = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
                // Set the Input to S/PDIF once told to start playback, if enabled
                if (dspInput == CONFIG_SETTING_DSP_INPUT_SPDIF) {
                    IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
                } else if (dspInput == CONFIG_SETTING_DSP_INPUT_ANALOG) {
                    // Set the Input to the radio if we are overridden
                    IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
                }
            }
            if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
                LogWarning("SET KL-99");
                IBusSetInternalIgnitionStatus(context->ibus, IBUS_IGNITION_KL99);
            }
        } else {
            curStatus = requestedCommand;
        }
    }
    unsigned char discCount = IBUS_CDC_DISC_COUNT_6;
    // Report disc 7 loaded so any button press causes a CD Changer command
    // to be sent by the RAD (since there is no 7th disc)
    unsigned char discNumber = 0x07;
    if (context->uiMode == CONFIG_UI_BMBT) {
        discCount = IBUS_CDC_DISC_COUNT_1;
        discNumber = 0x01;
    }
    IBusCommandCDCStatus(
        context->ibus,
        curStatus,
        curFunction,
        discCount,
        discNumber
    );
    context->cdChangerLastPoll = TimerGetMillis();
    context->cdChangerLastStatus = TimerGetMillis();
}

/**
 * HandlerIBusDSPConfigSet()
 *     Description:
 *         Sort through received DSP configurations to ensure
 *         the correct input source is selected when appropriate
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusDSPConfigSet(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char dspInput = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
    if (context->ibusModuleStatus.DSP == 1 &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
    ) {
        if (pkt[4] == IBUS_DSP_CONFIG_SET_INPUT_RADIO &&
            dspInput == CONFIG_SETTING_DSP_INPUT_SPDIF
        ) {
            // Set the Input to S/PDIF if we are overridden
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
        }
        if (pkt[4] == IBUS_DSP_CONFIG_SET_INPUT_SPDIF &&
            dspInput == CONFIG_SETTING_DSP_INPUT_ANALOG
        ) {
            // Set the Input to the radio if we are overridden
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
        }
    }
    // Identify the vehicle using S/PDIF so we can use additional features
    if (pkt[4] == IBUS_DSP_CONFIG_SET_INPUT_SPDIF && dspInput == CONFIG_SETTING_OFF) {
        ConfigSetSetting(
            CONFIG_SETTING_DSP_INPUT_SRC,
            CONFIG_SETTING_DSP_INPUT_SPDIF
        );
    }
}

/**
 * HandlerIBusFirstMessageReceived()
 *     Description:
 *         Request module status after the first IBus message is received.
 *         DO NOT change the order in which these modules are polled.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusFirstMessageReceived(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibusModulePingState == HANDLER_IBUS_MODULE_PING_STATE_OFF) {
        context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_READY;
    }
}

/**
 * HandlerIBusGMDoorsFlapStatusResponse()
 *     Description:
 *         Track which doors have been opened while the ignition was on
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The navigation type
 *     Returns:
 *         void
 */
void HandlerIBusGMDoorsFlapsStatusResponse(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->gmState.lowSideDoors == 0) {
        unsigned char doorStatus = pkt[4] & 0x0F;
        if (doorStatus > 0x01) {
            context->gmState.lowSideDoors = 1;
        }
    }
    // The 5th bit in the first data byte contains the lock status
    if (CHECK_BIT(pkt[4], 5) != 0) {
        LogInfo(LOG_SOURCE_SYSTEM, "Handler: Central Locks locked");
        context->gmState.doorsLocked = 1;
    } else {
        LogInfo(LOG_SOURCE_SYSTEM, "Handler: Central Locks unlocked");
        context->gmState.doorsLocked = 0;
    }
}

/**
 * HandlerIBusGTDIAIdentityResponse()
 *     Description:
 *         Identify the navigation module hardware and software versions
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The navigation type
 *     Returns:
 *         void
 */
void HandlerIBusGTDIAIdentityResponse(void *ctx, unsigned char *type)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char navType = *type;
    if (ConfigGetNavType() != navType) {
        ConfigSetNavType(navType);
    }
    if (navType != IBUS_GT_DETECT_ERROR && navType < IBUS_GT_MKIII_NEW_UI) {
        // Assume this is a color graphic terminal as GT's < IBUS_GT_MKIII_NEW_UI
        // do not support the OS identity request
        if (context->ibusModuleStatus.MID == 0) {
            if (ConfigGetUIMode() != CONFIG_UI_BMBT) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected BMBT UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_BMBT);
            }
        } else {
            if (ConfigGetUIMode() != CONFIG_UI_MID_BMBT) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID / BMBT UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_MID_BMBT);
            }
        }
    }
    // Request the OS Identity (Color or Monochrome Nav)
    IBusCommandDIAGetOSIdentity(context->ibus, IBUS_DEVICE_GT);
}

/**
 * HandlerIBusGTDIAOSIdentityResponse()
 *     Description:
 *         Identify the navigation module type from its OS.
 *         Note: MKII, and presumably older OS modules do NOT respond to this
 *         query
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - Data packet
 *     Returns:
 *         void
 */
void HandlerIBusGTDIAOSIdentityResponse(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    char navigationOS[8] = {};
    uint8_t i;
    for (i = 0; i < 7; i++) {
        navigationOS[i] = pkt[i + 4];
    }
    // The string should come null terminated, but we should not trust that
    navigationOS[7] = '\0';
    if (UtilsStricmp(navigationOS, "BMWC01S") == 0) {
        if (context->ibusModuleStatus.MID == 0) {
            if (ConfigGetUIMode() != CONFIG_UI_BMBT) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected BMBT UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_BMBT);
            }
        } else {
            if (ConfigGetUIMode() != CONFIG_UI_MID_BMBT) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID / BMBT UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_MID_BMBT);
            }
        }
    } else if (UtilsStricmp(navigationOS, "BMWM01S") == 0) {
        if (ConfigGetUIMode() != CONFIG_UI_BUSINESS_NAV) {
            LogInfo(LOG_SOURCE_SYSTEM, "Detected Business Nav UI");
            HandlerIBusSwitchUI(context, CONFIG_UI_BUSINESS_NAV);
        }
    } else {
        LogError("Unable to identify GT OS: %s", navigationOS);
    }
}

/**
 * HandlerIBusIKEIgnitionStatus()
 *     Description:
 *         Track the Ignition state and update the BT Module accordingly. We set
 *         the BT device "off" when the key is set to position 0 and on
 *         as soon as it goes to a position >= 1.
 *         Request the LCM status when the car is turned to or past position 1
 *         Unlock the vehicle once the key is turned to position 1
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusIKEIgnitionStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char ignitionStatus = pkt[0];
    // If the ignition status has changed
    if (ignitionStatus != context->ibus->ignitionStatus) {
        // If the first bit is set, the key is in position 1 at least, otherwise
        // the ignition is off
        if (ignitionStatus == IBUS_IGNITION_OFF) {
            // Disable Telephone On
            UtilsSetPinMode(UTILS_PIN_TEL_ON, 0);
            // Set the BT module not connectable/discoverable. Disconnect all devices
            BTCommandSetConnectable(context->bt, BT_STATE_OFF);
            if (context->bt->discoverable == BT_STATE_ON) {
                BTCommandSetDiscoverable(context->bt, BT_STATE_OFF);
            }
            BTCommandDisconnect(context->bt);
            BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
            // Unlock the vehicle
            if (ConfigGetComfortUnlock() == CONFIG_SETTING_COMFORT_UNLOCK_POS_0 &&
                context->gmState.doorsLocked == 1
            ) {
                if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                    IBusCommandGMDoorCenterLockButton(context->ibus);
                } else if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
                    if (context->gmState.lowSideDoors == 1) {
                        IBusCommandGMDoorUnlockAll(context->ibus);
                    } else {
                        IBusCommandGMDoorUnlockHigh(context->ibus);
                    }
                }
            }
            context->gmState.lowSideDoors = 0;
        // If the engine was on, but now it's in position 1
        } else if (context->ibus->ignitionStatus >= IBUS_IGNITION_KL15 &&
                   ignitionStatus == IBUS_IGNITION_KLR
        ) {
            // Unlock the vehicle
            if (ConfigGetComfortUnlock() == CONFIG_SETTING_COMFORT_UNLOCK_POS_1 &&
                context->gmState.doorsLocked == 1
            ) {
                if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                    IBusCommandGMDoorCenterLockButton(context->ibus);
                } else if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
                    if (context->gmState.lowSideDoors == 1) {
                        IBusCommandGMDoorUnlockAll(context->ibus);
                    } else {
                        IBusCommandGMDoorUnlockHigh(context->ibus);
                    }
                }
            }
            if (context->lmState.comfortBlinkerStatus != HANDLER_LM_COMF_BLINK_OFF ||
                context->lmState.comfortParkingLampsStatus != HANDLER_LM_COMF_PARKING_OFF
            ) {
                HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_ALL_OFF);
            }
        // If the ignition WAS off, but now it's not, then run these actions.
        // I realize the second condition is frivolous, but it helps with
        // readability.
        } else if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF &&
                   ignitionStatus != IBUS_IGNITION_OFF
        ) {
            LogDebug(LOG_SOURCE_SYSTEM, "Handler: Ignition On");
            // Enable Telephone On
            UtilsSetPinMode(UTILS_PIN_TEL_ON, 1);
            // Reset the metadata so we don't display the wrong data
            BTClearMetadata(context->bt);
            // Set the BT module connectable
            BTCommandSetConnectable(context->bt, BT_STATE_ON);
            BTCommandList(context->bt);
            if (context->bt->type == BT_BTM_TYPE_BC127) {
                // Play a tone to wake up the WM8804 / PCM5122
                BC127CommandTone(context->bt, "V 0 N C6 L 4");
                // Request BC127 state
                BC127CommandStatus(context->bt);
            } else {
                if (context->bt->pairedDevicesCount > 0) {
                    uint8_t lastDevice = ConfigGetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE);
                    BTPairedDevice_t *dev = &context->bt->pairedDevices[lastDevice];
                    BTCommandConnect(context->bt, dev);
                }
            }
            // Enable the TEL LEDs
            if (ConfigGetTelephonyFeaturesActive() == CONFIG_SETTING_ON) {
                if (context->bt->activeDevice.avrcpId == 0 &&
                    context->bt->activeDevice.a2dpId == 0
                ) {
                    IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED);
                } else {
                    IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_GREEN);
                }
            }
            IBusCommandSetModuleStatus(
                context->ibus,
                IBUS_DEVICE_CDC,
                IBUS_DEVICE_LOC,
                0x01
            );
            context->cdChangerLastPoll = TimerGetMillis();
            // Ask the LCM for the redundant data
            LogDebug(LOG_SOURCE_SYSTEM, "Handler: Request LCM Redundant Data");
            IBusCommandLMGetRedundantData(context->ibus);
        }
    } else {
        if (ignitionStatus > IBUS_IGNITION_OFF) {
            HandlerIBusBroadcastCDCStatus(context);
            if (ConfigGetTelephonyFeaturesActive() == CONFIG_SETTING_ON) {
                HandlerSetIBusTELStatus(context, HANDLER_TEL_STATUS_FORCE);
                if (context->bt->activeDevice.avrcpId != 0 &&
                    context->bt->activeDevice.a2dpId != 0
                ) {
                    IBusCommandTELSetLED(
                        context->ibus,
                        IBUS_TEL_LED_STATUS_GREEN
                    );
                } else {
                    IBusCommandTELSetLED(
                        context->ibus,
                        IBUS_TEL_LED_STATUS_RED
                    );
                }
            }
            // Set the IKE to "found" if we haven't already to prevent
            // sending the telephone status multiple times
            context->ibusModuleStatus.IKE = 1;
        }
    }
    if (context->ibusModuleStatus.IKE == 0) {
        HandlerSetIBusTELStatus(context, HANDLER_TEL_STATUS_FORCE);
        context->ibusModuleStatus.IKE = 1;
    }
}

/**
 * HandlerIBusIKESpeedRPMUpdate()
 *     Description:
 *         Act upon updates from the IKE about the vehicle speed / RPM
 *         * Lock the vehicle at 20mph
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusIKESpeedRPMUpdate(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char comfortLock = ConfigGetComfortLock();
    if (comfortLock != CONFIG_SETTING_OFF &&
        context->gmState.doorsLocked == 0x00
    ) {
        uint16_t speed = pkt[4] * 2;
        if ((comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM && speed >= 10) ||
            (comfortLock == CONFIG_SETTING_COMFORT_LOCK_20KM && speed >= 20)
        ) {
            if (context->ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                IBusCommandGMDoorCenterLockButton(context->ibus);
            } else {
                IBusCommandGMDoorLockAll(context->ibus);
            }
        }
    }
}

/**
 * HandlerIBusIKEVehicleConfig()
 *     Description:
 *         Handle updates to the vehicle configuration values
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus Packet
 *     Returns:
 *         void
 */
void HandlerIBusIKEVehicleConfig(void *ctx, unsigned char *pkt)
{
    unsigned char rawVehicleType = (pkt[4] >> 4) & 0xF;
    unsigned char detectedVehicleType = IBusGetVehicleType(pkt);
    if (detectedVehicleType == 0xFF) {
        LogError("Handler: Unknown Vehicle Detected");
    }
    if (detectedVehicleType != ConfigGetVehicleType() &&
        detectedVehicleType != 0xFF
    ) {
        ConfigSetVehicleType(detectedVehicleType);
        if (rawVehicleType == 0x0A || rawVehicleType == 0x0F) {
            ConfigSetIKEType(IBUS_IKE_TYPE_LOW);
            LogDebug(LOG_SOURCE_SYSTEM, "Detected New Vehicle Type: E46/Z4");
        } else if (rawVehicleType == 0x02) {
            ConfigSetIKEType(IBUS_IKE_TYPE_LOW);
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Detected New Vehicle Type: E38/E39/E53 - Low OBC"
            );
        } else if (rawVehicleType == 0x00) {
            ConfigSetIKEType(IBUS_IKE_TYPE_HIGH);
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Detected New Vehicle Type: E38/E39/E53 - High OBC"
            );
        }
    }
}

/**
 * HandlerIBusLMIdentResponse()
 *     Description:
 *         Identify the light module variant
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The light module variant
 *     Returns:
 *         void
 */
void HandlerIBusLMIdentResponse(void *ctx, unsigned char *variant)
{
    unsigned char lmVariant = *variant;
    if (ConfigGetLMVariant() != lmVariant) {
        ConfigSetLMVariant(lmVariant);
    }
}

/**
 * HandlerIBusLMLightStatus()
 *     Description:
 *         Track the Light Status messages in case the user has configured
 *         Three/Five One-Touch Blinkers.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLMLightStatus(void *ctx, unsigned char *pkt)
{
    // Changed identifier as to not confuse it with the blink counter.
    uint8_t configBlinkLimit = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
    unsigned char parkingLamps = ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS);
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (configBlinkLimit > 1) {
        unsigned char lightStatus = pkt[4];
        unsigned char lightStatus2 = pkt[6];
        // Left blinker
        if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) != 0 &&
            CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) == 0 &&
            CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) != 0
        ) {
            // If quickly switching blinker direction the LM will activate the
            // opposing blinker immediately, bypassing the "off" message.
            if (context->lmState.blinkStatus == HANDLER_LM_BLINK_RIGHT) {
                LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "LEFT > Quick Switch > Reset");
                context->lmState.blinkCount = 0;
            }
            context->lmState.blinkCount++;
            context->lmState.blinkStatus = HANDLER_LM_BLINK_LEFT;

            switch (context->lmState.comfortBlinkerStatus) {
                case HANDLER_LM_COMF_BLINK_OFF:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "LEFT > COMFORT_INACTIVE > %d/%d",
                        context->lmState.blinkCount,
                        configBlinkLimit
                    );
                    break;
                case HANDLER_LM_COMF_BLINK_LEFT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "LEFT > COMFORT_LEFT > %d/%d",
                        context->lmState.blinkCount,
                        configBlinkLimit
                    );
                    if (context->lmState.blinkCount >= configBlinkLimit) {
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "LEFT > COMFORT_LEFT > Blink limit"
                        );
                        HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_OFF);
                    }
                    break;
                case HANDLER_LM_COMF_BLINK_RIGHT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "LEFT > COMFORT_RIGHT > Cancel"
                    );
                    // If comfort blinkers are active, the first opposing blink
                    // will cancel comfort blinkers, and not count towards the blink count.
                    context->lmState.blinkCount = 0;
                    HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_OFF);
                    break;
                default:
                    LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "LEFT > Unknown State");
                    break;
            }
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) != 0 &&
                   CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) == 0 &&
                   CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) == 0
        ) {
            LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "LEFT > Unrelated activity");
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) == 0 &&
                  CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) != 0 &&
                  CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) != 0
        ) {
            if (context->lmState.blinkStatus == HANDLER_LM_BLINK_LEFT) {
                LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "RIGHT > Quick Switch > Reset");
                context->lmState.blinkCount = 0;
            }
            context->lmState.blinkCount++;
            context->lmState.blinkStatus = HANDLER_LM_BLINK_RIGHT;
            switch (context->lmState.comfortBlinkerStatus) {
                case HANDLER_LM_COMF_BLINK_OFF:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "RIGHT > COMFORT_INACTIVE > %d/%d",
                        context->lmState.blinkCount,
                        configBlinkLimit
                    );
                    break;
                case HANDLER_LM_COMF_BLINK_RIGHT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "RIGHT > COMFORT_RIGHT > %d/%d",
                        context->lmState.blinkCount,
                        configBlinkLimit
                    );
                    if (context->lmState.blinkCount >= configBlinkLimit) {
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "RIGHT > COMFORT_RIGHT > Blink limit"
                        );
                        HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_OFF);
                    }
                    break;
                case HANDLER_LM_COMF_BLINK_LEFT:
                    LogDebug(
                        CONFIG_DEVICE_LOG_SYSTEM,
                        "RIGHT > COMFORT_LEFT > Cancel"
                    );
                    context->lmState.blinkCount = 0;
                    HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_OFF);
                    break;
                default:
                    LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "RIGHT > Unknown State");
                    break;
            }
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) == 0 &&
                   CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) != 0 &&
                   CHECK_BIT(lightStatus2, IBUS_LM_BLINK_SIG_BIT) == 0
        ) {
            LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "RIGHT > Unrelated activity");
        } else if (CHECK_BIT(lightStatus, IBUS_LM_LEFT_SIG_BIT) == 0 &&
                   CHECK_BIT(lightStatus, IBUS_LM_RIGHT_SIG_BIT) == 0
        ) {
            // OFF blinker (or anything non-blinker)
            // Only activate comfort blinkers after a single blink.
            if (context->lmState.blinkCount == 1) {
                LogDebug(
                    CONFIG_DEVICE_LOG_SYSTEM,
                    "OFF > Blinks: %d => Activate",
                    context->lmState.blinkCount
                );
                // I believe this is redundant, but better safe than sorry.
                switch (context->lmState.comfortBlinkerStatus) {
                    case HANDLER_LM_COMF_BLINK_RIGHT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %d > COMFORT_RIGHT > Cancel",
                            context->lmState.blinkCount
                        );
                        HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_OFF);
                        break;
                    case HANDLER_LM_COMF_BLINK_LEFT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %d > COMFORT_LEFT > Cancel",
                            context->lmState.blinkCount
                        );
                        HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_OFF);
                        break;
                }
                // Activate comfort
                switch (context->lmState.blinkStatus) {
                    case HANDLER_LM_BLINK_LEFT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %d > BLINK_LEFT => Activate",
                            context->lmState.blinkCount
                        );
                        HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_LEFT);
                        break;
                    case HANDLER_LM_BLINK_RIGHT:
                        LogDebug(
                            CONFIG_DEVICE_LOG_SYSTEM,
                            "OFF > Blinks: %d > BLINK_RIGHT => Activate",
                            context->lmState.blinkCount
                        );
                        HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_BLINK_RIGHT);
                        break;
                }
            } else if (context->lmState.blinkCount > 1) {
                LogDebug(
                    CONFIG_DEVICE_LOG_SYSTEM,
                    "OFF > Blinks: %d => Do not activate comfort",
                    context->lmState.blinkCount
                );
                context->lmState.blinkCount = 0;
            } else {
                // Sequential non-blinker lamp activity
                LogDebug(CONFIG_DEVICE_LOG_SYSTEM, "OFF > Unrelated activity!");
            }
            context->lmState.blinkStatus = HANDLER_LM_BLINK_OFF;
        }
    }
    // Engage ANGEL EYEZ
    if (parkingLamps == CONFIG_SETTING_ON) {
        unsigned char lightStatus = pkt[4];
        if (CHECK_BIT(lightStatus, IBUS_LM_PARKING_SIG_BIT) == 0) {
            HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_PARKING_ON);
        }
    } else {
        if (context->lmState.comfortParkingLampsStatus == HANDLER_LM_COMF_PARKING_ON) {
            HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_PARKING_OFF);
        }
    }
}

/**
 * HandlerIBusLCMDimmerStatus()
 *     Description:
 *         Track the Dimmer Status messages so we can correctly set the
 *         dimming state when issuing lighting diagnostics requests
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLMDimmerStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetLightingFeaturesActive() == CONFIG_SETTING_ON) {
        uint8_t checksum = IBusGetLMDimmerChecksum(pkt);
        if (checksum != context->lmDimmerChecksum) {
            IBusCommandDIAGetIOStatus(context->ibus, IBUS_DEVICE_LCM);
            context->lmDimmerChecksum = checksum;
        }
        context->lmLastIOStatus = TimerGetMillis();
    }
}

/**
 * HandlerIBusLMRedundantData()
 *     Description:
 *         Check the VIN to see if we're in a new vehicle
 *         Raw: D0 10 80 54 50 4E 66 05 80 06 10 42 38 07 00 06 05 81
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerIBusLMRedundantData(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char currentVehicleId[5] = {};
    ConfigGetVehicleIdentity(currentVehicleId);
    unsigned char vehicleId[] = {
        pkt[4],
        pkt[5],
        pkt[6],
        pkt[7],
        (pkt[8] >> 4) & 0xF,
    };
    // Check VIN
    if (memcmp(&vehicleId, &currentVehicleId, 5) != 0) {
        char vinTwo[] = {vehicleId[0], vehicleId[1], '\0'};
        LogWarning(
            "Detected VIN Change: %s%02x%02x%x",
            vinTwo,
            vehicleId[2],
            vehicleId[3],
            vehicleId[4]
        );
        // Request light module ident
        IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_LCM);
        // Save the new VIN
        ConfigSetVehicleIdentity(vehicleId);
        // Request the vehicle configuration
        IBusCommandIKEGetVehicleConfig(context->ibus);
        // Fallback to the CD53 UI as appropriate
        if (context->ibusModuleStatus.MID == 0 &&
            context->ibusModuleStatus.GT == 0 &&
            context->ibusModuleStatus.BMBT == 0 &&
            context->ibusModuleStatus.VM == 0 &&
            context->uiMode != CONFIG_UI_CD53
        ) {
            LogInfo(LOG_SOURCE_SYSTEM, "Fallback to CD53");
            HandlerIBusSwitchUI(context, CONFIG_UI_CD53);
        }
    } else if (ConfigGetLMVariant() == CONFIG_SETTING_OFF) {
        // Identify the LM if we do not have an ID for it
        IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_LCM);
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
    unsigned char mflButton = pkt[IBUS_PKT_DB1];
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
        if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_PRESS) {
            context->mflButtonStatus = HANDLER_MFL_STATUS_OFF;
        }
        if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_REL &&
            context->mflButtonStatus == HANDLER_MFL_STATUS_OFF
        ) {
            if (context->bt->callStatus == BT_CALL_ACTIVE) {
                BTCommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BT_CALL_INCOMING) {
                BTCommandCallAccept(context->bt);
            } else if (context->bt->callStatus == BT_CALL_OUTGOING) {
                BTCommandCallEnd(context->bt);
            } else if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
                if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                    BTCommandPause(context->bt);
                } else {
                    BTCommandPlay(context->bt);
                }
            }
        } else if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_HOLD) {
            context->mflButtonStatus = HANDLER_MFL_STATUS_SPEAK_HOLD;
            BTCommandToggleVoiceRecognition(context->bt);
        }
    } else {
        if (mflButton == IBUS_MFL_BTN_EVENT_VOICE_REL) {
            if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
                if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                    BTCommandPause(context->bt);
                } else {
                    BTCommandPlay(context->bt);
                }
            }
        }
    }
}

/**
 * HandlerIBusModuleStatusRequest()
 *     Description:
 *         Respond to module status requests for those modules which
 *         we are emulating
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusModuleStatusRequest(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_CDC) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_CDC,
            pkt[IBUS_PKT_SRC],
            0x00
        );
        context->cdChangerLastPoll = TimerGetMillis();
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL &&
               ConfigGetTelephonyFeaturesActive() == CONFIG_SETTING_ON
    ) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_TEL,
            pkt[IBUS_PKT_SRC],
            IBUS_TEL_SIG_EVEREST
        );
    }
}

/**
 * HandlerIBusPDCStatus()
 *     Description:
 *         Handle PDC Status Updates
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusPDCStatus(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    context->pdcLastStatus = TimerGetMillis();
    if (context->ibusModuleStatus.PDC == 0) {
        context->ibusModuleStatus.PDC = 1;
    }
    if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_ON &&
        context->volumeMode == HANDLER_VOLUME_MODE_NORMAL &&
        context->bt->activeDevice.a2dpId != 0
    ) {
        LogWarning(
            "PDC START - LOWER VOLUME -- Currently %d",
            context->bt->activeDevice.a2dpVolume
        );
        HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_DOWN);
    }
}


/**
 * HandlerIBusRADVolumeChange()
 *     Description:
 *         Adjust the volume for calls based on where the user is adjusting
 *         the audio volume.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusRADVolumeChange(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t direction = pkt[IBUS_PKT_DB1] & 0xF;
    // Only watch for changes when not on a call
    if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE) {
        uint8_t steps = pkt[IBUS_PKT_DB1] >> 4;
        unsigned char volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (direction == IBUS_RAD_VOLUME_DOWN) {
            while (steps > 0 && volume < HANDLER_TEL_VOL_OFFSET_MAX) {
                volume = volume + 1;
                steps--;
            }
        } else if (direction == IBUS_RAD_VOLUME_UP) {
            while (steps > 0 && volume > 0) {
                volume = volume - 1;
                steps--;
            }
        }
        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, volume);
    }
}

/**
 * HandlerIBusSensorValueUpdate()
 *     Description:
 *         Parse Sensor Status
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *type - The Update Type
 *     Returns:
 *         void
 */
void HandlerIBusSensorValueUpdate(void *ctx, unsigned char *type)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (*type == IBUS_SENSOR_VALUE_GEAR_POS) {
        context->gearLastStatus = TimerGetMillis();
    }
}

/**
 * HandlerIBusTELVolumeChange()
 *     Description:
 *         Adjust the volume for calls when asked to do so by the BMBT / MID.
 *         If the vehicle has a DSP or MID, then forward those changes to the DSP
 *         or RAD rather than adjusting the DAC volume.
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void HandlerIBusTELVolumeChange(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t direction = pkt[IBUS_PKT_DB1] & 0x0F;
    // Forward volume changes to the RAD / DSP when in Bluetooth mode OR
    // when in radio mode if the S/PDIF input is selected and the DSP is found
    if ((context->uiMode == CONFIG_UI_BMBT ||
         context->uiMode == CONFIG_UI_MID ||
         context->uiMode == CONFIG_UI_MID_BMBT)
        && (
            context->ibus->cdChangerFunction != IBUS_CDC_FUNC_NOT_PLAYING ||
            (
                ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC) == CONFIG_SETTING_DSP_INPUT_SPDIF &&
                context->ibusModuleStatus.DSP == 1
            )
        )
    )  {
        // Drop Telephony mode so the radio acknowledges the volume changes
        IBusCommandTELStatus(context->ibus, IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE);
        unsigned char sourceSystem = IBUS_DEVICE_BMBT;
        if (context->ibusModuleStatus.MID == 1) {
            sourceSystem = IBUS_DEVICE_MID;
        }
        IBusCommandSetVolume(
            context->ibus,
            sourceSystem,
            IBUS_DEVICE_RAD,
            pkt[4]
        );
        uint8_t steps = pkt[IBUS_PKT_DB1] >> 4;
        unsigned char volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (direction == IBUS_RAD_VOLUME_UP) {
            while (steps > 0 && volume < HANDLER_TEL_VOL_OFFSET_MAX) {
                volume = volume + 1;
                steps--;
            }
        } else if (direction == IBUS_RAD_VOLUME_DOWN) {
            while (steps > 0 && volume > 0) {
                volume = volume - 1;
                steps--;
            }
        }
        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, volume);
        // Re-enable telephony mode
        IBusCommandTELStatus(context->ibus, IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE);
        IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
    } else if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING) {
        unsigned char volumeConfig = CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL;
        unsigned char volume = ConfigGetSetting(volumeConfig);
        // PCM51XX volume gets lower as you raise the value in the register
        if (direction == IBUS_RAD_VOLUME_UP && volume > 0x00) {
            volume = volume - 1;
        } else if (IBUS_RAD_VOLUME_DOWN == 0 && volume < 0xFF) {
            volume = volume + 1;
        }
        PCM51XXSetVolume(volume);
        ConfigSetSetting(volumeConfig, volume);
    }
}

/**
 * HandlerIBusModuleStatusResponse()
 *     Description:
 *         Track module status as we get them & track UI changes
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *pkt - The packet
 *     Returns:
 *         void
 */
void HandlerIBusModuleStatusResponse(void *ctx, unsigned char *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    unsigned char module = pkt[IBUS_PKT_SRC];
    if (module == IBUS_DEVICE_DSP && context->ibusModuleStatus.DSP == 0) {
        context->ibusModuleStatus.DSP = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "DSP Detected");
    } else if (module == IBUS_DEVICE_BMBT ||
               module == IBUS_DEVICE_GT ||
               module == IBUS_DEVICE_VM
    ) {
        if (context->ibusModuleStatus.BMBT == 0) {
            context->ibusModuleStatus.BMBT = 1;
            LogInfo(LOG_SOURCE_SYSTEM, "BMBT Detected");
        }
        if (context->ibusModuleStatus.GT == 0) {
            context->ibusModuleStatus.GT = 1;
            LogInfo(LOG_SOURCE_SYSTEM, "GT Detected");
        }
        if (context->ibusModuleStatus.VM == 0) {
            context->ibusModuleStatus.VM = 1;
            LogInfo(LOG_SOURCE_SYSTEM, "VM Detected");
        }
        // The GT is slow to boot, so continue to query
        // for the ident until we get it
        unsigned char uiMode = ConfigGetUIMode();
        if (uiMode != CONFIG_UI_BMBT &&
            uiMode != CONFIG_UI_MID_BMBT &&
            uiMode != CONFIG_UI_BUSINESS_NAV
        ) {
            // Request the Navigation Identity
            IBusCommandDIAGetIdentity(context->ibus, IBUS_DEVICE_GT);
        }
    } else if (module == IBUS_DEVICE_LCM && context->ibusModuleStatus.LCM == 0) {
        LogInfo(LOG_SOURCE_SYSTEM, "LCM Detected");
        context->ibusModuleStatus.LCM = 1;
    } else if (module == IBUS_DEVICE_MID &&
        context->ibusModuleStatus.MID == 0
    ) {
        context->ibusModuleStatus.MID = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "MID Detected");
        unsigned char uiMode = ConfigGetUIMode();
        if (uiMode != CONFIG_UI_MID &&
            uiMode != CONFIG_UI_MID_BMBT
        ) {
            if (context->ibusModuleStatus.GT == 1) {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID / BMBT UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_MID_BMBT);
            } else {
                LogInfo(LOG_SOURCE_SYSTEM, "Detected MID UI");
                HandlerIBusSwitchUI(context, CONFIG_UI_MID);
            }
        }
    } else if (module == IBUS_DEVICE_PDC && context->ibusModuleStatus.PDC == 0) {
        context->ibusModuleStatus.PDC = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "PDC Detected");
    } else if (module == IBUS_DEVICE_RAD && context->ibusModuleStatus.RAD == 0) {
        context->ibusModuleStatus.RAD = 1;
        LogInfo(LOG_SOURCE_SYSTEM, "RAD Detected");
        // If the radio responds, announce that the CD Changer is present
        IBusCommandCDCAnnounce(context->ibus);
    }
}

/**
 * HandlerTimerIBusCDCAnnounce()
 *     Description:
 *         This periodic task tracks how long it has been since the radio
 *         sent us (the CDC) a "ping". We should re-announce ourselves if that
 *         value reaches the timeout specified and the ignition is on.
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerIBusCDCAnnounce(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    uint32_t timeDiff = now - context->cdChangerLastPoll;
    if (timeDiff >= HANDLER_CDC_ANOUNCE_TIMEOUT &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF &&
        HandlerIBusGetIsIgnitionStatusOn(context) == 1
    ) {
        IBusCommandSetModuleStatus(
            context->ibus,
            IBUS_DEVICE_CDC,
            IBUS_DEVICE_LOC,
            0x01
        );
        context->cdChangerLastPoll = now;
    }
}

/**
 * HandlerTimerIBusCDCSendStatus()
 *     Description:
 *         This periodic task will proactively send the CDC status to the BM5x
 *         radio if we don't see a status poll within the last 20000 milliseconds.
 *         The CDC poll happens every 19945 milliseconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerIBusCDCSendStatus(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastStatus) >= HANDLER_CDC_STATUS_TIMEOUT &&
        (context->uiMode == CONFIG_UI_BMBT || context->uiMode == CONFIG_UI_MID_BMBT) &&
        HandlerIBusGetIsIgnitionStatusOn(context) == 1
    ) {
        HandlerIBusBroadcastCDCStatus(context);
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: Send CDC status preemptively");
    }
}

/**
 * HandlerTimerIBusLCMIOStatus()
 *     Description:
 *         Request the LM I/O Status when the key is in position 1 or above
 *         every 30 seconds if we have not seen it
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerIBusLCMIOStatus(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetLightingFeaturesActive() == CONFIG_SETTING_ON) {
        uint32_t now = TimerGetMillis();
        uint32_t timeDiff = now - context->lmLastIOStatus;
        if (timeDiff >= 30000 && HandlerIBusGetIsIgnitionStatusOn(context) == 1) {
            IBusCommandDIAGetIOStatus(context->ibus, IBUS_DEVICE_LCM);
            context->lmLastIOStatus = now;
        }
    }
}

/**
 * HandlerTimerIBusLightingState()
 *     Description:
 *         Periodically update the LM I/O state to enable the lamps we want
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerIBusLightingState(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetLightingFeaturesActive() == CONFIG_SETTING_ON) {
        uint32_t now = TimerGetMillis();
        if (HandlerIBusGetIsIgnitionStatusOn(context) == 1 &&
            (now - context->lmLastStatusSet) >= 10000 &&
            (
                context->lmState.comfortBlinkerStatus != HANDLER_LM_COMF_BLINK_OFF ||
                context->lmState.comfortParkingLampsStatus != HANDLER_LM_COMF_PARKING_OFF
            )
        ) {
            HandlerIBusLMActivateBulbs(context, HANDLER_LM_EVENT_REFRESH);
        }
    }
}

/**
 * HandlerTimerIBusPings()
 *     Description:
 *         Request various pongs from different modules. Also send the TEL
 *         broadcast to the system and request the ignition status
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerIBusPings(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    switch(context->ibusModulePingState) {
        case HANDLER_IBUS_MODULE_PING_STATE_READY: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_IKE;
            if (context->ibusModuleStatus.IKE == 0) {
                IBusCommandGetModuleStatus(
                    context->ibus,
                    IBUS_DEVICE_RAD,
                    IBUS_DEVICE_IKE
                );
            } else {
                HandlerTimerIBusPings(ctx);
            }
            break;
        }
        case HANDLER_IBUS_MODULE_PING_STATE_IKE: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_GT;
            if (context->ibusModuleStatus.GT == 0) {
                IBusCommandGetModuleStatus(
                    context->ibus,
                    IBUS_DEVICE_RAD,
                    IBUS_DEVICE_GT
                );
            } else {
                HandlerTimerIBusPings(ctx);
            }
            break;
        }
        case HANDLER_IBUS_MODULE_PING_STATE_GT: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_MID;
            if (context->ibusModuleStatus.MID == 0) {
                IBusCommandGetModuleStatus(
                    context->ibus,
                    IBUS_DEVICE_RAD,
                    IBUS_DEVICE_MID
                );
            } else {
                HandlerTimerIBusPings(ctx);
            }
            break;
        }
        case HANDLER_IBUS_MODULE_PING_STATE_MID: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_RAD;
            if (context->ibusModuleStatus.RAD == 0) {
                IBusCommandGetModuleStatus(
                    context->ibus,
                    IBUS_DEVICE_CDC,
                    IBUS_DEVICE_RAD
                );
            } else {
                HandlerTimerIBusPings(ctx);
            }
            break;
        }
        case HANDLER_IBUS_MODULE_PING_STATE_RAD: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_LM;
            if (context->ibusModuleStatus.LCM == 0) {
                IBusCommandGetModuleStatus(
                    context->ibus,
                    IBUS_DEVICE_IKE,
                    IBUS_DEVICE_LCM
                );
            } else {
                HandlerTimerIBusPings(ctx);
            }
            break;
        }
        case HANDLER_IBUS_MODULE_PING_STATE_LM: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_TEL;
            if (ConfigGetTelephonyFeaturesActive() == CONFIG_SETTING_ON) {
                IBusCommandSetModuleStatus(
                    context->ibus,
                    IBUS_DEVICE_TEL,
                    IBUS_DEVICE_LOC,
                    IBUS_TEL_SIG_EVEREST | 0x01
                );
            } else {
                HandlerTimerIBusPings(ctx);
            }
            break;
        }
        case HANDLER_IBUS_MODULE_PING_STATE_TEL: {
            context->ibusModulePingState = HANDLER_IBUS_MODULE_PING_STATE_OFF;
            IBusCommandIKEGetIgnitionStatus(context->ibus);
            // Unregister this timer so we do not waste resources on it
            TimerUnregisterScheduledTask(&HandlerTimerIBusPings);
            break;
        }
    }
}
