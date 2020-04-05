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
    MID_SETTING_IDX_VEH_TYPE,
    MID_SETTING_IDX_BLINKERS,
    MID_SETTING_IDX_COMFORT_LOCKS,
    MID_SETTING_IDX_TCU_MODE,
    MID_SETTING_IDX_PAIRINGS,
};

uint8_t MID_SETTINGS_TO_MENU[] = {
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY,
    CONFIG_VEHICLE_TYPE_ADDRESS,
    CONFIG_SETTING_COMFORT_BLINKERS,
    CONFIG_SETTING_COMFORT_LOCKS,
    CONFIG_SETTING_TCU_MODE
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
        IBusEvent_CDStatusRequest,
        &MIDIBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_MIDButtonPress,
        &MIDIBusMIDButtonPress,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_RADMIDDisplayText,
        &MIDIIBusRADMIDDisplayUpdate,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_RADMIDDisplayMenu,
        &MIDIIBusRADMIDMenuUpdate,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_MIDModeChange,
        &MIDIBusMIDModeChange,
        &Context
    );
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &MIDTimerDisplay,
        &Context,
        MID_DISPLAY_TIMER_INT
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
        IBusEvent_CDStatusRequest,
        &MIDIBusCDChangerStatus
    );
    EventUnregisterCallback(
        IBusEvent_MIDButtonPress,
        &MIDIBusMIDButtonPress
    );
    EventUnregisterCallback(
        IBusEvent_RADMIDDisplayText,
        &MIDIIBusRADMIDDisplayUpdate
    );
    EventUnregisterCallback(
        IBusEvent_RADMIDDisplayMenu,
        &MIDIIBusRADMIDMenuUpdate
    );
    EventUnregisterCallback(
        IBusEvent_MIDModeChange,
        &MIDIBusMIDModeChange
    );
    TimerUnregisterScheduledTask(&MIDTimerDisplay);
    memset(&Context, 0, sizeof(MIDContext_t));
}

static void MIDSetMainDisplayText(
    MIDContext_t *context,
    const char *str,
    int8_t timeout
) {
    strncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE - 1);
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
    strncpy(context->tempDisplay.text, str, MID_DISPLAY_TEXT_SIZE - 1);
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
        UtilsRemoveNonAscii(text, dev->deviceName);
        char cleanText[16];
        strncpy(cleanText, text, 15);
        text[15] = '\0';
        // Add a space and asterisks to the end of the device name
        // if it's the currently selected device
        if (strcmp(dev->macId, context->bt->activeDevice.macId) == 0) {
            uint8_t startIdx = strlen(cleanText);
            if (startIdx > 15) {
                startIdx = 16;
            }
            cleanText[startIdx++] = 0x20;
            cleanText[startIdx++] = 0x2A;
        }
        MIDSetMainDisplayText(context, cleanText, 0);
    }
}

static void MIDShowNextSetting(MIDContext_t *context, uint8_t direction)
{
    uint8_t nextMenu = 0;
    if (context->settingIdx == MID_SETTING_IDX_HFP && direction != MID_BUTTON_NEXT_VAL) {
        nextMenu = MID_SETTINGS_MENU[MID_SETTING_IDX_PAIRINGS];
    } else if(context->settingIdx == MID_SETTING_IDX_PAIRINGS && direction == MID_BUTTON_NEXT_VAL) {
        nextMenu = MID_SETTINGS_MENU[MID_SETTING_IDX_HFP];
    } else {
        if (direction == MID_BUTTON_NEXT_VAL) {
            nextMenu = MID_SETTINGS_MENU[context->settingIdx + 1];
        } else {
            nextMenu = MID_SETTINGS_MENU[context->settingIdx - 1];
        }
    }
    if (nextMenu == MID_SETTING_IDX_HFP) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
            MIDSetMainDisplayText(context, "HFP: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MIDSetMainDisplayText(context, "HFP: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MID_SETTING_IDX_HFP;
    }
    if (nextMenu == MID_SETTING_IDX_METADATA_MODE) {
        unsigned char value = ConfigGetSetting(
            CONFIG_SETTING_METADATA_MODE
        );
        if (value == MID_SETTING_METADATA_MODE_PARTY) {
            MIDSetMainDisplayText(context, "Meta: Party", 0);
        } else if (value == MID_SETTING_METADATA_MODE_CHUNK) {
            MIDSetMainDisplayText(context, "Meta: Chunk", 0);
        } else {
            MIDSetMainDisplayText(context, "Meta: Party", 0);
            value = MID_SETTING_METADATA_MODE_PARTY;
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
        if (blinkCount == 0x03) {
            MIDSetMainDisplayText(context, "OT Blink: 3", 0);
            context->settingValue = 0x03;
        } else if (blinkCount == 0x05) {
            MIDSetMainDisplayText(context, "OT Blink: 5", 0);
            context->settingValue = 0x05;
        } else {
            MIDSetMainDisplayText(context, "OT Blink: 1", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
        context->settingIdx = MID_SETTING_IDX_BLINKERS;
    }
    if (nextMenu == MID_SETTING_IDX_COMFORT_LOCKS) {
        if (ConfigGetSetting(CONFIG_SETTING_COMFORT_LOCKS) == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Comfort Locks: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MIDSetMainDisplayText(context, "Comfort Locks: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MID_SETTING_IDX_COMFORT_LOCKS;
    }
    if (nextMenu == MID_SETTING_IDX_TCU_MODE) {
        unsigned char tcuMode = ConfigGetSetting(CONFIG_SETTING_TCU_MODE);
        if (tcuMode == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "TCU Mode: Always", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MIDSetMainDisplayText(context, "TCU Mode: Out of BT", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MID_SETTING_IDX_TCU_MODE;
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
            MIDSetMainDisplayText(context, "HFP: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "HFP: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_METADATA_MODE) {
        if (context->settingValue == MID_SETTING_METADATA_MODE_CHUNK) {
            MIDSetMainDisplayText(context, "Meta: Party", 0);
            context->settingValue = MID_SETTING_METADATA_MODE_PARTY;
        } else if (context->settingValue == MID_SETTING_METADATA_MODE_PARTY) {
            MIDSetMainDisplayText(context, "Meta: Chunk", 0);
            context->settingValue = MID_SETTING_METADATA_MODE_CHUNK;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_AUTOPLAY) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Autoplay: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "Autoplay: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_VEH_TYPE) {
        if (context->settingValue == 0x00 ||
            context->settingValue == 0xFF ||
            context->settingValue == IBUS_VEHICLE_TYPE_E46_Z4
        ) {
            MIDSetMainDisplayText(context, "Car: E38/E39/E53", 0);
            context->settingValue = IBUS_VEHICLE_TYPE_E38_E39_E53;
        } else {
            MIDSetMainDisplayText(context, "Car: E46/Z4", 0);
            context->settingValue = IBUS_VEHICLE_TYPE_E46_Z4;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_BLINKERS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "OT Blink: 3", 0);
            context->settingValue = 0x03;
        } else if (context->settingValue == 0x03) {
            MIDSetMainDisplayText(context, "OT Blink: 5", 0);
            context->settingValue = 0x05;
        } else {
            MIDSetMainDisplayText(context, "OT Blink: 1", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_COMFORT_LOCKS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MIDSetMainDisplayText(context, "Comfort Locks: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "Comfort Locks: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MID_SETTING_IDX_TCU_MODE) {
        if (context->settingValue == CONFIG_SETTING_ON) {
            MIDSetMainDisplayText(context, "TCU Mode: Out of BT", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MIDSetMainDisplayText(context, "TCU Mode: Always", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    //if (context->settingIdx == MID_SETTING_IDX_DAC_GAIN) {
    //    unsigned char currentVolume = ConfigGetSetting(CONFIG_SETTING_DAC_VOL);
    //    if (context->settingValue == CONFIG_SETTING_ON) {
    //        MIDSetMainDisplayText(context, "TCU Mode: Out of BT", 0);
    //        context->settingValue = CONFIG_SETTING_ON;
    //    } else {
    //        MIDSetMainDisplayText(context, "TCU Mode: Always", 0);
    //        context->settingValue = CONFIG_SETTING_OFF;
    //    }
    //}
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
    IBusCommandMIDDisplayTitleText(context->ibus, "Devices");
    context->btDeviceIndex = MID_PAIRING_DEVICE_NONE;
    MIDShowNextDevice(context, 0);
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_EDIT_SAVE, " Con");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_PREV_VAL, "<   ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_NEXT_VAL, "   >");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_R, "   ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_L, "   ");
}

static void MIDMenuMain(MIDContext_t *context)
{
    context->mode = MID_MODE_ACTIVE;
    IBusCommandMIDDisplayTitleText(context->ibus, "Bluetooth");
    MIDBC127MetadataUpdate((void *) context, 0x00);
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
        IBusCommandMIDMenuText(context->ibus, MID_BUTTON_PLAYBACK, ">  ");
    } else {
        IBusCommandMIDMenuText(context->ibus, MID_BUTTON_PLAYBACK, "|| ");
    }
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_META, "Meta");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_SETTINGS_L, "Sett");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_SETTINGS_R, "ings");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_L, "Devi");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_R, "ces ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_PAIR, "Pair");
}

static void MIDMenuSettings(MIDContext_t *context)
{
    context->mode = MID_MODE_SETTINGS;
    IBusCommandMIDDisplayTitleText(context->ibus, "Settings");
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
        MIDSetMainDisplayText(context, "HFP: Off", 0);
        context->settingValue = CONFIG_SETTING_OFF;
    } else {
        MIDSetMainDisplayText(context, "HFP: On", 0);
        context->settingValue = CONFIG_SETTING_ON;
    }
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_PREV_VAL, "<   ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_NEXT_VAL, "   >");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_R, "   ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_L, "   ");
    context->settingIdx = MID_SETTING_IDX_HFP;
    context->settingMode = MID_SETTING_MODE_SCROLL_SETTINGS;
}

void MIDBC127MetadataUpdate(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE && strlen(context->bt->title) > 0) {
        char text[UTILS_DISPLAY_TEXT_SIZE];
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
        char cleanText[UTILS_DISPLAY_TEXT_SIZE];
        UtilsRemoveNonAscii(cleanText, text);
        MIDSetMainDisplayText(context, cleanText, 3000 / MID_DISPLAY_SCROLL_SPEED);
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void MIDBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            IBusCommandMIDMenuText(context->ibus, 0, " >");
            BC127CommandGetMetadata(context->bt);
        } else {
            MIDSetMainDisplayText(context, "Paused", 0);
            IBusCommandMIDMenuText(context->ibus, 0, "|| ");
        }
    }
}

void MIDIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    unsigned char requestedCommand = pkt[4];
    if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        IBusCommandMIDDisplayText(context->ibus, "                    ");
        // Stop Playing
        context->mode = MID_MODE_OFF;
    } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING) {
        // Start Playing
        if (context->mode == MID_MODE_OFF) {
            MIDMenuMain(context);
        }
    } else {
        context->displayUpdate = MID_DISPLAY_REWRITE;
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
                IBusCommandMIDMenuText(context->ibus, 0, "|| ");
            } else {
                BC127CommandPlay(context->bt);
                IBusCommandMIDMenuText(context->ibus, 0, ">  ");
            }
        } else if (btnPressed == MID_BUTTON_META) {
            // Temp fix to write out the UI
            if (context->mode == MID_MODE_ACTIVE) {
                MIDMenuMain(context);
            } else if (context->mode == MID_MODE_SETTINGS) {
                MIDMenuSettings(context);
            } else if (context->mode == MID_MODE_DEVICES) {
                MIDMenuDevices(context);
            }
        } else if (btnPressed == 2 || btnPressed == 3) {
            MIDMenuSettings(context);
        }  else if (btnPressed == 4 || btnPressed == 5) {
            MIDMenuDevices(context);
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
            if (strlen(context->bt->title) > 0) {
                MIDBC127MetadataUpdate((void *) context, 0x00);
            } else {
                MIDSetMainDisplayText(context, "                    ", 0);
            }
            MIDMenuMain(context);
        } else if (btnPressed == MID_BUTTON_EDIT_SAVE) {
            if (context->settingMode == MID_SETTING_MODE_SCROLL_SETTINGS) {
                IBusCommandMIDMenuText(context->ibus, MID_BUTTON_EDIT_SAVE, "Save");
                context->settingMode = MID_SETTING_MODE_SCROLL_VALUES;
            } else if (context->settingMode == MID_SETTING_MODE_SCROLL_VALUES) {
                IBusCommandMIDMenuText(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
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
            }
        }  else if (btnPressed == MID_BUTTON_PREV_VAL ||
                   btnPressed == MID_BUTTON_NEXT_VAL
        ) {
            if (context->settingMode == MID_SETTING_MODE_SCROLL_SETTINGS) {
                MIDShowNextSetting(context, btnPressed);
            } else if (context->settingMode == MID_SETTING_MODE_SCROLL_VALUES) {
                MIDShowNextSettingValue(context, btnPressed);
            }
        }
    } else if (context->mode == MID_MODE_DEVICES) {
        if (btnPressed == MID_BUTTON_BACK) {
            if (strlen(context->bt->title) > 0) {
                MIDBC127MetadataUpdate((void *) context, 0x00);
            } else {
                MIDSetMainDisplayText(context, "                    ", 0);
            }
            MIDMenuMain(context);
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
}

/**
 * MIDIIBusRADMIDDisplayUpdate()
 *     Description:
 *         Handle the RAD writing to the MID.
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void MIDIIBusRADMIDDisplayUpdate(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    unsigned char watermark = pkt[pkt[IBUS_PKT_LEN]];
    if (watermark != IBUS_RAD_MAIN_AREA_WATERMARK) {
        if (context->mode == MID_MODE_ACTIVE) {
            IBusCommandMIDDisplayTitleText(context->ibus, "Bluetooth");
        } else if (context->mode == MID_MODE_DISPLAY_OFF) {
            context->mode = MID_MODE_ACTIVE;        
        } else if (context->mode == MID_MODE_SETTINGS) {
            IBusCommandMIDDisplayTitleText(context->ibus, "Settings");
        } else if (context->mode == MID_MODE_DEVICES) {
            IBusCommandMIDDisplayTitleText(context->ibus, "Devices");
        }
    }
}

/**
 * MIDIIBusRADMIDDisplayUpdate()
 *     Description:
 *         Handle the RAD writing to the MID menu. Refresh on the second
 *         message, if the menu update correlates to the SCAN/RANDOM buttons
 *         being pressed on the MID.
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void MIDIIBusRADMIDMenuUpdate(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    // Ensure that we're only looking at updates sent by the radio
    // which writes the menu using a different command and always has
    // a long length
    if ((pkt[4] == 0x00 || pkt[4] == 0xC0) &&
        pkt[IBUS_PKT_LEN] > 0x09 &&
        context->mode != MID_MODE_OFF
    ) {
        if (context->displayUpdate == MID_DISPLAY_REWRITE) {
            context->displayUpdate = MID_DISPLAY_REWRITE_NEXT;
        } else if (context->displayUpdate == MID_DISPLAY_REWRITE_NEXT) {
            context->displayUpdate = MID_DISPLAY_NONE;
            if (context->mode == MID_MODE_ACTIVE ||
                context->mode == MID_MODE_SETTINGS ||
                context->mode == MID_MODE_DEVICES
            ) {
                TimerTriggerScheduledTask(context->displayUpdateTaskId);
            }
        } else {
            context->displayUpdate = MID_DISPLAY_REWRITE_NEXT;
        }
    }
}

/**
 * MIDIBusMIDModeChange()
 *     Description:
 *         
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *pkt - The IBus packet
 *     Returns:
 *         void
 */
void MIDIBusMIDModeChange(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (pkt[4] == 0x08) {
        if (pkt[5] == 0xB0) {
            MIDSetMainDisplayText(context, "", 0);
        } else {
            context->mode = MID_MODE_OFF;
        }
    } else if (pkt[4] == 0x01) {
        if (context->mode == MID_MODE_OFF) {
            context->mode = MID_MODE_ACTIVE;
        } else {
            context->mode = MID_MODE_DISPLAY_OFF;
        }
    }
}

void MIDTimerDisplay(void *ctx)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode != MID_MODE_OFF) {
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
                    char text[MID_DISPLAY_TEXT_SIZE + 1];
                    strncpy(
                        text,
                        &context->mainDisplay.text[context->mainDisplay.index],
                        MID_DISPLAY_TEXT_SIZE
                    );
                    text[MID_DISPLAY_TEXT_SIZE] = '\0';
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
