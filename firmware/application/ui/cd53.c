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
    CD53_SETTING_IDX_VEH_TYPE,
    CD53_SETTING_IDX_BLINKERS,
    CD53_SETTING_IDX_COMFORT_LOCKS,
    CD53_SETTING_IDX_TCU_MODE,
    CD53_SETTING_IDX_PAIRINGS,
};

uint8_t SETTINGS_TO_MENU[] = {
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY,
    CONFIG_VEHICLE_TYPE_ADDRESS,
    CONFIG_SETTING_COMFORT_BLINKERS,
    CONFIG_SETTING_COMFORT_LOCKS,
    CONFIG_SETTING_TCU_MODE
};

void CD53Init(BC127_t *bt, IBus_t *ibus)
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
    EventRegisterCallback(
        BC127Event_Boot,
        &CD53BC127DeviceReady,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceDisconnected,
        &CD53BC127DeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_MetadataChange,
        &CD53BC127Metadata,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &CD53BC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &CD53BC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_BMBTButton,
        &CD53IBusBMBTButtonPress,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDStatusRequest,
        &CD53IBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_RADUpdateMainArea,
        &CD53IBusRADUpdateMainArea,
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
        BC127Event_Boot,
        &CD53BC127DeviceReady
    );
    EventUnregisterCallback(
        BC127Event_DeviceDisconnected,
        &CD53BC127DeviceDisconnected
    );
    EventUnregisterCallback(
        BC127Event_MetadataChange,
        &CD53BC127Metadata
    );
    EventUnregisterCallback(
        BC127Event_PlaybackStatusChange,
        &CD53BC127PlaybackStatus
    );
    EventUnregisterCallback(
        BC127Event_PlaybackStatusChange,
        &CD53BC127PlaybackStatus
    );
    EventUnregisterCallback(
        IBusEvent_BMBTButton,
        &CD53IBusBMBTButtonPress
    );
    EventUnregisterCallback(
        IBusEvent_CDStatusRequest,
        &CD53IBusCDChangerStatus
    );
    EventUnregisterCallback(
        IBusEvent_RADUpdateMainArea,
        &CD53IBusRADUpdateMainArea
    );
    TimerUnregisterScheduledTask(&CD53TimerDisplay);
    memset(&Context, 0, sizeof(CD53Context_t));
}

static void CD53SetMainDisplayText(
    CD53Context_t *context,
    const char *str,
    int8_t timeout
) {
    strncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE - 1);
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
    strncpy(context->tempDisplay.text, str, CD53_DISPLAY_TEMP_TEXT_SIZE - 1);
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
    BC127PairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
    char text[12];
    UtilsRemoveNonAscii(text, dev->deviceName);
    char cleanText[12];
    strncpy(cleanText, text, 11);
    text[11] = '\0';
    // Add a space and asterisks to the end of the device name
    // if it's the currently selected device
    if (strcmp(dev->macId, context->bt->activeDevice.macId) == 0) {
        uint8_t startIdx = strlen(cleanText);
        if (startIdx > 9) {
            startIdx = 9;
        }
        cleanText[startIdx++] = 0x20;
        cleanText[startIdx++] = 0x2A;
    }
    CD53SetTempDisplayText(context, cleanText, -1);
}

static void CD53HandleUIButtons(CD53Context_t *context, unsigned char *pkt)
{
    unsigned char requestedCommand = pkt[4];
    if (requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK) {
        unsigned char direction = pkt[5];
        if (context->mode == CD53_MODE_ACTIVE) {
            // No special menu, so act as next/previous track commands
            char displayText[2];
            displayText[1] = '\0';
            if (pkt[5] == 0x00) {
                displayText[0] = IBusMIDSymbolNext;
                BC127CommandForward(context->bt);
            } else {
                displayText[0] = IBusMIDSymbolBack;
                BC127CommandBackward(context->bt);
                // We need to ask for the metadata of the playing song again
                // to clear the screen
                BC127CommandGetMetadata(context->bt);
            }
            CD53SetMainDisplayText(context, displayText, 0);
        } else if (context->mode == CD53_MODE_DEVICE_SEL) {
            CD53ShowNextAvailableDevice(context, direction);
        } else if (context->mode == CD53_MODE_SETTINGS &&
                   context->settingMode == CD53_SETTING_MODE_SCROLL_SETTINGS
        ) {
            uint8_t nextMenu = 0;
            if (context->settingIdx == CD53_SETTING_IDX_HFP && direction != 0x00) {
                nextMenu = SETTINGS_MENU[CD53_SETTING_IDX_PAIRINGS];
            } else if(context->settingIdx == CD53_SETTING_IDX_PAIRINGS && direction == 0x00) {
                nextMenu = SETTINGS_MENU[CD53_SETTING_IDX_HFP];
            } else {
                if (direction == 0x00) {
                    nextMenu = SETTINGS_MENU[context->settingIdx + 1];
                } else {
                    nextMenu = SETTINGS_MENU[context->settingIdx - 1];
                }
            }
            if (nextMenu == CD53_SETTING_IDX_HFP) {
                if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
                    CD53SetMainDisplayText(context, "Handsfree: 0", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                } else {
                    CD53SetMainDisplayText(context, "Handsfree: 1", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                }
                context->settingIdx = CD53_SETTING_IDX_HFP;
            }
            if (nextMenu == CD53_SETTING_IDX_METADATA_MODE) {
                unsigned char value = ConfigGetSetting(
                    CONFIG_SETTING_METADATA_MODE
                );
                if (value == CD53_METADATA_MODE_PARTY) {
                    CD53SetMainDisplayText(context, "Meta: Party", 0);
                } else if (value == CD53_METADATA_MODE_CHUNK) {
                    CD53SetMainDisplayText(context, "Meta: Chunk", 0);
                } else {
                    CD53SetMainDisplayText(context, "Meta: Party", 0);
                    value = CD53_METADATA_MODE_PARTY;
                }
                context->settingIdx = CD53_SETTING_IDX_METADATA_MODE;
                context->settingValue = value;
            }
            if (nextMenu == CD53_SETTING_IDX_AUTOPLAY) {
                if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == 0x00) {
                    CD53SetMainDisplayText(context, "Autoplay: 0", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                } else {
                    CD53SetMainDisplayText(context, "Autoplay: 1", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                }
                context->settingIdx = CD53_SETTING_IDX_AUTOPLAY;
            }
            if (nextMenu == CD53_SETTING_IDX_VEH_TYPE) {
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
            if (nextMenu == CD53_SETTING_IDX_BLINKERS) {
                unsigned char blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
                if (blinkCount == 0x03) {
                    CD53SetMainDisplayText(context, "OT Blink: 3", 0);
                    context->settingValue = 0x03;
                } else if (blinkCount == 0x05) {
                    CD53SetMainDisplayText(context, "OT Blink: 5", 0);
                    context->settingValue = 0x05;
                } else {
                    CD53SetMainDisplayText(context, "OT Blink: 1", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
                context->settingIdx = CD53_SETTING_IDX_BLINKERS;
            }
            if (nextMenu == CD53_SETTING_IDX_COMFORT_LOCKS) {
                if (ConfigGetSetting(CONFIG_SETTING_COMFORT_LOCKS) == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "Comfort Locks: 0", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                } else {
                    CD53SetMainDisplayText(context, "Comfort Locks: 1", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                }
                context->settingIdx = CD53_SETTING_IDX_COMFORT_LOCKS;
            }
            if (nextMenu == CD53_SETTING_IDX_TCU_MODE) {
                context->settingValue = ConfigGetSetting(CONFIG_SETTING_TCU_MODE);
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "TCU Mode: Always", 0);
                } else {
                    CD53SetMainDisplayText(context, "TCU Mode: Out of BT", 0);
                }
                context->settingIdx = CD53_SETTING_IDX_TCU_MODE;
            }
            if (nextMenu== CD53_SETTING_IDX_PAIRINGS) {
                CD53SetMainDisplayText(context, "Clear Pairings", 0);
                context->settingIdx = CD53_SETTING_IDX_PAIRINGS;
                context->settingValue = CONFIG_SETTING_OFF;
            }
        } else if(context->mode == CD53_MODE_SETTINGS &&
                  context->settingMode == CD53_SETTING_MODE_SCROLL_VALUES
        ) {
            // Select different configuration options
            if (context->settingIdx == CD53_SETTING_IDX_HFP) {
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "Handsfree: 1", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                } else {
                    CD53SetMainDisplayText(context, "Handsfree: 0", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_METADATA_MODE) {
                if (context->settingValue == CD53_METADATA_MODE_CHUNK) {
                    CD53SetMainDisplayText(context, "Meta: Party", 0);
                    context->settingValue = CD53_METADATA_MODE_PARTY;
                } else if (context->settingValue == CD53_METADATA_MODE_PARTY) {
                    CD53SetMainDisplayText(context, "Meta: Chunk", 0);
                    context->settingValue = CD53_METADATA_MODE_CHUNK;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_AUTOPLAY) {
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "Autoplay: 1", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                } else {
                    CD53SetMainDisplayText(context, "Autoplay: 0", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_VEH_TYPE) {
                if (context->settingValue == 0x00 ||
                    context->settingValue == 0xFF ||
                    context->settingValue == IBUS_VEHICLE_TYPE_E46_Z4
                ) {
                    CD53SetMainDisplayText(context, "Car: E38/E39/E53", 0);
                    context->settingValue = IBUS_VEHICLE_TYPE_E38_E39_E53;
                } else {
                    CD53SetMainDisplayText(context, "Car: E46/Z4", 0);
                    context->settingValue = IBUS_VEHICLE_TYPE_E46_Z4;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_BLINKERS) {
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "OT Blink: 3", 0);
                    context->settingValue = 0x03;
                } else if (context->settingValue == 0x03) {
                    CD53SetMainDisplayText(context, "OT Blink: 5", 0);
                    context->settingValue = 0x05;
                } else {
                    CD53SetMainDisplayText(context, "OT Blink: 1", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_COMFORT_LOCKS) {
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "Comfort Locks: 1", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                } else {
                    CD53SetMainDisplayText(context, "Comfort Locks: 0", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_TCU_MODE) {
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "TCU Mode: Out of BT", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                } else {
                    CD53SetMainDisplayText(context, "TCU Mode: Always", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
            }
            if (context->settingIdx == CD53_SETTING_IDX_PAIRINGS) {
                if (context->settingValue == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "Press Ok", 0);
                    context->settingValue = CONFIG_SETTING_ON;
                } else {
                    CD53SetMainDisplayText(context, "Clear Pairings", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                }
            }
        }
    }
    if (pkt[5] == 0x01) {
        if (context->mode == CD53_MODE_ACTIVE) {
            if (context->bt->activeDevice.deviceId != 0) {
                if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                    // Set the display to paused so it doesn't flash back to the
                    // Now playing data
                    CD53SetMainDisplayText(context, "Paused", 0);
                    BC127CommandPause(context->bt);
                } else {
                    BC127CommandPlay(context->bt);
                }
            } else {
                CD53SetTempDisplayText(context, "No Device", 4);
                CD53SetMainDisplayText(context, "Bluetooth", 0);
            }
        } else {
            CD53RedisplayText(context);
        }
    } else if (pkt[5] == 0x02) {
        if (context->mode == CD53_MODE_ACTIVE) {
            // Toggle Metadata scrolling
            if (context->displayMetadata == CD53_DISPLAY_METADATA_ON) {
                CD53SetMainDisplayText(context, "Bluetooth", 0);
                context->displayMetadata = CD53_DISPLAY_METADATA_OFF;
            } else {
                context->displayMetadata = CD53_DISPLAY_METADATA_ON;
                // We are sending a null pointer because we don't even need
                // the second parameter
                CD53BC127Metadata(context, 0x00);
            }
        } else if (context->mode == CD53_MODE_SETTINGS) {
            // Use as "Okay" button
            if (context->settingMode == CD53_SETTING_MODE_SCROLL_SETTINGS) {
                if (context->settingIdx != CD53_SETTING_IDX_PAIRINGS) {
                    CD53SetTempDisplayText(context, "Edit", 1);
                }
                context->settingMode = CD53_SETTING_MODE_SCROLL_VALUES;
            } else {
                context->settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
                if (context->settingIdx == CD53_SETTING_IDX_PAIRINGS) {
                    if (context->settingValue == CONFIG_SETTING_ON) {
                        BC127CommandUnpair(context->bt);
                        CD53SetTempDisplayText(context, "Unpaired", 1);
                    }
                } else if (context->settingIdx == CD53_SETTING_IDX_VEH_TYPE) {
                    ConfigSetVehicleType(context->settingValue);
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
            }
            CD53RedisplayText(context);
        } else if (context->mode == CD53_MODE_DEVICE_SEL) {
            BC127PairedDevice_t *dev = &context->bt->pairedDevices[
                context->btDeviceIndex
            ];
            // Do nothing if the user selected the active device
            if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0) {
                // Immediately connect if there isn't an active device
                if (context->bt->activeDevice.deviceId == 0) {
                    // Trigger device selection event
                    EventTriggerCallback(
                        UIEvent_InitiateConnection,
                        (unsigned char *)&context->btDeviceIndex
                    );
                    CD53SetTempDisplayText(context, "Connecting", 2);
                }
            } else {
                CD53SetTempDisplayText(context, "Connected", 2);
            }
            CD53RedisplayText(context);
        }
    } else if (pkt[3] == 0x03) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            uint32_t now = TimerGetMillis();
            if (context->bt->callStatus == BC127_CALL_ACTIVE) {
                BC127CommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_INCOMING) {
                BC127CommandCallAnswer(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_OUTGOING) {
                BC127CommandCallEnd(context->bt);
            }
            if ((now - context->lastTelephoneButtonPress) <= CD53_VR_TOGGLE_TIME &&
                context->bt->callStatus == BC127_CALL_INACTIVE
            ) {
                BC127CommandToggleVR(context->bt);
            }
        }
        context->lastTelephoneButtonPress = TimerGetMillis();
        CD53RedisplayText(context);
    } else if (pkt[5] == 0x04) {
        // Settings Menu
        if (context->mode != CD53_MODE_SETTINGS) {
            CD53SetTempDisplayText(context, "Settings", 2);
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "Handsfree: 0", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            } else {
                CD53SetMainDisplayText(context, "Handsfree: 1", 0);
                context->settingValue = CONFIG_SETTING_ON;
            }
            context->settingIdx = CD53_SETTING_IDX_HFP;
            context->mode = CD53_MODE_SETTINGS;
            context->settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
        } else {
            context->mode = CD53_MODE_ACTIVE;
            CD53SetMainDisplayText(context, "Bluetooth", 0);
            if (context->displayMetadata != CD53_DISPLAY_METADATA_OFF) {
                CD53BC127Metadata(context, 0x00);
            }
        }
    } else if (pkt[5] == 0x05) {
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
                CD53BC127Metadata(context, 0x00);
            }
        }
    } else if (pkt[5] == 0x06) {
        // Toggle the discoverable state
        uint8_t state;
        int8_t timeout = 1500 / CD53_DISPLAY_SCROLL_SPEED;
        if (context->bt->discoverable == BC127_STATE_ON) {
            CD53SetTempDisplayText(context, "Pairing Off", timeout);
            state = BC127_STATE_OFF;
        } else {
            CD53SetTempDisplayText(context, "Pairing On", timeout);
            state = BC127_STATE_ON;
            if (context->bt->activeDevice.deviceId != 0) {
                // To pair a new device, we must disconnect the active one
                EventTriggerCallback(UIEvent_CloseConnection, 0x00);
            }
        }
        BC127CommandBtState(context->bt, context->bt->connectable, state);
    } else {
        // A button was pressed - Push our display text back
        if (context->mode == CD53_MODE_ACTIVE) {
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
        } else if (context->mode != CD53_MODE_OFF) {
            CD53RedisplayText(context);
        }
    }
}

void CD53BC127DeviceDisconnected(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode == CD53_MODE_ACTIVE) {
        CD53SetMainDisplayText(context, "Bluetooth", 0);
    }
}

void CD53BC127DeviceReady(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // The BT Device Reset -- Clear the Display
    context->mainDisplay = UtilsDisplayValueInit("", CD53_DISPLAY_STATUS_OFF);
    // If we're in Bluetooth mode, display our banner
    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING) {
        CD53SetMainDisplayText(context, "Bluetooth", 0);
    }
}

void CD53BC127Metadata(CD53Context_t *context, unsigned char *metadata)
{
    if (context->displayMetadata &&
        context->mode == CD53_MODE_ACTIVE &&
        strlen(context->bt->title) > 0
    ) {
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
        CD53SetMainDisplayText(context, cleanText, 3000 / CD53_DISPLAY_SCROLL_SPEED);
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void CD53BC127PlaybackStatus(void *ctx, unsigned char *status)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // Display "Paused" if we're in Bluetooth mode
    if (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING &&
        context->mode == CD53_MODE_ACTIVE &&
        context->displayMetadata
    ) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
            CD53SetMainDisplayText(context, "Paused", 0);
        } else {
            CD53SetMainDisplayText(context, "Playing", 0);
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
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_PlayPause) {
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                BC127CommandPlay(context->bt);
            }
        }
    }
}

void CD53IBusClearScreen(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->mode == CD53_MODE_ACTIVE) {
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    } else if (context->mode != CD53_MODE_OFF) {
        CD53RedisplayText(context);
    }
}

void CD53IBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    unsigned char requestedCommand = pkt[4];
    uint8_t btPlaybackStatus = context->bt->playbackStatus;
    if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        // Stop Playing
        IBusCommandIKETextClear(context->ibus);
        context->mode = CD53_MODE_OFF;
    } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING) {
        // Start Playing
        if (context->mode == CD53_MODE_OFF) {
            CD53SetMainDisplayText(context, "Bluetooth", 0);
            if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON) {
                BC127CommandPlay(context->bt);
            } else if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            }
            BC127CommandStatus(context->bt);
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
        requestedCommand == IBUS_CDC_CMD_CHANGE_TRACK
    ) {
        CD53HandleUIButtons(context, pkt);
    }
}

void CD53IBusRADUpdateMainArea(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (pkt[4] == 0xC4) {
        context->radioType = IBus_UI_BUSINESS_NAV;
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
                if (context->radioType == IBus_UI_CD53) {
                    IBusCommandIKEText(
                        context->ibus,
                        context->tempDisplay.text
                    );
                } else if (context->radioType == IBus_UI_BUSINESS_NAV) {
                    IBusCommandGTWriteBusinessNavTitle(context->ibus, context->tempDisplay.text);
                }
                context->tempDisplay.status = CD53_DISPLAY_STATUS_ON;
            }
            if (context->mainDisplay.length <= 11) {
                context->mainDisplay.index = 0;
            }
        } else {
            // Display the main text if there isn't a timeout set
            if (context->mainDisplay.timeout > 0) {
                context->mainDisplay.timeout--;
            } else {
                if (context->mainDisplay.length > 11) {
                    char text[12];
                    strncpy(
                        text,
                        &context->mainDisplay.text[context->mainDisplay.index],
                        11
                    );
                    text[11] = '\0';
                    if (context->radioType == IBus_UI_CD53) {
                        IBusCommandIKEText(context->ibus, text);
                    } else if (context->radioType == IBus_UI_BUSINESS_NAV) {
                        IBusCommandGTWriteBusinessNavTitle(context->ibus, text);
                    }
                    // Pause at the beginning of the text
                    if (context->mainDisplay.index == 0) {
                        context->mainDisplay.timeout = 5;
                    }
                    uint8_t idxEnd = context->mainDisplay.index + 11;
                    if (idxEnd >= context->mainDisplay.length) {
                        // Pause at the end of the text
                        context->mainDisplay.timeout = 2;
                        context->mainDisplay.index = 0;
                    } else {
                        if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) ==
                            CD53_METADATA_MODE_CHUNK
                        ) {
                            context->mainDisplay.timeout = 2;
                            context->mainDisplay.index += 11;
                        } else {
                            context->mainDisplay.index++;
                        }
                    }
                } else {
                    if (context->mainDisplay.index == 0) {
                        if (context->radioType == IBus_UI_CD53) {
                            IBusCommandIKEText(context->ibus, context->mainDisplay.text);
                        } else if (context->radioType == IBus_UI_BUSINESS_NAV) {
                            IBusCommandGTWriteBusinessNavTitle(context->ibus, context->mainDisplay.text);
                        }
                    }
                    context->mainDisplay.index = 1;
                }
            }
        }
    }
}
