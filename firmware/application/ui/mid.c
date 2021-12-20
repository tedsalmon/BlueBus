/*
 * File: mid.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#include "mid.h"
static MIDContext_t Context;

uint8_t MID_SETTINGS_MENU[] = {
    MID_SETTING_IDX_HFP,
    MID_SETTING_IDX_METADATA_MODE,
    MID_SETTING_IDX_AUTOPLAY,
    MID_SETTING_IDX_LOWER_VOL_REV,
    MID_SETTING_IDX_VEH_TYPE,
    MID_SETTING_IDX_BLINKERS,
    MID_SETTING_IDX_COMFORT_LOCKS,
    MID_SETTING_IDX_COMFORT_UNLOCK,
    MID_SETTING_IDX_AUDIO_DSP,
    MID_SETTING_IDX_PAIRINGS
};

uint8_t MID_SETTINGS_TO_MENU[] = {
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY,
    CONFIG_SETTING_VOLUME_LOWER_ON_REV,
    CONFIG_VEHICLE_TYPE_ADDRESS,
    CONFIG_SETTING_COMFORT_BLINKERS
};

void MIDInit(BC127_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.btDeviceIndex = 0;
    Context.mode = MID_MODE_OFF;
    Context.displayUpdate = MID_DISPLAY_NONE;
    Context.mainDisplay = UtilsDisplayValueInit("", MID_DISPLAY_STATUS_OFF);
    Context.tempDisplay = UtilsDisplayValueInit("", MID_DISPLAY_STATUS_OFF);
    Context.modeChangeStatus = MID_MODE_CHANGE_OFF;
    strncpy(Context.mainText, "Bluetooth", 10);
    EventRegisterCallback(
        BC127Event_MetadataChange,
        &MIDBC127MetadataUpdate,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &MIDBC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &MIDIBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_MIDButtonPress,
        &MIDIBusMIDButtonPress,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_RADMIDDisplayText,
        &MIDIIBusRADMIDDisplayUpdate,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_MIDModeChange,
        &MIDIBusMIDModeChange,
        &Context
    );
    TimerRegisterScheduledTask(
        &MIDTimerMenuWrite,
        &Context,
        MID_TIMER_MENU_WRITE_INT
    );
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &MIDTimerDisplay,
        &Context,
        MID_TIMER_DISPLAY_INT
    );
}

/**
 * MIDDestroy()
 *     Description:
 *         Unregister all event handlers, scheduled tasks and clear the context
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void MIDDestroy()
{
    EventUnregisterCallback(
        BC127Event_MetadataChange,
        &MIDBC127MetadataUpdate
    );
    EventUnregisterCallback(
        BC127Event_PlaybackStatusChange,
        &MIDBC127PlaybackStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &MIDIBusCDChangerStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_MIDButtonPress,
        &MIDIBusMIDButtonPress
    );
    EventUnregisterCallback(
        IBUS_EVENT_RADMIDDisplayText,
        &MIDIIBusRADMIDDisplayUpdate
    );
    EventUnregisterCallback(
        IBUS_EVENT_MIDModeChange,
        &MIDIBusMIDModeChange
    );
    TimerUnregisterScheduledTask(&MIDTimerMenuWrite);
    TimerUnregisterScheduledTask(&MIDTimerDisplay);
    memset(&Context, 0, sizeof(MIDContext_t));
}

static void MIDSetMainDisplayText(
    MIDContext_t *context,
    const char *str,
    int8_t timeout
) {
    char text[UTILS_DISPLAY_TEXT_SIZE] = {0};
    snprintf(
        text,
        UTILS_DISPLAY_TEXT_SIZE,
        "%s %s",
        context->mainText,
        str
    );
    strncpy(context->mainDisplay.text, text, UTILS_DISPLAY_TEXT_SIZE - 1);
    context->mainDisplay.length = strlen(context->mainDisplay.text);
    context->mainDisplay.index = 0;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
    context->mainDisplay.timeout = timeout;
}

static void MIDSetTempDisplayText(
    MIDContext_t *context,
    char *str,
    int8_t timeout
) {
    char text[UTILS_DISPLAY_TEXT_SIZE] = {0};
    snprintf(
        text,
        UTILS_DISPLAY_TEXT_SIZE,
        "%s %s",
        context->mainText,
        str
    );
    strncpy(context->tempDisplay.text, text, UTILS_DISPLAY_TEXT_SIZE - 1);
    context->tempDisplay.length = strlen(context->tempDisplay.text);
    context->tempDisplay.index = 0;
    context->tempDisplay.status = MID_DISPLAY_STATUS_NEW;
    // Unlike the main display, we need to set the timeout beforehand, that way
    // the timer knows how many iterations to display the text for.
    context->tempDisplay.timeout = timeout;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

// Menu Creation
static void MIDShowNextDevice(MIDContext_t *context, uint8_t direction)
{
    if (context->bt->pairedDevicesCount == 0) {
        MIDSetMainDisplayText(context, "No Devices Available", 0);
    } else {
        if (direction == MID_BUTTON_NEXT_VAL) {
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
        BC127PairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
        char text[16];
        strncpy(text, dev->deviceName, 15);
        text[15] = '\0';
        // Add a space and asterisks to the end of the device name
        // if it's the currently selected device
        if (strcmp(dev->macId, context->bt->activeDevice.macId) == 0) {
            uint8_t startIdx = strlen(text);
            if (startIdx > 15) {
                startIdx = 16;
            }
            text[startIdx++] = 0x20;
            text[startIdx++] = 0x2A;
        }
        MIDSetMainDisplayText(context, text, 0);
    }
}

static void MIDShowNextSetting(MIDContext_t *context, uint8_t nextMenu)
{
    if (nextMenu == MID_SETTING_IDX_HFP) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
            MIDSetMainDisplayText(context, "Handsfree: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MIDSetMainDisplayText(context, "Handsfree: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MID_SETTING_IDX_HFP;
    }
    if (nextMenu == MID_SETTING_IDX_METADATA_MODE) {
        unsigned char value = ConfigGetSetting(
            CONFIG_SETTING_METADATA_MODE
        );
        if (value == MID_SETTING_METADATA_MODE_OFF) {
            MIDSetMainDisplayText(context, "Metadata: Off", 0);
        } else if (value == MID_SETTING_METADATA_MODE_PARTY) {
            MIDSetMainDisplayText(context, "Metadata: Party", 0);
        } else if (value == MID_SETTING_METADATA_MODE_CHUNK) {
            MIDSetMainDisplayText(context, "Metadata: Chunk", 0);
        }
        context->settingIdx = MID_SETTING_IDX_METADATA_MODE;
        context->settingValue = value;
    }
    if (nextMenu == MID_SETTING_IDX_AUTOPLAY) {
        if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == 0x00) {
            MIDSetMainDisplayText(context, "Autoplay: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MIDSetMainDisplayText(context, "Autoplay: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MID_SETTING_IDX_AUTOPLAY;
    }
    if (nextMenu == MID_SETTING_IDX_AUTOPLAY) {
        if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Lower Volume On Reverse: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MIDSetMainDisplayText(context, "Lower Vol. On Reverse: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MID_SETTING_IDX_LOWER_VOL_REV;
    }
    if (nextMenu == MID_SETTING_IDX_VEH_TYPE) {
        unsigned char vehicleType = ConfigGetVehicleType();
        context->settingValue = vehicleType;
        if (vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
            MIDSetMainDisplayText(context, "Car: E38/E39/E53", 0);
        } else if (vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
            MIDSetMainDisplayText(context, "Car: E46/Z4", 0);
        } else {
            MIDSetMainDisplayText(context, "Car: Unset", 0);
        }
        context->settingIdx = MID_SETTING_IDX_VEH_TYPE;
    }
    if (nextMenu == MID_SETTING_IDX_BLINKERS) {
        unsigned char blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
        if (blinkCount > 8 || blinkCount == 0) {
            blinkCount = 1;
        }
        char blinkerText[19] = {0};
        snprintf(blinkerText, 19, "Comfort Blinks: %d", context->settingValue);
        MIDSetMainDisplayText(context, blinkerText, 0);
        context->settingIdx = MID_SETTING_IDX_BLINKERS;
    }
    if (nextMenu == MID_SETTING_IDX_COMFORT_LOCKS) {
        context->settingValue = ConfigGetComfortLock();
        if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            MIDSetMainDisplayText(context, "Comfort Lock: 10km/h", 0);
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_20KM) {
            MIDSetMainDisplayText(context, "Comfort Lock: 20km/h", 0);
        } else {
            MIDSetMainDisplayText(context, "Comfort Lock: Off", 0);
        }
        context->settingIdx = MID_SETTING_IDX_COMFORT_LOCKS;
    }
    if (nextMenu == MID_SETTING_IDX_COMFORT_UNLOCK) {
        context->settingValue = ConfigGetComfortUnlock();
        if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            MIDSetMainDisplayText(context, "Comfort Unlock: Pos 1", 0);
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_0) {
            MIDSetMainDisplayText(context, "Comfort Unlock: Pos 0", 0);
        } else {
            MIDSetMainDisplayText(context, "Comfort Unlock: Off", 0);
        }
        context->settingIdx = MID_SETTING_IDX_COMFORT_UNLOCK;
    }
    if (nextMenu == MID_SETTING_IDX_AUDIO_DSP) {
        context->settingValue = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
        if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
            MIDSetMainDisplayText(context, "DSP: Digital", 0);
        } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_ANALOG) {
            MIDSetMainDisplayText(context, "DSP: Analog", 0);
        } else {
            MIDSetMainDisplayText(context, "DSP: Default", 0);
        }
        context->settingIdx = MID_SETTING_IDX_AUDIO_DSP;
    }
    if (nextMenu== MID_SETTING_IDX_PAIRINGS) {
        MIDSetMainDisplayText(context, "Clear Pairings", 0);
        context->settingIdx = MID_SETTING_IDX_PAIRINGS;
        context->settingValue = CONFIG_SETTING_OFF;
    }
}

static void MIDShowNextSettingValue(MIDContext_t *context, uint8_t direction)
{
    // Select different configuration options
    if (context->settingIdx == MID_SETTING_IDX_HFP) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_METADATA_MODE) {
        if (context->settingValue == MID_SETTING_METADATA_MODE_OFF) {
            MIDSetMainDisplayText(context, "Party", 0);
            context->settingValue = MID_SETTING_METADATA_MODE_PARTY;
        } else if (context->settingValue == MID_SETTING_METADATA_MODE_PARTY) {
            MIDSetMainDisplayText(context, "Chunk", 0);
            context->settingValue = MID_SETTING_METADATA_MODE_CHUNK;
        } else if (context->settingValue == MID_SETTING_METADATA_MODE_CHUNK) {
            MIDSetMainDisplayText(context, "Off", 0);
            context->settingValue = MID_SETTING_METADATA_MODE_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_AUTOPLAY) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_LOWER_VOL_REV) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_VEH_TYPE) {
        if (context->settingValue == 0x00 ||
            context->settingValue == 0xFF ||
            context->settingValue == IBUS_VEHICLE_TYPE_E46_Z4
        ) {
            MIDSetMainDisplayText(context, "E38/E39/E53", 0);
            context->settingValue = IBUS_VEHICLE_TYPE_E38_E39_E53;
        } else {
            MIDSetMainDisplayText(context, "E46/Z4", 0);
            context->settingValue = IBUS_VEHICLE_TYPE_E46_Z4;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_BLINKERS) {
        context->settingValue++;
        if (context->settingValue > 8) {
            context->settingValue = 1;
        }
        char blinkerText[2] = {0};
        snprintf(blinkerText, 2, "%d", context->settingValue);
        MIDSetMainDisplayText(context, blinkerText, 0);
    }
    if (context->settingIdx == MID_SETTING_IDX_COMFORT_LOCKS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "10km/h", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_10KM;
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            MIDSetMainDisplayText(context, "20km/h", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_20KM;
        } else {
            MIDSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_COMFORT_UNLOCK) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Pos 1", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_1;
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            MIDSetMainDisplayText(context, "Pos 0", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_0;
        } else {
            MIDSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_AUDIO_DSP) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Digital", 0);
            context->settingValue = CONFIG_SETTING_DSP_INPUT_SPDIF;
        } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
            MIDSetMainDisplayText(context, "Analog", 0);
            context->settingValue = CONFIG_SETTING_DSP_INPUT_ANALOG;
        } else {
            MIDSetMainDisplayText(context, "Default", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_PAIRINGS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Press Save", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "Clear Pairings", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
}


static void MIDMenuDevices(MIDContext_t *context)
{
    context->mode = MID_MODE_DEVICES;
    strncpy(context->mainText, "Devices", 8);
    context->btDeviceIndex = MID_PAIRING_DEVICE_NONE;
    MIDShowNextDevice(context, 0);
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, " Con");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_PREV_VAL, "<   ");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_NEXT_VAL, "   >");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_DEVICES_R, "   ");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_DEVICES_L, "   ");
}

static void MIDMenuMain(MIDContext_t *context)
{
    context->mode = MID_MODE_ACTIVE;
    strncpy(context->mainText, "Bluetooth", 10);
    MIDSetMainDisplayText(context, "", 0);
    MIDBC127MetadataUpdate((void *) context, 0x00);
    // This sucks
    unsigned char mainMenuText[] = {
        0x06,
        'M','e','t','a',
        0x06,
        'S', 'e', 't', 't',
        'i', 'n', 'g', 's',
        0x06,
        'D', 'e', 'v', 'i',
        'c', 'e', 's', ' ',
    };
    IBusCommandMIDMenuWriteMany(context->ibus, 0x61, mainMenuText, sizeof(mainMenuText));
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
        IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_PLAYBACK, ">  ");
    } else {
        IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_PLAYBACK, "|| ");
    }
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_PAIR, "Pair");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_MODE, "MODE");
}

static void MIDMenuSettings(MIDContext_t *context)
{
    context->mode = MID_MODE_SETTINGS;
    strncpy(context->mainText, "Settings", 9);
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
        MIDSetMainDisplayText(context, "Handsfree: Off", 0);
        context->settingValue = CONFIG_SETTING_OFF;
    } else {
        MIDSetMainDisplayText(context, "Handsfree: On", 0);
        context->settingValue = CONFIG_SETTING_ON;
    }
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_PREV_VAL, "<   ");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_NEXT_VAL, "   >");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_DEVICES_R, "   ");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_DEVICES_L, "   ");
    context->settingIdx = MID_SETTING_IDX_HFP;
    context->settingMode = MID_SETTING_MODE_SCROLL_SETTINGS;
}

void MIDBC127MetadataUpdate(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE &&
        strlen(context->bt->title) > 0 &&
        ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) != MID_SETTING_METADATA_MODE_OFF
    ) {
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
                UTILS_DISPLAY_TEXT_SIZE ,
                "%s on %s",
                context->bt->title,
                context->bt->album
            );
        } else {
            snprintf(text, UTILS_DISPLAY_TEXT_SIZE, "%s", context->bt->title);
        }
        MIDSetMainDisplayText(context, text, 3000 / MID_DISPLAY_SCROLL_SPEED);
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void MIDBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            IBusCommandMIDMenuWriteSingle(context->ibus, 0, " >");
            BC127CommandGetMetadata(context->bt);
        } else {
            if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) != MID_SETTING_METADATA_MODE_OFF) {
                MIDSetMainDisplayText(context, "Paused", 0);
            }
            IBusCommandMIDMenuWriteSingle(context->ibus, 0, "|| ");
        }
    }
}

void MIDIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    unsigned char requestedCommand = pkt[4];
    if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        if (context->mode != MID_MODE_DISPLAY_OFF &&
            context->mode != MID_MODE_OFF
        ) {
            IBusCommandMIDSetMode(context->ibus, IBUS_DEVICE_TEL, 0x00);
        }
        // Stop Playing
        context->mode = MID_MODE_OFF;
        context->modeChangeStatus = MID_MODE_CHANGE_OFF;
    } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING) {
        context->modeChangeStatus = MID_MODE_CHANGE_OFF;
        // Start Playing
        if (context->mode == MID_MODE_OFF) {
            IBusCommandMIDSetMode(context->ibus, IBUS_DEVICE_TEL, 0x02);
        }
    } else if (requestedCommand == IBUS_CDC_CMD_CD_CHANGE &&
               context->mode == MID_MODE_DISPLAY_OFF
    ) {
        IBusCommandMIDSetMode(context->ibus, IBUS_DEVICE_TEL, 0x02);
    } else if (requestedCommand == IBUS_CDC_CMD_GET_STATUS) {
        // Reset the UI if the user has chosen not to
        // continue with the mode change
        if (context->mode == MID_MODE_DISPLAY_OFF &&
            context->modeChangeStatus == MID_MODE_CHANGE_RELEASE
        ) {
            context->modeChangeStatus = MID_MODE_CHANGE_OFF;
            IBusCommandMIDSetMode(context->ibus, IBUS_DEVICE_TEL, 0x02);
        }
    }
}

void MIDIBusMIDButtonPress(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    unsigned char btnPressed = pkt[6];
    if (context->mode == MID_MODE_ACTIVE) {
        if (btnPressed == MID_BUTTON_PLAYBACK) {
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
                IBusCommandMIDMenuWriteSingle(context->ibus, 0, "|| ");
            } else {
                BC127CommandPlay(context->bt);
                IBusCommandMIDMenuWriteSingle(context->ibus, 0, ">  ");
            }
        } else if (btnPressed == MID_BUTTON_META) {
            if (context->mode == MID_MODE_ACTIVE) {
                context->mode = MID_MODE_DISPLAY_OFF;
            } else {
                context->mode = MID_MODE_ACTIVE_NEW;
            }
        } else if (btnPressed == 2 || btnPressed == 3) {
            context->mode = MID_MODE_SETTINGS_NEW;
        }  else if (btnPressed == 4 || btnPressed == 5) {
            context->mode = MID_MODE_DEVICES_NEW;
        } else if (btnPressed == MID_BUTTON_PAIR) {
            // Toggle the discoverable state
            uint8_t state;
            int8_t timeout = 1500 / MID_DISPLAY_SCROLL_SPEED;
            if (context->bt->discoverable == BC127_STATE_ON) {
                MIDSetTempDisplayText(context, "Pairing Off", timeout);
                state = BC127_STATE_OFF;
            } else {
                MIDSetTempDisplayText(context, "Pairing On", timeout);
                state = BC127_STATE_ON;
                if (context->bt->activeDevice.deviceId != 0) {
                    // To pair a new device, we must disconnect the active one
                    EventTriggerCallback(UIEvent_CloseConnection, 0x00);
                }
            }
            BC127CommandBtState(context->bt, context->bt->connectable, state);
        } else if (btnPressed == MID_BUTTON_MODE) {
            context->mode = MID_MODE_DISPLAY_OFF;
        }
    } else if (context->mode == MID_MODE_SETTINGS) {
        if (btnPressed == MID_BUTTON_BACK) {
            context->mode = MID_MODE_ACTIVE_NEW;
        } else if (btnPressed == MID_BUTTON_EDIT_SAVE) {
            if (context->settingMode == MID_SETTING_MODE_SCROLL_SETTINGS) {
                IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, "Save");
                context->settingMode = MID_SETTING_MODE_SCROLL_VALUES;
            } else if (context->settingMode == MID_SETTING_MODE_SCROLL_VALUES) {
                IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
                context->settingMode = MID_SETTING_MODE_SCROLL_SETTINGS;
                // Save Setting
                if (context->settingIdx == MID_SETTING_IDX_PAIRINGS) {
                    if (context->settingValue == CONFIG_SETTING_ON) {
                        BC127CommandUnpair(context->bt);
                        MIDSetTempDisplayText(context, "Unpaired", 1);
                    }
                } else if (context->settingIdx == MID_SETTING_IDX_VEH_TYPE) {
                    ConfigSetVehicleType(context->settingValue);
                    MIDSetTempDisplayText(context, "Saved", 1);
                } else if (context->settingIdx == MID_SETTING_IDX_COMFORT_LOCKS) {
                    ConfigSetComfortLock(context->settingValue);
                    MIDSetTempDisplayText(context, "Saved", 1);
                } else if (context->settingIdx == MID_SETTING_IDX_COMFORT_UNLOCK) {
                    ConfigSetComfortUnlock(context->settingValue);
                    MIDSetTempDisplayText(context, "Saved", 1);
                } else if (context->settingIdx == MID_SETTING_IDX_AUDIO_DSP) {
                    ConfigSetSetting(CONFIG_SETTING_DSP_INPUT_SRC, context->settingValue);
                    MIDSetTempDisplayText(context, "Saved", 1);
                    if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
                        IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
                    } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_ANALOG) {
                        IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
                    }
                } else {
                    ConfigSetSetting(
                        MID_SETTINGS_TO_MENU[context->settingIdx],
                        context->settingValue
                    );
                    MIDSetTempDisplayText(context, "Saved", 1);
                    if (context->settingIdx == MID_SETTING_IDX_HFP) {
                        if (context->settingValue == CONFIG_SETTING_OFF) {
                            BC127CommandSetProfiles(context->bt, 1, 1, 0, 0);
                        } else {
                            BC127CommandSetProfiles(context->bt, 1, 1, 0, 1);
                        }
                        BC127CommandReset(context->bt);
                    }
                }
                MIDShowNextSetting(context, context->settingIdx);
            }
        }  else if (btnPressed == MID_BUTTON_PREV_VAL ||
                   btnPressed == MID_BUTTON_NEXT_VAL
        ) {
            if (context->settingMode == MID_SETTING_MODE_SCROLL_SETTINGS) {
                uint8_t nextMenu = 0;
                if (context->settingIdx == MID_SETTING_IDX_HFP &&
                    btnPressed != MID_BUTTON_NEXT_VAL
                ) {
                    nextMenu = MID_SETTINGS_MENU[MID_SETTING_IDX_PAIRINGS];
                } else if (context->settingIdx == MID_SETTING_IDX_PAIRINGS &&
                           btnPressed == MID_BUTTON_NEXT_VAL
                ) {
                    nextMenu = MID_SETTINGS_MENU[MID_SETTING_IDX_HFP];
                } else {
                    if (btnPressed == MID_BUTTON_NEXT_VAL) {
                        nextMenu = MID_SETTINGS_MENU[context->settingIdx + 1];
                    } else {
                        nextMenu = MID_SETTINGS_MENU[context->settingIdx - 1];
                    }
                }
                MIDShowNextSetting(context, nextMenu);
            } else if (context->settingMode == MID_SETTING_MODE_SCROLL_VALUES) {
                MIDShowNextSettingValue(context, btnPressed);
            }
        }
    } else if (context->mode == MID_MODE_DEVICES) {
        if (btnPressed == MID_BUTTON_BACK) {
            context->mode = MID_MODE_ACTIVE_NEW;
        } else if (btnPressed == MID_BUTTON_EDIT_SAVE) {
            if (context->bt->pairedDevicesCount > 0) {
                // Connect to device
                BC127PairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
                if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0 &&
                    dev != 0
                ) {
                    // Trigger device selection event
                    EventTriggerCallback(
                        UIEvent_InitiateConnection,
                        (unsigned char *)&context->btDeviceIndex
                    );
                }
            }
        } else if (btnPressed == MID_BUTTON_PREV_VAL ||
                   btnPressed == MID_BUTTON_NEXT_VAL
        ) {
            MIDShowNextDevice(context, btnPressed);
        }
    }
    if (btnPressed == MID_BUTTON_MODE && pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL) {
        // Close TEL UI
        IBusCommandMIDSetMode(context->ibus, IBUS_DEVICE_TEL, 0x00);
        context->modeChangeStatus = MID_MODE_CHANGE_PRESS;
    }
    if (btnPressed == MID_BUTTON_MODE &&
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_RAD &&
        context->modeChangeStatus == MID_MODE_CHANGE_RELEASE
    ) {
        IBusCommandMIDButtonPress(context->ibus, IBUS_DEVICE_RAD, MID_BUTTON_MODE | 0x40);
    }
    // Handle Next and Previous
    if (context->ibus->cdChangerFunction != IBUS_CDC_FUNC_NOT_PLAYING) {
        if (btnPressed == IBus_MID_BTN_TEL_RIGHT_RELEASE) {
            BC127CommandForward(context->bt);
        } else if (btnPressed == IBus_MID_BTN_TEL_LEFT_RELEASE) {
            BC127CommandBackward(context->bt);
        }
    }
}

/**
 * MIDIIBusRADMIDDisplayUpdate()
 *     Description:
 *         Handle the RAD writing to the MID.
 *     Params:
 *         void *context - A void pointer to the MIDContext_t struct
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void MIDIIBusRADMIDDisplayUpdate(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    unsigned char watermark = pkt[pkt[IBUS_PKT_LEN]];
    if (watermark != IBUS_RAD_MAIN_AREA_WATERMARK) {
        if ((context->mode == MID_MODE_ACTIVE ||
             context->mode == MID_MODE_DISPLAY_OFF) &&
            context->modeChangeStatus == MID_MODE_CHANGE_OFF
        ) {
            IBusCommandMIDDisplayRADTitleText(context->ibus, "Bluetooth");
        }
    }
}

/**
 * MIDIBusMIDModeChange()
 *     Description:
 *         
 *     Params:
 *         void *context - A void pointer to the MIDContext_t struct
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void MIDIBusMIDModeChange(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (pkt[IBUS_PKT_DB2] == 0x8E) {
        if (pkt[IBUS_PKT_DB1] == IBUS_MID_MODE_REQUEST_TYPE_PHYSICAL) {
            if (ConfigGetSetting(CONFIG_SETTING_SELF_PLAY) == CONFIG_SETTING_ON) {
                IBusCommandRADCDCRequest(context->ibus, IBUS_CDC_CMD_START_PLAYING);
            }
        } else {
            context->mode = MID_MODE_ACTIVE_NEW;
        }
    } else if (pkt[IBUS_PKT_DB2] != 0x8F) {
        if (pkt[IBUS_PKT_DB2] == 0x00) {
            if (context->mode != MID_MODE_DISPLAY_OFF &&
                context->mode != MID_MODE_OFF
            ) {
                IBusCommandMIDSetMode(context->ibus, IBUS_DEVICE_TEL, 0x02);
            }
        } else if (context->mode != MID_MODE_OFF) {
            context->mode = MID_MODE_DISPLAY_OFF;
        }
        if (pkt[IBUS_PKT_DB2] == 0xB0 &&
               context->modeChangeStatus == MID_MODE_CHANGE_PRESS
        ) {
            IBusCommandMIDButtonPress(context->ibus, IBUS_DEVICE_RAD, MID_BUTTON_MODE);
            context->modeChangeStatus = MID_MODE_CHANGE_RELEASE;
        }
    } else {
        // This should be 0x8F, which is "close TEL UI"
        if (ConfigGetSetting(CONFIG_SETTING_SELF_PLAY) == CONFIG_SETTING_ON) {
            IBusCommandRADCDCRequest(context->ibus, IBUS_CDC_CMD_STOP_PLAYING);
        }
    }
}

void MIDTimerMenuWrite(void *ctx)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    switch (context->mode) {
        case MID_MODE_ACTIVE_NEW:
            MIDMenuMain(context);
            break;
        case MID_MODE_SETTINGS_NEW:
            MIDMenuSettings(context);
            break;
        case MID_MODE_DEVICES_NEW:
            MIDMenuDevices(context);
            break;
    }
}

void MIDTimerDisplay(void *ctx)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode != MID_MODE_OFF && context->mode != MID_MODE_DISPLAY_OFF) {
        // Display the temp text, if there is any
        if (context->tempDisplay.status > MID_DISPLAY_STATUS_OFF) {
            if (context->tempDisplay.timeout == 0) {
                context->tempDisplay.status = MID_DISPLAY_STATUS_OFF;
            } else if (context->tempDisplay.timeout > 0) {
                context->tempDisplay.timeout--;
            } else if (context->tempDisplay.timeout < -1) {
                context->tempDisplay.status = MID_DISPLAY_STATUS_OFF;
            }
            if (context->tempDisplay.status == MID_DISPLAY_STATUS_NEW) {
                IBusCommandMIDDisplayText(
                    context->ibus,
                    context->tempDisplay.text
                );
                context->tempDisplay.status = MID_DISPLAY_STATUS_ON;
            }
            if (context->mainDisplay.length <= MID_DISPLAY_TEXT_SIZE) {
                context->mainDisplay.index = 0;
            }
        } else {
            // Display the main text if there isn't a timeout set
            if (context->mainDisplay.timeout > 0) {
                context->mainDisplay.timeout--;
            } else {
                if (context->mainDisplay.length > MID_DISPLAY_TEXT_SIZE) {
                    char text[MID_DISPLAY_TEXT_SIZE + 1] = {0};
                    uint8_t textLength = MID_DISPLAY_TEXT_SIZE;
                    // Prevent strncpy() from going out of bounds
                    if ((context->mainDisplay.index + textLength) > context->mainDisplay.length) {
                        textLength = context->mainDisplay.length - context->mainDisplay.index;
                    }
                    strncpy(
                        text,
                        &context->mainDisplay.text[context->mainDisplay.index],
                        textLength
                    );
                    IBusCommandMIDDisplayText(context->ibus, text);
                    // Pause at the beginning of the text
                    if (context->mainDisplay.index == 0) {
                        context->mainDisplay.timeout = 5;
                    }
                    uint8_t idxEnd = context->mainDisplay.index + MID_DISPLAY_TEXT_SIZE;
                    if (idxEnd >= context->mainDisplay.length) {
                        // Pause at the end of the text
                        context->mainDisplay.timeout = 2;
                        context->mainDisplay.index = 0;
                    } else {
                        if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) ==
                            MID_SETTING_METADATA_MODE_CHUNK
                        ) {
                            context->mainDisplay.timeout = 2;
                            context->mainDisplay.index += MID_DISPLAY_TEXT_SIZE;
                        } else {
                            context->mainDisplay.index++;
                        }
                    }
                } else {
                    if (context->mainDisplay.index == 0 &&
                        strlen(context->mainDisplay.text) > 0
                    ) {
                        IBusCommandMIDDisplayText(
                            context->ibus,
                            context->mainDisplay.text
                        );
                    }
                    context->mainDisplay.index = 1;
                }
            }
        }
    }
}
