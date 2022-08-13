/*
 * File: cd53.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the CD53 UI Mode handler
 */
#include "cd53.h"
static CD53Context_t Context;

uint8_t SETTINGS_MENU[] = {
    CD53_SETTING_IDX_HFP,
    CD53_SETTING_IDX_METADATA_MODE,
    CD53_SETTING_IDX_AUTOPLAY,
    CD53_SETTING_IDX_LOWER_VOL_REV,
    CD53_SETTING_IDX_VEH_TYPE,
    CD53_SETTING_IDX_BLINKERS,
    CD53_SETTING_IDX_COMFORT_LOCKS,
    CD53_SETTING_IDX_COMFORT_UNLOCK,
    CD53_SETTING_IDX_TEL_VOL_OFFSET,
    CD53_SETTING_IDX_PAIRINGS
};

uint8_t SETTINGS_TO_MENU[] = {
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY,
    CONFIG_SETTING_VOLUME_LOWER_ON_REV,
    CONFIG_VEHICLE_TYPE_ADDRESS,
    CONFIG_SETTING_COMFORT_BLINKERS,
    0x00,
    0x00,
    CONFIG_SETTING_TEL_VOL
};

void CD53Init(BT_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.mode = CD53_MODE_OFF;
    Context.mainDisplay = UtilsDisplayValueInit("Bluetooth", CD53_DISPLAY_STATUS_OFF);
    Context.tempDisplay = UtilsDisplayValueInit("", CD53_DISPLAY_STATUS_OFF);
    Context.btDeviceIndex = CD53_PAIRING_DEVICE_NONE;
    Context.displayMetadata = CD53_DISPLAY_METADATA_ON;
    Context.settingIdx = CD53_SETTING_IDX_HFP;
    Context.settingValue = CONFIG_SETTING_OFF;
    Context.settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
    Context.radioType = ConfigGetUIMode();
    Context.mediaChangeState = CD53_MEDIA_STATE_OK;
    EventRegisterCallback(
        BT_EVENT_CALLER_ID_UPDATE,
        &CD53BTCallerID,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_CALL_STATUS_UPDATE,
        &CD53BTCallStatus,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_BOOT,
        &CD53BTDeviceReady,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &CD53BTDeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_METADATA_UPDATE,
        &CD53BTMetadata,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &CD53BTPlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_BMBTButton,
        &CD53IBusBMBTButtonPress,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &CD53IBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_MFLButton,
        &CD53IBusMFLButton,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_RAD_WRITE_DISPLAY,
        &CD53IBusRADWriteDisplay,
        &Context
    );
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &CD53TimerDisplay,
        &Context,
        CD53_DISPLAY_TIMER_INT
    );
}

/**
 * CD53Destroy()
 *     Description:
 *         Unregister all event handlers, scheduled tasks and clear the context
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void CD53Destroy()
{
    EventUnregisterCallback(
        BT_EVENT_BOOT,
        &CD53BTDeviceReady
    );
    EventUnregisterCallback(
        BT_EVENT_CALLER_ID_UPDATE,
        &CD53BTCallerID
    );
    EventUnregisterCallback(
        BT_EVENT_CALL_STATUS_UPDATE,
        &CD53BTCallStatus
    );
    EventUnregisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &CD53BTDeviceDisconnected
    );
    EventUnregisterCallback(
        BT_EVENT_METADATA_UPDATE,
        &CD53BTMetadata
    );
    EventUnregisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &CD53BTPlaybackStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_BMBTButton,
        &CD53IBusBMBTButtonPress
    );
    EventUnregisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &CD53IBusCDChangerStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_MFLButton,
        &CD53IBusMFLButton
    );
    EventUnregisterCallback(
        IBUS_EVENT_RAD_WRITE_DISPLAY,
        &CD53IBusRADWriteDisplay
    );
    TimerUnregisterScheduledTask(&CD53TimerDisplay);
    memset(&Context, 0, sizeof(CD53Context_t));
}

static void CD53SetMainDisplayText(
    CD53Context_t *context,
    const char *str,
    int8_t timeout
) {
    memset(context->mainDisplay.text, 0, UTILS_DISPLAY_TEXT_SIZE);
    strncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE);
    // If the source is longer than the destination, we would not null terminate
    context->mainDisplay.text[UTILS_DISPLAY_TEXT_SIZE - 1] = '\0';
    context->mainDisplay.length = strlen(context->mainDisplay.text);
    context->mainDisplay.index = 0;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
    context->mainDisplay.timeout = timeout;
}

static void CD53SetTempDisplayText(
    CD53Context_t *context,
    char *str,
    int8_t timeout
) {
    memset(context->tempDisplay.text, 0, UTILS_DISPLAY_TEXT_SIZE);
    strncpy(context->tempDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE);
    // If the source is longer than the destination, we would not null terminate
    context->tempDisplay.text[UTILS_DISPLAY_TEXT_SIZE - 1] = '\0';
    context->tempDisplay.length = strlen(context->tempDisplay.text);
    context->tempDisplay.index = 0;
    context->tempDisplay.status = CD53_DISPLAY_STATUS_NEW;
    // Unlike the main display, we need to set the timeout beforehand, that way
    // the timer knows how many iterations to display the text for.
    context->tempDisplay.timeout = timeout;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

static void CD53RedisplayText(CD53Context_t *context)
{
    context->mainDisplay.index = 0;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

static void CD53ShowNextAvailableDevice(CD53Context_t *context, uint8_t direction)
{
    if (direction == 0x00) {
        if (context->btDeviceIndex < context->bt->pairedDevicesCount - 1) {
            context->btDeviceIndex++;
        } else {
            context->btDeviceIndex = 0;
        }
    } else {
        if (context->btDeviceIndex == 0) {
            context->btDeviceIndex = context->bt->pairedDevicesCount - 1;
        } else {
            context->btDeviceIndex--;
        }
    }
    BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
    char text[CD53_DISPLAY_TEXT_LEN + 1] = {0};
    strncpy(text, dev->deviceName, CD53_DISPLAY_TEXT_LEN);
    // Add a space and asterisks to the end of the device name
    // if it's the currently selected device
    if (memcmp(dev->macId, context->bt->activeDevice.macId, BT_LEN_MAC_ID) == 0) {
        uint8_t startIdx = strlen(text);
        if (startIdx > 9) {
            startIdx = 9;
        }
        text[startIdx++] = 0x20;
        text[startIdx++] = 0x2A;
    }
    CD53SetMainDisplayText(context, text, 0);
}

static void CD53MenuSettingsShow(CD53Context_t *context, unsigned char nextOption)
{
    if (nextOption == CD53_SETTING_IDX_HFP) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
            CD53SetMainDisplayText(context, "Handsfree: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            CD53SetMainDisplayText(context, "Handsfree: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = CD53_SETTING_IDX_HFP;
    }
    if (nextOption == CD53_SETTING_IDX_METADATA_MODE) {
        unsigned char value = ConfigGetSetting(
            CONFIG_SETTING_METADATA_MODE
        );
        if (value == CD53_METADATA_MODE_PARTY) {
            CD53SetMainDisplayText(context, "Metadata: Party", 0);
        } else if (value == CD53_METADATA_MODE_CHUNK) {
            CD53SetMainDisplayText(context, "Metadata: Chunk", 0);
        } else {
            CD53SetMainDisplayText(context, "Metadata: Party", 0);
            value = CD53_METADATA_MODE_PARTY;
        }
        context->settingIdx = CD53_SETTING_IDX_METADATA_MODE;
        context->settingValue = value;
    }
    if (nextOption == CD53_SETTING_IDX_AUTOPLAY) {
        if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_OFF) {
            CD53SetMainDisplayText(context, "Autoplay: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            CD53SetMainDisplayText(context, "Autoplay: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = CD53_SETTING_IDX_AUTOPLAY;
    }
    if (nextOption == CD53_SETTING_IDX_LOWER_VOL_REV) {
        if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_OFF) {
            CD53SetMainDisplayText(context, "Lower Volume On Reverse: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            CD53SetMainDisplayText(context, "Lower Vol. On Reverse: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = CD53_SETTING_IDX_LOWER_VOL_REV;
    }
    if (nextOption == CD53_SETTING_IDX_VEH_TYPE) {
        unsigned char vehicleType = ConfigGetVehicleType();
        context->settingValue = vehicleType;
        if (vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
            CD53SetMainDisplayText(context, "Car: E38/E39/E53", 0);
        } else if (vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
            CD53SetMainDisplayText(context, "Car: E46/Z4", 0);
        } else {
            CD53SetMainDisplayText(context, "Car: Unset", 0);
        }
        context->settingIdx = CD53_SETTING_IDX_VEH_TYPE;
    }
    if (nextOption == CD53_SETTING_IDX_BLINKERS) {
        unsigned char blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
        if (blinkCount > 8 || blinkCount == 0) {
            blinkCount = 1;
        }
        char blinkerText[19] = {0};
        snprintf(blinkerText, 19, "Comfort Blinks: %d", blinkCount);
        CD53SetMainDisplayText(context, blinkerText, 0);
        context->settingIdx = CD53_SETTING_IDX_BLINKERS;
        context->settingValue = blinkCount;
    }
    if (nextOption == CD53_SETTING_IDX_COMFORT_LOCKS) {
        unsigned char comfortLock = ConfigGetComfortLock();
        if (comfortLock == CONFIG_SETTING_OFF ||
            comfortLock > CONFIG_SETTING_COMFORT_LOCK_20KM
        ) {
            CD53SetMainDisplayText(context, "Comfort Locks: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else if (comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            CD53SetMainDisplayText(context, "Comfort Locks: 10km/h", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_10KM;
        } else {
            CD53SetMainDisplayText(context, "Comfort Locks: 20km/h", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_20KM;
        }
        context->settingIdx = CD53_SETTING_IDX_COMFORT_LOCKS;
    }
    if (nextOption == CD53_SETTING_IDX_COMFORT_UNLOCK) {
        unsigned char comfortUnlock = ConfigGetComfortUnlock();
        if (comfortUnlock == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            CD53SetMainDisplayText(context, "Comfort Unlock: Pos 1", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_1;
        } else if (comfortUnlock == CONFIG_SETTING_COMFORT_UNLOCK_POS_0) {
            CD53SetMainDisplayText(context, "Comfort Unlock: Pos 0", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_0;
        } else {
            CD53SetMainDisplayText(context, "Comfort Unlock: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
        context->settingIdx = CD53_SETTING_IDX_COMFORT_UNLOCK;
    }
    if (nextOption == CD53_SETTING_IDX_TEL_VOL_OFFSET) {
        unsigned char telephoneVolume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (telephoneVolume > 0xF || telephoneVolume == 0) {
            telephoneVolume = 1;
        }
        char telephoneVolumeText[21] = {0};
        snprintf(telephoneVolumeText, 21, "Call Vol. Offset: %d", telephoneVolume);
        CD53SetMainDisplayText(context, telephoneVolumeText, 0);
        context->settingIdx = CD53_SETTING_IDX_TEL_VOL_OFFSET;
        context->settingValue = telephoneVolume;
    }
    if (nextOption == CD53_SETTING_IDX_PAIRINGS) {
        CD53SetMainDisplayText(context, "Clear Pairings", 0);
        context->settingIdx = CD53_SETTING_IDX_PAIRINGS;
        context->settingValue = CONFIG_SETTING_OFF;
    }
}

static void CD53HandleUIButtonsNextPrev(CD53Context_t *context, unsigned char direction)
{
    if (context->mode == CD53_MODE_ACTIVE) {
        if (direction == 0x00) {
            BTCommandPlaybackTrackNext(context->bt);
        } else {
            BTCommandPlaybackTrackPrevious(context->bt);
        }
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
        context->mediaChangeState = CD53_MEDIA_STATE_CHANGE;
    } else if (context->mode == CD53_MODE_DEVICE_SEL) {
        CD53ShowNextAvailableDevice(context, direction);
    } else if (context->mode == CD53_MODE_SETTINGS &&
               context->settingMode == CD53_SETTING_MODE_SCROLL_SETTINGS
    ) {
        uint8_t nextOption = 0;
        if (context->settingIdx == CD53_SETTING_IDX_HFP && direction != 0x00) {
            nextOption = SETTINGS_MENU[CD53_SETTING_IDX_PAIRINGS];
        } else if(context->settingIdx == CD53_SETTING_IDX_PAIRINGS && direction == 0x00) {
            nextOption = SETTINGS_MENU[CD53_SETTING_IDX_HFP];
        } else {
            if (direction == 0x00) {
                nextOption = SETTINGS_MENU[context->settingIdx + 1];
            } else {
                nextOption = SETTINGS_MENU[context->settingIdx - 1];
            }
        }
        CD53MenuSettingsShow(context, nextOption);
    } else if(context->mode == CD53_MODE_SETTINGS &&
              context->settingMode == CD53_SETTING_MODE_SCROLL_VALUES
    ) {
        // Select different configuration options
        if (context->settingIdx == CD53_SETTING_IDX_HFP) {
            if (context->settingValue == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "On", 0);
                context->settingValue = CONFIG_SETTING_ON;
            } else {
                CD53SetMainDisplayText(context, "Off", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_METADATA_MODE) {
            if (context->settingValue == CD53_METADATA_MODE_CHUNK) {
                CD53SetMainDisplayText(context, "Party", 0);
                context->settingValue = CD53_METADATA_MODE_PARTY;
            } else if (context->settingValue == CD53_METADATA_MODE_PARTY) {
                CD53SetMainDisplayText(context, "Chunk", 0);
                context->settingValue = CD53_METADATA_MODE_CHUNK;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_AUTOPLAY) {
            if (context->settingValue == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "On", 0);
                context->settingValue = CONFIG_SETTING_ON;
            } else {
                CD53SetMainDisplayText(context, "Off", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_LOWER_VOL_REV) {
            if (context->settingValue == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "On", 0);
                context->settingValue = CONFIG_SETTING_ON;
            } else {
                CD53SetMainDisplayText(context, "Off", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_VEH_TYPE) {
            if (context->settingValue == 0x00 ||
                context->settingValue == 0xFF ||
                context->settingValue == IBUS_VEHICLE_TYPE_E46_Z4
            ) {
                CD53SetMainDisplayText(context, "E38/E39/E53", 0);
                context->settingValue = IBUS_VEHICLE_TYPE_E38_E39_E53;
            } else {
                CD53SetMainDisplayText(context, "E46/Z4", 0);
                context->settingValue = IBUS_VEHICLE_TYPE_E46_Z4;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_BLINKERS) {
            context->settingValue++;
            if (context->settingValue > 8) {
                context->settingValue = 1;
            }
            char blinkerText[2] = {0};
            snprintf(blinkerText, 2, "%d", context->settingValue);
            CD53SetMainDisplayText(context, blinkerText, 0);
        }
        if (context->settingIdx == CD53_SETTING_IDX_COMFORT_LOCKS) {
            unsigned char comfortLock = ConfigGetComfortLock();
            if (comfortLock == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "10km/h", 0);
                context->settingValue = CONFIG_SETTING_COMFORT_LOCK_10KM;
            } else if (comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM) {
                CD53SetMainDisplayText(context, "20km/h", 0);
                context->settingValue = CONFIG_SETTING_COMFORT_LOCK_20KM;
            } else if (comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM) {
                CD53SetMainDisplayText(context, "Off", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_COMFORT_UNLOCK) {
            unsigned char comfortUnlock = ConfigGetComfortUnlock();
            if (comfortUnlock == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "Pos 1", 0);
                context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_1;
            } else if (comfortUnlock == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
                CD53SetMainDisplayText(context, "Pos 0", 0);
                context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_0;
            } else {
                CD53SetMainDisplayText(context, "Off", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            }
        }
        if (context->settingIdx == CD53_SETTING_IDX_TEL_VOL_OFFSET) {
            context->settingValue++;
            if (context->settingValue > 0xF) {
                context->settingValue = 1;
            }
            char telephoneVolumeText[3] = {0};
            snprintf(telephoneVolumeText, 3, "%d", context->settingValue);
            CD53SetMainDisplayText(context, telephoneVolumeText, 0);
        }
        if (context->settingIdx == CD53_SETTING_IDX_PAIRINGS) {
            if (context->settingValue == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "Press 2", 0);
                context->settingValue = CONFIG_SETTING_ON;
            } else {
                CD53SetMainDisplayText(context, "Clear Pairings", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            }
        }
    }
}

static void CD53HandleUIButtons(CD53Context_t *context, unsigned char *pkt)
{
    unsigned char requestedCommand = pkt[IBUS_PKT_DB1];
    if (requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK ||
        requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK_BLAUPUNKT
    ) {
        CD53HandleUIButtonsNextPrev(context, pkt[IBUS_PKT_DB2]);
    }
    if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE && pkt[IBUS_PKT_DB2] == 0x01) {
        if (context->mode == CD53_MODE_ACTIVE) {
            if (context->bt->activeDevice.a2dpId > 0) {
                if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                    // Set the display to paused so it doesn't flash back to the
                    // Now playing data
                    CD53SetMainDisplayText(context, "Paused", 0);
                    BTCommandPause(context->bt);
                } else {
                    BTCommandPlay(context->bt);
                }
            } else {
                CD53SetTempDisplayText(context, "No Device", 4);
                CD53SetMainDisplayText(context, "Bluetooth", 0);
            }
        } else {
            CD53RedisplayText(context);
        }
    } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE &&
               pkt[IBUS_PKT_DB2] == 0x02
    ) {
        if (context->mode == CD53_MODE_ACTIVE) {
            // Toggle Metadata scrolling
            if (context->displayMetadata == CD53_DISPLAY_METADATA_ON) {
                CD53SetMainDisplayText(context, "Bluetooth", 0);
                context->displayMetadata = CD53_DISPLAY_METADATA_OFF;
            } else {
                context->displayMetadata = CD53_DISPLAY_METADATA_ON;
                // We are sending a null pointer because we do not need
                // the second parameter
                CD53BTMetadata(context, 0x00);
            }
        } else if (context->mode == CD53_MODE_SETTINGS) {
            // Use as "Okay" button
            if (context->settingMode == CD53_SETTING_MODE_SCROLL_SETTINGS) {
                if (context->settingIdx != CD53_SETTING_IDX_PAIRINGS) {
                    CD53SetTempDisplayText(context, "Edit", 1);
                }
                context->settingMode = CD53_SETTING_MODE_SCROLL_VALUES;
                CD53RedisplayText(context);
            } else {
                context->settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
                if (context->settingIdx == CD53_SETTING_IDX_PAIRINGS) {
                    if (context->settingValue == CONFIG_SETTING_ON) {
                        if (context->bt->type == BT_BTM_TYPE_BC127) {
                            BC127CommandUnpair(context->bt);
                        } else {
                            BM83CommandRestore(context->bt);
                            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x00);
                            ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE, 0x00);
                        }
                        CD53SetTempDisplayText(context, "Unpaired", 1);
                    }
                } else if (context->settingIdx == CD53_SETTING_IDX_VEH_TYPE) {
                    ConfigSetVehicleType(context->settingValue);
                    CD53SetTempDisplayText(context, "Saved", 1);
                } else if (context->settingIdx == CD53_SETTING_IDX_COMFORT_LOCKS) {
                    ConfigSetComfortLock(context->settingValue);
                    CD53SetTempDisplayText(context, "Saved", 1);
                } else if (context->settingIdx == CD53_SETTING_IDX_COMFORT_UNLOCK) {
                    ConfigSetComfortUnlock(context->settingValue);
                    CD53SetTempDisplayText(context, "Saved", 1);
                } else {
                    ConfigSetSetting(
                        SETTINGS_TO_MENU[context->settingIdx],
                        context->settingValue
                    );
                    CD53SetTempDisplayText(context, "Saved", 1);
                    if (context->settingIdx == CD53_SETTING_IDX_HFP) {
                        if (context->settingValue == CONFIG_SETTING_OFF) {
                            BC127CommandSetProfiles(context->bt, 1, 1, 0, 0);
                        } else {
                            BC127CommandSetProfiles(context->bt, 1, 1, 0, 1);
                        }
                        BC127CommandReset(context->bt);
                    }
                }
                CD53MenuSettingsShow(context, context->settingIdx);
            }
        } else if (context->mode == CD53_MODE_DEVICE_SEL) {
            BTPairedDevice_t *dev = &context->bt->pairedDevices[
                context->btDeviceIndex
            ];
            // Do nothing if the user selected the active device
            if (memcmp(dev->macId, context->bt->activeDevice.macId, BT_LEN_MAC_ID) != 0) {
                // Trigger device selection event
                EventTriggerCallback(
                    UIEvent_InitiateConnection,
                    (unsigned char *)&context->btDeviceIndex
                );
                CD53SetTempDisplayText(context, "Connecting", 2);
            } else {
                CD53SetTempDisplayText(context, "Connected", 2);
            }
            CD53RedisplayText(context);
        }
    } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE && pkt[3] == 0x03) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            uint32_t now = TimerGetMillis();
            if (context->bt->callStatus == BT_CALL_ACTIVE) {
                BTCommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BT_CALL_INCOMING) {
                BTCommandCallAccept(context->bt);
            } else if (context->bt->callStatus == BT_CALL_OUTGOING) {
                BTCommandCallEnd(context->bt);
            }
            if ((now - context->lastTelephoneButtonPress) <= CD53_VR_TOGGLE_TIME &&
                context->bt->callStatus == BT_CALL_INACTIVE
            ) {
                BTCommandToggleVoiceRecognition(context->bt);
            }
        }
        context->lastTelephoneButtonPress = TimerGetMillis();
        CD53RedisplayText(context);
    } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE &&  pkt[IBUS_PKT_DB2] == 0x04) {
        // Settings Menu
        if (context->mode != CD53_MODE_SETTINGS) {
            CD53SetTempDisplayText(context, "Settings", 2);
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "Handsfree: Off", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            } else {
                CD53SetMainDisplayText(context, "Handsfree: On", 0);
                context->settingValue = CONFIG_SETTING_ON;
            }
            context->settingIdx = CD53_SETTING_IDX_HFP;
            context->mode = CD53_MODE_SETTINGS;
            context->settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
        } else {
            context->mode = CD53_MODE_ACTIVE;
            CD53SetMainDisplayText(context, "Bluetooth", 0);
            if (context->displayMetadata != CD53_DISPLAY_METADATA_OFF) {
                CD53BTMetadata(context, 0x00);
            }
        }
    } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE && pkt[IBUS_PKT_DB2] == 0x05) {
        // Device selection mode
        if (context->mode != CD53_MODE_DEVICE_SEL) {
            if (context->bt->pairedDevicesCount == 0) {
                CD53SetTempDisplayText(context, "No Devices", 4);
            } else {
                CD53SetTempDisplayText(context, "Devices", 2);
                context->btDeviceIndex = CD53_PAIRING_DEVICE_NONE;
                CD53ShowNextAvailableDevice(context, 0);
            }
            context->mode = CD53_MODE_DEVICE_SEL;
        } else {
            context->mode = CD53_MODE_ACTIVE;
            CD53SetMainDisplayText(context, "Bluetooth", 0);
            if (context->displayMetadata != CD53_DISPLAY_METADATA_OFF) {
                CD53BTMetadata(context, 0x00);
            }
        }
    } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE && pkt[IBUS_PKT_DB2] == 0x06) {
        // Toggle the discoverable state
        uint8_t state;
        int8_t timeout = 1500 / CD53_DISPLAY_SCROLL_SPEED;
        if (context->bt->discoverable == BT_STATE_ON) {
            CD53SetTempDisplayText(context, "Pairing Off", timeout);
            state = BT_STATE_OFF;
        } else {
            CD53SetTempDisplayText(context, "Pairing On", timeout);
            state = BT_STATE_ON;
            // To pair a new device, we must disconnect the active one
            EventTriggerCallback(UIEvent_CloseConnection, 0x00);
        }
        BTCommandSetDiscoverable(context->bt, state);
    } else {
        // A button was pressed - Push our display text back
        if (context->mode == CD53_MODE_ACTIVE) {
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
        } else if (context->mode != CD53_MODE_OFF) {
            CD53RedisplayText(context);
        }
    }
}

void CD53BTCallerID(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode != CD53_MODE_CALL) {
        context->mode = CD53_MODE_CALL;
        context->mainDisplay.timeout = 0;
        CD53SetMainDisplayText(
            context,
            context->bt->callerId,
            3000 / CD53_DISPLAY_SCROLL_SPEED
        );
    }
}

void CD53BTCallStatus(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode == CD53_MODE_CALL &&
        context->bt->scoStatus != BT_CALL_SCO_OPEN
    ) {
        // Clear Caller ID
        if (context->displayMetadata == CD53_DISPLAY_METADATA_ON) {
            CD53BTMetadata(context, 0x00);
        } else {
            CD53SetMainDisplayText(context, "Bluetooth", 0);
        }
        context->mode = CD53_MODE_ACTIVE;
    }
}


void CD53BTDeviceDisconnected(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode == CD53_MODE_ACTIVE) {
        CD53SetMainDisplayText(context, "Bluetooth", 0);
    }
}

void CD53BTDeviceReady(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // The BT Device Reset -- Clear the Display
    context->mainDisplay = UtilsDisplayValueInit("", CD53_DISPLAY_STATUS_OFF);
    // If we're in Bluetooth mode, display our banner
    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
        CD53SetMainDisplayText(context, "Bluetooth", 0);
    }
}


void CD53BTMetadata(CD53Context_t *context, unsigned char *metadata)
{
    if (context->displayMetadata == CD53_DISPLAY_METADATA_ON &&
        context->mode == CD53_MODE_ACTIVE
    ) {
        if (strlen(context->bt->title) > 0) {
            char text[UTILS_DISPLAY_TEXT_SIZE] = {0};
            if (strlen(context->bt->artist) > 0 && strlen(context->bt->album) > 0) {
                snprintf(
                    text,
                    UTILS_DISPLAY_TEXT_SIZE,
                    "%s - %s on %s",
                    context->bt->title,
                    context->bt->artist,
                    context->bt->album
                );
            } else if (strlen(context->bt->artist) > 0) {
                snprintf(
                    text,
                    UTILS_DISPLAY_TEXT_SIZE,
                    "%s - %s",
                    context->bt->title,
                    context->bt->artist
                );
            } else if (strlen(context->bt->album) > 0) {
                snprintf(
                    text,
                    UTILS_DISPLAY_TEXT_SIZE,
                    "%s on %s",
                    context->bt->title,
                    context->bt->album
                );
            } else {
                snprintf(text, UTILS_DISPLAY_TEXT_SIZE, "%s", context->bt->title);
            }
            context->mainDisplay.timeout = 0;
            CD53SetMainDisplayText(context, text, 3000 / CD53_DISPLAY_SCROLL_SPEED);
            if (context->mediaChangeState == CD53_MEDIA_STATE_CHANGE) {
                context->mediaChangeState = CD53_MEDIA_STATE_METADATA_OK;
            }
        } else {
            CD53SetMainDisplayText(context, "Bluetooth", 0);
        }
    }
}

void CD53BTPlaybackStatus(void *ctx, unsigned char *status)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // Display "Paused" if we're in Bluetooth mode
    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING &&
        context->mode == CD53_MODE_ACTIVE &&
        context->displayMetadata
    ) {
        if (context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED) {
            // If we are not mid-song change
            if (context->mediaChangeState == CD53_MEDIA_STATE_OK) {
                CD53SetMainDisplayText(context, "Paused", 0);
            }
        } else {
            if (context->mediaChangeState == CD53_MEDIA_STATE_OK) {
                CD53BTMetadata(context, 0x00);
            }
            context->mediaChangeState = CD53_MEDIA_STATE_OK;
        }
    }
}

/**
 * CD53IBusBMBTButtonPress()
 *     Description:
 *         Handle button presses on the BoardMonitor when installed with
 *         a monochrome navigation unit
 *     Params:
 *         void *context - A void pointer to the CD53Context_t struct
 *         unsigned char *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void CD53IBusBMBTButtonPress(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode != CD53_MODE_OFF) {
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_PlayPause) {
            if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                BTCommandPause(context->bt);
            } else {
                BTCommandPlay(context->bt);
            }
        }
    }
}

void CD53IBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    unsigned char requestedCommand = pkt[IBUS_PKT_DB1];
    uint8_t btPlaybackStatus = context->bt->playbackStatus;
    if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        // Stop Playing
        IBusCommandTELIKEDisplayClear(context->ibus);
        context->mode = CD53_MODE_OFF;
    } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING) {
        // Start Playing
        if (context->mode == CD53_MODE_OFF) {
            CD53SetMainDisplayText(context, "Bluetooth", 0);
            if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON) {
                BTCommandPlay(context->bt);
            } else if (btPlaybackStatus == BT_AVRCP_STATUS_PLAYING) {
                BTCommandPause(context->bt);
            }
            //BC127CommandStatus(context->bt);
            context->mode = CD53_MODE_ACTIVE;
        }
    } else if (requestedCommand == IBUS_CDC_CMD_SCAN ||
               requestedCommand == IBUS_CDC_CMD_RANDOM_MODE
    ) {
        if (context->mode == CD53_MODE_ACTIVE) {
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
        } else if (context->mode != CD53_MODE_OFF) {
            CD53RedisplayText(context);
        }
    }
    if (requestedCommand == IBUS_CDC_CMD_CD_CHANGE ||
        requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK ||
        requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK_BLAUPUNKT
    ) {
        CD53HandleUIButtons(context, pkt);
    }
}

void CD53IBusMFLButton(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL) {
        // 0x00 = R/T Mode RAD
        if (pkt[IBUS_PKT_DB1] == 0x00 &&
            context->displayMetadata == CD53_DISPLAY_METADATA_ON
        ) {
            //CD53RedisplayText(context);
        } else if (pkt[IBUS_PKT_DB1] == IBUS_MFL_BTN_EVENT_NEXT_REL ||
                   pkt[IBUS_PKT_DB1] == IBUS_MFL_BTN_EVENT_PREV_REL
        ) {
            unsigned char direction = 0x00;
            if (pkt[IBUS_PKT_DB1] == IBUS_MFL_BTN_EVENT_PREV_REL) {
                direction = 0x01;
            }
            CD53HandleUIButtonsNextPrev(context, direction);
        }
    }
}

void CD53IBusRADWriteDisplay(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (pkt[IBUS_PKT_DB1] == 0xC4) {
        context->radioType = CONFIG_UI_BUSINESS_NAV;
    }
    if (context->mode != CD53_MODE_OFF) {
        CD53RedisplayText(context);
    }
}

void CD53TimerDisplay(void *ctx)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode != CD53_MODE_OFF) {
        // Display the temp text, if there is any
        if (context->tempDisplay.status > CD53_DISPLAY_STATUS_OFF) {
            if (context->tempDisplay.timeout == 0) {
                context->tempDisplay.status = CD53_DISPLAY_STATUS_OFF;
            } else if (context->tempDisplay.timeout > 0) {
                context->tempDisplay.timeout--;
            } else if (context->tempDisplay.timeout < -1) {
                context->tempDisplay.status = CD53_DISPLAY_STATUS_OFF;
            }
            if (context->tempDisplay.status == CD53_DISPLAY_STATUS_NEW) {
                if (context->radioType == CONFIG_UI_CD53) {
                    IBusCommandTELIKEDisplayWrite(
                        context->ibus,
                        context->tempDisplay.text
                    );
                } else if (context->radioType == CONFIG_UI_BUSINESS_NAV) {
                    IBusCommandGTWriteBusinessNavTitle(context->ibus, context->tempDisplay.text);
                }
                context->tempDisplay.status = CD53_DISPLAY_STATUS_ON;
            }
            if (context->mainDisplay.length <= CD53_DISPLAY_TEXT_LEN) {
                context->mainDisplay.index = 0;
            }
        } else {
            // Display the main text if there isn't a timeout set
            if (context->mainDisplay.timeout > 0) {
                context->mainDisplay.timeout--;
            } else {
                if (context->mainDisplay.length > CD53_DISPLAY_TEXT_LEN) {
                    char text[CD53_DISPLAY_TEXT_LEN + 1] = {0};
                    uint8_t textLength = CD53_DISPLAY_TEXT_LEN;
                    uint8_t idxEnd = context->mainDisplay.index + textLength;
                    // Prevent strncpy() from going out of bounds
                    if (idxEnd >= context->mainDisplay.length) {
                        textLength = (context->mainDisplay.length - 1) - context->mainDisplay.index;
                        idxEnd = context->mainDisplay.index + textLength;
                    }
                    UtilsStrncpy(
                        text,
                        &context->mainDisplay.text[context->mainDisplay.index],
                        textLength
                    );
                    if (context->radioType == CONFIG_UI_CD53) {
                        IBusCommandTELIKEDisplayWrite(context->ibus, text);
                    } else if (context->radioType == CONFIG_UI_BUSINESS_NAV) {
                        IBusCommandGTWriteBusinessNavTitle(context->ibus, text);
                    }
                    // Pause at the beginning of the text
                    if (context->mainDisplay.index == 0) {
                        context->mainDisplay.timeout = 5;
                    }
                    if (idxEnd >= context->mainDisplay.length) {
                        // Pause at the end of the text
                        context->mainDisplay.timeout = 2;
                        context->mainDisplay.index = 0;
                    } else {
                        if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) ==
                            CD53_METADATA_MODE_CHUNK
                        ) {
                            context->mainDisplay.timeout = 2;
                            context->mainDisplay.index += CD53_DISPLAY_TEXT_LEN;
                        } else {
                            context->mainDisplay.index++;
                        }
                    }
                } else {
                    if (context->mainDisplay.index == 0) {
                        if (context->radioType == CONFIG_UI_CD53) {
                            IBusCommandTELIKEDisplayWrite(
                                context->ibus, 
                                context->mainDisplay.text
                            );
                        } else if (context->radioType == CONFIG_UI_BUSINESS_NAV) {
                            IBusCommandGTWriteBusinessNavTitle(
                                context->ibus,
                                context->mainDisplay.text
                            );
                        }
                    }
                    context->mainDisplay.index = 1;
                }
            }
        }
    }
}
