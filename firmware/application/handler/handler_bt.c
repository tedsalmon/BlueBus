/*
 * File: handler_bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic around the BT events
 */
#include "handler_bt.h"
static char *PROFILES[] = {
    "A2DP",
    "AVRCP",
    0,
    "HFP",
    "BLE",
    0,
    "PBAP",
    0,
    "MAP"
};

void HandlerBTInit(HandlerContext_t *context)
{
    EventRegisterCallback(
        BT_EVENT_CALL_STATUS_UPDATE,
        &HandlerBTCallStatus,
        context
    );
    EventRegisterCallback(
        BT_EVENT_CALLER_ID_UPDATE,
        &HandlerBTCallerID,
        context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_FOUND,
        &HandlerBTDeviceFound,
        context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_CONNECTED,
        &HandlerBTDeviceLinkConnected,
        context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &HandlerBTDeviceDisconnected,
        context
    );
    EventRegisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &HandlerBTPlaybackStatus,
        context
    );
    if (context->bt->type == BT_BTM_TYPE_BC127) {
        EventRegisterCallback(
            BT_EVENT_BOOT,
            &HandlerBTBC127Boot,
            context
        );
        EventRegisterCallback(
            BT_EVENT_BOOT_STATUS,
            &HandlerBTBC127BootStatus,
            context
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127State,
            context,
            HANDLER_INT_BC127_STATE
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127DeviceConnection,
            context,
            HANDLER_INT_DEVICE_CONN
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127ScanDevices,
            context,
            HANDLER_INT_DEVICE_SCAN
        );
        TimerRegisterScheduledTask(
            &HandlerTimerBTBC127OpenProfileErrors,
            context,
            HANDLER_INT_PROFILE_ERROR
        );
        BC127CommandStatus(context->bt);
    } else {
        EventRegisterCallback(
            BT_EVENT_AVRCP_PDU_CHANGE,
            &HandlerBTBM83AVRCPUpdates,
            context
        );
        EventRegisterCallback(
            BT_EVENT_BOOT,
            &HandlerBTBM83Boot,
            context
        );
        EventRegisterCallback(
            BT_EVENT_BOOT_STATUS,
            &HandlerBTBM83BootStatus,
            context
        );
        EventRegisterCallback(
            BT_EVENT_LINK_BACK_STATUS,
            &HandlerBTBM83LinkBackStatus,
            context
        );
        context->avrcpRegisterStatusNotifierTimerId = TimerRegisterScheduledTask(
            &HandlerTimerBTBM83AVRCPManager,
            context,
            HANDLER_INT_BT_AVRCP_UPDATER
        );
        context->bm83PowerStateTimerId = TimerRegisterScheduledTask(
            &HandlerTimerBTBM83ManagePowerState,
            context,
            HANDLER_INT_BM83_POWER_RESET
        );
    }
    TimerRegisterScheduledTask(
        &HandlerTimerBTVolumeManagement,
        context,
        HANDLER_INT_VOL_MGMT
    );
    TimerRegisterScheduledTask(
        &HandlerTimerBTBM83ScanDevices,
        context,
        HANDLER_INT_DEVICE_SCAN
    );
}

/**
 * HandlerBTCallStatus()
 *     Description:
 *         Handle call status updates. This includes setting the car up
 *         for telephony mode and updating the vehicle volume.
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTCallStatus(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If we were playing before the call, try to resume playback
    if (context->bt->callStatus == BT_CALL_INACTIVE &&
        context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING
    ) {
        BC127CommandPlay(context->bt);
    }
    // Tell the vehicle what the call status is
    uint8_t statusChange = HandlerSetIBusTELStatus(
        context,
        HANDLER_TEL_STATUS_SET
    );
    if (statusChange == 1) {
        LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU");
        // Handle volume control
        if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING &&
            (
                ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC) == CONFIG_SETTING_DSP_INPUT_SPDIF ||
                context->ibusModuleStatus.DSP == 0
            )
        ) {
            if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > Begin");
                // Enable the amp and mute the radio
                PAM_SHDN = 1;
                UtilsSetPinMode(UTILS_PIN_TEL_MUTE, 1);
                // Set the DAC Volume to the "telephone" volume
                PCM51XXSetVolume(ConfigGetSetting(CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL));
            } else {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > TCU > End");
                // Reset the DAC volume
                PCM51XXSetVolume(ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL));
                // Disable the amp and unmute the radio
                PAM_SHDN = 0;
                TimerDelayMicroseconds(250);
                UtilsSetPinMode(UTILS_PIN_TEL_MUTE, 0);
            }
        } else {
            uint8_t volume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
            uint8_t dspMode = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
            if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
                if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING &&
                    dspMode == CONFIG_SETTING_DSP_INPUT_SPDIF &&
                    context->ibusModuleStatus.DSP == 1
                ) {
                    IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
                }
                LogDebug(LOG_SOURCE_SYSTEM, "Call > Begin");
                if (strlen(context->bt->callerId) > 0 &&
                    context->uiMode != CONFIG_UI_CD53
                ) {
                    IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
                }
                if (volume > HANDLER_TEL_VOL_OFFSET_MAX) {
                    volume = HANDLER_TEL_VOL_OFFSET_MAX;
                    ConfigSetSetting(CONFIG_SETTING_TEL_VOL, HANDLER_TEL_VOL_OFFSET_MAX);
                }
                LogDebug(LOG_SOURCE_SYSTEM, "Call > Volume: %d", volume);
                uint8_t sourceSystem = IBUS_DEVICE_BMBT;
                uint8_t volStepMax = 0x03;
                if (context->ibusModuleStatus.MID == 1) {
                    sourceSystem = IBUS_DEVICE_MID;
                }
                if (context->uiMode == CONFIG_UI_CD53) {
                    sourceSystem = IBUS_DEVICE_MFL;
                    volStepMax = 0x01;
                }
                while (volume > 0) {
                    uint8_t volStep = volume;
                    if (volStep > volStepMax) {
                        volStep = volStepMax;
                    }
                    IBusCommandSetVolume(
                        context->ibus,
                        sourceSystem,
                        IBUS_DEVICE_RAD,
                        (volStep << 4) | 0x01
                    );
                    volume = volume - volStep;
                }
            } else {
                LogDebug(LOG_SOURCE_SYSTEM, "Call > End");
                // Reset the volume
                // Temporarily set the call status flag to on so we do not alter
                // the volume we are lowering ourselves
                context->telStatus = HANDLER_TEL_STATUS_VOL_CHANGE;
                uint8_t sourceSystem = IBUS_DEVICE_BMBT;
                uint8_t volStepMax = 0x03;
                if (context->ibusModuleStatus.MID == 1) {
                    sourceSystem = IBUS_DEVICE_MID;
                }
                if (context->uiMode == CONFIG_UI_CD53) {
                    sourceSystem = IBUS_DEVICE_MFL;
                    volStepMax = 0x01;
                }
                while (volume > 0) {
                    uint8_t volStep = volume;
                    if (volStep > volStepMax) {
                        volStep = volStepMax;
                    }
                    IBusCommandSetVolume(
                        context->ibus,
                        sourceSystem,
                        IBUS_DEVICE_RAD,
                        volStep << 4
                    );
                    volume = volume - volStep;
                }
                context->telStatus = IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE;
                if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING &&
                    dspMode == CONFIG_SETTING_DSP_INPUT_SPDIF &&
                    context->ibusModuleStatus.DSP == 1
                ) {
                    IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
                }
            }
        }
    }
}

/**
 * HandlerBTDeviceFound()
 *     Description:
 *         If a device is found and we are not connected, connect to it
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTDeviceFound(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->status == BT_STATUS_DISCONNECTED &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        LogDebug(LOG_SOURCE_SYSTEM, "Handler: No Device -- Attempt connection");
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            memcpy(context->bt->activeDevice.macId, data, BT_MAC_ID_LEN);
            BC127CommandProfileOpen(context->bt, "A2DP");
        } else {
            if (context->bt->pairedDevicesCount > 0) {
                if (context->btSelectedDevice == HANDLER_BT_SELECTED_DEVICE_NONE ||
                    context->bt->pairedDevicesCount == 1
                ) {
                    BTPairedDevice_t *dev = &context->bt->pairedDevices[0];
                    BTCommandConnect(context->bt, dev);
                    context->btSelectedDevice = 0;
                } else {
                    if (context->btSelectedDevice + 1 < context->bt->pairedDevicesCount) {
                        context->btSelectedDevice++;
                    } else {
                        context->btSelectedDevice = 0;
                    }
                    BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btSelectedDevice];
                    BTCommandConnect(context->bt, dev);
                }
            }
        }
    } else {
        LogDebug(
            LOG_SOURCE_SYSTEM,
            "Handler: Not connecting to new device %d %d %d",
            context->bt->activeDevice.deviceId,
            context->bt->status,
            context->ibus->ignitionStatus
        );
    }
}


/**
 * HandlerBTCallStatus()
 *     Description:
 *         Handle caller ID updates
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTCallerID(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->telStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE) {
        LogDebug(LOG_SOURCE_SYSTEM, "Call > ID: %s", context->bt->callerId);
        IBusCommandTELStatusText(context->ibus, context->bt->callerId, 0);
    }
}

/**
 * HandlerBTDeviceLinkConnected()
 *     Description:
 *         If a device link is opened, disable connectability once all profiles
 *         are opened. Otherwise if the ignition is off, disconnect all devices
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *linkType - The connected link type
 *     Returns:
 *         void
 */
void HandlerBTDeviceLinkConnected(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        uint8_t linkType = *data;
        // Once A2DP and AVRCP are connected, we can disable connectability
        // If HFP is enabled, do not disable connectability until the
        // profile opens
        if (context->bt->activeDevice.a2dpId != 0) {
            // Raise the volume one step to trigger the absolute volume notification
            if (context->bt->type == BT_BTM_TYPE_BC127) {
                BC127CommandVolume(
                    context->bt,
                    context->bt->activeDevice.a2dpId,
                    "UP"
                );
            }
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF ||
                context->bt->activeDevice.hfpId != 0
            ) {
                if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON &&
                    context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING
                ) {
                    BTCommandPlay(context->bt);
                }
                if (linkType == BT_LINK_TYPE_HFP) {
                    if (context->bt->type == BT_BTM_TYPE_BM83) {
                        BM83CommandDisconnect(context->bt, BM83_CMD_DISCONNECT_PARAM_HF);
                    } else {
                        BC127CommandClose(context->bt, context->bt->activeDevice.hfpId);
                    }
                }
            } else if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON &&
                       context->bt->activeDevice.hfpId == 0 &&
                       context->bt->type == BT_BTM_TYPE_BC127
            ) {
                BC127CommandProfileOpen(
                    context->bt,
                    "HFP"
                );
            }
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_GREEN);
            }
        }
        if (linkType == BT_LINK_TYPE_HFP && context->bt->type == BT_BTM_TYPE_BC127) {
            // Set the device character set to UTF-8
            BC127CommandAT(context->bt, "CSCS", "\"UTF-8\"");
            // Explicitly enable Calling Line Identification (Caller ID)
            BC127CommandAT(context->bt, "CLIP", "1");
        }
        if (context->bt->type == BT_BTM_TYPE_BM83) {
            // Request Device Name if it is empty
            char tmp[BT_DEVICE_NAME_LEN] = {0};
            if (memcmp(tmp, context->bt->activeDevice.deviceName, BT_DEVICE_NAME_LEN) == 0) {
                ConfigSetSetting(
                    CONFIG_SETTING_LAST_CONNECTED_DEVICE,
                    context->btSelectedDevice
                );
                BM83CommandReadLinkedDeviceInformation(
                    context->bt,
                    BM83_LINKED_DEVICE_QUERY_NAME
                );
            }
        }
        // @TODO Handle cases where PBAP access is not given
        //if (context->bt->activeDevice.hfpId != 0 &&
        //    context->bt->activeDevice.pbapId == 0
        //) {
        //    BC127CommandProfileOpen(
        //        context->bt,
        //        "PBAP"
        //    );
        //}
    } else {
        BTCommandDisconnect(context->bt);
    }
}

/**
 * HandlerBTDeviceDisconnected()
 *     Description:
 *         If a device disconnects and our ignition is on,
 *         make the module connectable again.
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTDeviceDisconnected(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Reset the metadata so we do not display incorrect data
    BTClearMetadata(context->bt);
    if (context->bt->type == BT_BTM_TYPE_BC127) {
        BC127ClearPairingErrors(context->bt);
    }
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        if (context->btSelectedDevice != HANDLER_BT_SELECTED_DEVICE_NONE) {
            //BTPairedDevice_t *dev = &context->bt->pairedDevices[
            //    context->btSelectedDevice
            //];
            //BTCommandProfileOpen(context->bt, "A2DP");
        } else {
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED);
            }
            BTCommandList(context->bt);
        }
    }
}

/**
 * HandlerBTPlaybackStatus()
 *     Description:
 *         If the application is starting, request the BC127 AVRCP Metadata
 *         if it is playing. If the CD Change status is not set to "playing"
 *         then we pause playback.
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTPlaybackStatus(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // If this is the first status update
    if (context->btStartupIsRun == 0) {
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                // Request Metadata
                BTCommandGetMetadata(context->bt);
            }
        } else {
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
            );
            TimerResetScheduledTask(context->avrcpRegisterStatusNotifierTimerId);
        }
        context->btStartupIsRun = 1;
    }
    if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING &&
        context->ibus->cdChangerFunction == IBUS_CDC_FUNC_NOT_PLAYING
    ) {
        // We're playing but not in Bluetooth mode - stop playback
        BTCommandPause(context->bt);
    }
}

/* BC127 Specific Handlers */

/**
 * HandlerBTBC127Boot()
 *     Description:
 *         If the BC127 module restarts, reset our internal state
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTBC127Boot(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
    BC127CommandStatus(context->bt);
}

/**
 * HandlerBTBC127BootStatus()
 *     Description:
 *         If the BC127 Radios are off, meaning we rebooted and got the status
 *         back, then alter the module status to match the ignition status
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTBC127BootStatus(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    BC127CommandList(context->bt);
    if (context->ibus->ignitionStatus == IBUS_IGNITION_OFF) {
        // Set the BT module not connectable or discoverable and disconnect all devices
        BC127CommandBtState(context->bt, BT_STATE_OFF, BT_STATE_OFF);
        BC127CommandClose(context->bt, BT_CLOSE_ALL);
    } else {
        // Set the connectable and discoverable states to what they were
        BC127CommandBtState(context->bt, BT_STATE_ON, context->bt->discoverable);
    }
}

/* BM83 Specific Handlers */

/**
 * HandlerBTBM83AVRCPUpdates()
 *     Description:
 *         Handle AVRCP updates
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *data - Any event data
 *     Returns:
 *         void
 */
void HandlerBTBM83AVRCPUpdates(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t type = data[0];
    uint8_t status = data[1];
    if ((type == BM83_AVRCP_EVT_PLAYBACK_STATUS_CHANGED &&
        status == BM83_AVRCP_DATA_PLAYBACK_STATUS_PLAYING) ||
        type == BM83_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED
    ) {
        context->bt->avrcpUpdates = SET_BIT(
            context->bt->avrcpUpdates,
            BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
        );
        if (status == BM83_AVRCP_DATA_PLAYBACK_STATUS_PLAYING) {
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
        }
        TimerResetScheduledTask(context->avrcpRegisterStatusNotifierTimerId);
    } else if (type == BM83_AVRCP_EVT_PLAYBACK_TRACK_CHANGED) {
        // On AVRCP "Change"
        if (status == 0x00) {
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
            );
            context->bt->avrcpUpdates = SET_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
            TimerResetScheduledTask(context->avrcpRegisterStatusNotifierTimerId);
        }
    }
}

/**
 * HandlerBTBM83LinkBackStatus()
 *     Description:
 *         React to Link back Status updates. When the ACL connection
 *         is reported as failed, increment the selected device.
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *pkt - Any event data
 *     Returns:
 *         void
 */
void HandlerBTBM83LinkBackStatus(void *ctx, uint8_t *pkt)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibus->ignitionStatus != IBUS_IGNITION_OFF) {
        // Ensure the proper link status
    }
}

/**
 * HandlerBTBM83Boot()
 *     Description:
 *         When the BM83 reports that it has booted, power it on
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerBTBM83Boot(void *ctx, uint8_t *tmp)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    // Power the module on
    BM83CommandPowerOn(context->bt);
    TimerUnregisterScheduledTaskById(context->bm83PowerStateTimerId);
}

/**
 * HandlerBTBM83BootStatus()
 *     Description:
 *         When the BM83 reports power on, request the PDL
 *     Params:
 *         void *ctx - The context provided at registration
 *         uint8_t *data - Any event data
 *     Returns:
 *         void
 */
void HandlerBTBM83BootStatus(void *ctx, uint8_t *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    uint8_t type = *data;
    if (type == BM83_DATA_BOOT_STATUS_POWER_ON) {
        BM83CommandReadPairedDevices(context->bt);
    }
}

/* Timers */
/**
 * HandlerTimerVolumeManagement()
 *     Description:
 *         Manage the A2DP volume per user settings
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTVolumeManagement(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetSetting(CONFIG_SETTING_MANAGE_VOLUME) == CONFIG_SETTING_ON &&
        context->volumeMode != HANDLER_VOLUME_MODE_LOWERED &&
        context->bt->activeDevice.a2dpId != 0 &&
        context->bt->type != BT_BTM_TYPE_BM83
    ) {
        if (context->bt->activeDevice.a2dpVolume < 127) {
            LogWarning("SET MAX VOLUME -- Currently %d", context->bt->activeDevice.a2dpVolume);
            BC127CommandVolume(context->bt, context->bt->activeDevice.a2dpId, "F");
            context->bt->activeDevice.a2dpVolume = 127;
        }
    }
    uint8_t lowerVolumeOnReverse = ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV);
    uint32_t now = TimerGetMillis();
    // Lower volume when PDC is active
    if (lowerVolumeOnReverse == CONFIG_SETTING_ON &&
        context->ibusModuleStatus.PDC == 1 &&
        context->bt->activeDevice.a2dpId != 0
    ) {
        uint32_t timeSinceUpdate = now - context->pdcLastStatus;
        if (context->volumeMode == HANDLER_WAIT_REV_VOL &&
            timeSinceUpdate >= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "PDC DONE - RAISE VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_UP);
        }
        if (context->volumeMode == HANDLER_VOLUME_MODE_NORMAL &&
            timeSinceUpdate <= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "PDC START - LOWER VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_DOWN);
        }
    }
    // Lower volume when the transmission is in reverse
    if (lowerVolumeOnReverse == CONFIG_SETTING_ON &&
        context->bt->activeDevice.a2dpId != 0
    ) {
        uint32_t timeSinceUpdate = now - context->gearLastStatus;
        if (context->volumeMode == HANDLER_VOLUME_MODE_LOWERED &&
            context->ibus->gearPosition != IBUS_IKE_GEAR_REVERSE &&
            timeSinceUpdate >= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "TRANS OUT OF REV - RAISE VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_UP);
        }
        if (context->volumeMode == HANDLER_VOLUME_MODE_NORMAL &&
            context->ibus->gearPosition == IBUS_IKE_GEAR_REVERSE &&
            timeSinceUpdate >= HANDLER_WAIT_REV_VOL
        ) {
            LogWarning(
                "TRANS IN REV - LOWER VOLUME -- Currently %d",
                context->bt->activeDevice.a2dpVolume
            );
            HandlerSetVolume(context, HANDLER_VOLUME_DIRECTION_DOWN);
        }
    }
}

/* BC127 Specific Timers */

/**
 * HandlerTimerBTBC127State()
 *     Description:
 *         Ensure the BC127 has booted, and if not, blink the red TEL LED
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTBC127State(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->powerState == BT_STATE_OFF &&
        context->btBootState == HANDLER_BT_BOOT_OK
    ) {
        LogWarning("BC127 Boot Failure");
        uint16_t bootFailCount = ConfigGetBC127BootFailures();
        bootFailCount++;
        ConfigSetBC127BootFailures(bootFailCount);
        IBusCommandTELSetLED(context->ibus, IBUS_TEL_LED_STATUS_RED_BLINKING);
        context->btBootState = HANDLER_BT_BOOT_FAIL;
    }
}

/**
 * HandlerTimerBTBC127DeviceConnection()
 *     Description:
 *         Monitor the BT connection and ensure it stays connected
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTBC127DeviceConnection(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (BTHasActiveMacId(context->bt) != 0 && context->bt->activeDevice.a2dpId == 0) {
        if (context->btDeviceConnRetries <= HANDLER_DEVICE_MAX_RECONN) {
            LogDebug(
                LOG_SOURCE_SYSTEM,
                "Handler: A2DP link closed -- Attempting to connect"
            );
            BC127CommandProfileOpen(
                context->bt,
                "A2DP"
            );
            context->btDeviceConnRetries += 1;
        } else {
            LogError("Handler: Giving up on BT connection");
            context->btDeviceConnRetries = 0;
            BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
            BTCommandDisconnect(context->bt);
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
void HandlerTimerBTBC127OpenProfileErrors(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (BTHasActiveMacId(context->bt) != 0) {
        uint8_t idx;
        for (idx = 0; idx < BC127_PROFILE_COUNT; idx++) {
            if (context->bt->pairingErrors[idx] == 1 && PROFILES[idx] != 0) {
                LogDebug(LOG_SOURCE_SYSTEM, "Handler: Attempting to resolve pairing error");
                BC127CommandProfileOpen(
                    context->bt,
                    PROFILES[idx]
                );
                context->bt->pairingErrors[idx] = 0;
            }
        }
    }
}

/**
 * HandlerTimerBTBC127ScanDevices()
 *     Description:
 *         Rescan for devices on the PDL periodically. Scan every 5 seconds if
 *         there is no connected device, otherwise every 60 seconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTBC127ScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (((context->bt->activeDevice.deviceId == 0 &&
        context->bt->status == BT_STATUS_DISCONNECTED) ||
        context->scanIntervals == 12) &&
        context->ibus->ignitionStatus > IBUS_IGNITION_OFF
    ) {
        context->scanIntervals = 0;
        BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_INACTIVE);
        BC127CommandList(context->bt);
    } else {
        context->scanIntervals += 1;
    }
}

/**
 * HandlerTimerBTBC127Metadata()
 *     Description:
 *         Request current playing song periodically
 *     Params:
 *         HandlerContext_t *context - The handler context
 *     Returns:
 *         void
 */
void HandlerTimerBTBC127Metadata(HandlerContext_t *context)
{
    uint32_t now = TimerGetMillis();
    if (now - HANDLER_BT_METADATA_TIMEOUT >= context->bt->metadataTimestamp &&
        context->bt->activeDevice.avrcpId != 0 &&
        context->bt->callStatus == BT_CALL_INACTIVE &&
        context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING
    ) {
        BC127CommandGetMetadata(context->bt);
    }
}

/* BM83 Specific Timers */

/**
 * HandlerTimerBTBM83AVRCPManager()
 *     Description:
 *         Register the track change event notifier or grab the metadata
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTBM83AVRCPManager(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->avrcpUpdates != 0x00) {
        if (CHECK_BIT(context->bt->avrcpUpdates, BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF) > 0) {
            context->bt->avrcpUpdates = CLEAR_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF
            );
            BM83CommandAVRCPRegisterNotification(
                context->bt,
                BM83_AVRCP_EVT_PLAYBACK_TRACK_CHANGED
            );
        } else if (CHECK_BIT(context->bt->avrcpUpdates, BT_AVRCP_ACTION_GET_METADATA) > 0) {
            context->bt->avrcpUpdates = CLEAR_BIT(
                context->bt->avrcpUpdates,
                BT_AVRCP_ACTION_GET_METADATA
            );
            BM83CommandAVRCPGetElementAttributesAll(context->bt);
        }
        // Set the timeout to 250ms from now if we still need to get metadata
        if (CHECK_BIT(context->bt->avrcpUpdates, BT_AVRCP_ACTION_GET_METADATA) > 0) {
            TimerSetTaskInterval(
                context->avrcpRegisterStatusNotifierTimerId,
                HANDLER_INT_BT_AVRCP_UPDATER_METADATA
            );
        } else {
            TimerSetTaskInterval(
                context->avrcpRegisterStatusNotifierTimerId,
                HANDLER_INT_BT_AVRCP_UPDATER
            );
        }
    }
}

/**
 * HandlerTimerBTBM83ManagePowerState()
 *     Description:
 *         Ensure the BM83 is powered on
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTBM83ManagePowerState(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->bt->powerState == BT_STATE_OFF) {
        switch (context->btBootState) {
            case HANDLER_BT_BOOT_RESET: {
                BT_RST = 1;
                BT_MFB = 1;
                BT_RST_MODE = 1;
                context->btBootState = HANDLER_BT_BOOT_MFB_H;
                TimerSetTaskInterval(context->bm83PowerStateTimerId, HANDLER_INT_BM83_POWER_MFB_ON);
                break;
            }
            case HANDLER_BT_BOOT_MFB_H: {
                BT_MFB = 0;
                context->btBootState = HANDLER_BT_BOOT_MFB_L;
                TimerSetTaskInterval(context->bm83PowerStateTimerId, HANDLER_INT_BM83_POWER_MFB_OFF);
                break;
            }
            case HANDLER_BT_BOOT_MFB_L: {
                // Failed to boot
                BT_RST_MODE = 0;
                BT_RST = 0;
                BT_MFB = 1;
                context->btBootState = HANDLER_BT_BOOT_RESET;
                TimerSetTaskInterval(context->bm83PowerStateTimerId, HANDLER_INT_BM83_POWER_RESET);
                break;
            }
        }
    }
}

/**
 * HandlerTimerBTBC127ScanDevices()
 *     Description:
 *         Rescan for devices on the PDL periodically. Scan every 5 seconds if
 *         there is no connected device, otherwise every 60 seconds
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerBTBM83ScanDevices(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (context->ibus->ignitionStatus > IBUS_IGNITION_OFF) {
        if (context->bt->pairedDevicesCount == 0 &&
            context->bt->powerState == BT_STATE_STANDBY
        ) {
            BM83CommandReadPairedDevices(context->bt);
        }
        if (context->bt->status == BT_STATUS_DISCONNECTED &&
            context->bt->discoverable == BT_STATE_OFF &&
            context->bt->pairedDevicesCount > 0
        ) {
            if (context->btSelectedDevice == HANDLER_BT_SELECTED_DEVICE_NONE ||
                context->bt->pairedDevicesCount == 1
            ) {
                BTPairedDevice_t *dev = &context->bt->pairedDevices[0];
                BTCommandConnect(context->bt, dev);
                context->btSelectedDevice = 0;
            } else {
                if (context->btSelectedDevice + 1 < context->bt->pairedDevicesCount) {
                    context->btSelectedDevice++;
                } else {
                    context->btSelectedDevice = 0;
                }
                BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btSelectedDevice];
                BTCommandConnect(context->bt, dev);
            }
        }
    }
}