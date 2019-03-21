/*
 * File: cd53.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the CD53 UI Mode handler
 */
#include "cd53.h"
static CD53Context_t Context;

uint8_t SETTINGS_MENU[4] = {
    CD53_SETTING_IDX_HFP,
    CD53_SETTING_IDX_METADATA_MODE,
    CD53_SETTING_IDX_AUTOPLAY,
    CD53_SETTING_IDX_PAIRINGS,
};

uint8_t SETTINGS_TO_MENU[3] = {
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY
};

void CD53Init(BC127_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.mode = CD53_MODE_OFF;
    Context.mainDisplay = CD53DisplayValueInit("Bluetooth");
    Context.tempDisplay = CD53DisplayValueInit("");
    Context.btDeviceIndex = CD53_PAIRING_DEVICE_NONE;
    Context.seekMode = CD53_SEEK_MODE_NONE;
    Context.displayMetadata = CD53_DISPLAY_METADATA_ON;
    Context.settingIdx = CD53_SETTING_IDX_HFP;
    Context.settingValue = CONFIG_SETTING_OFF;
    Context.settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
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
        IBusEvent_CDClearDisplay,
        &CD53IBusClearScreen,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDStatusRequest,
        &CD53IBusCDChangerStatus,
        &Context
    );
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &CD53TimerDisplay,
        &Context,
        CD53_DISPLAY_TIMER_INT
    );
}

CD53DisplayValue_t CD53DisplayValueInit(char *text)
{
    CD53DisplayValue_t value;
    strncpy(value.text, text, CD53_DISPLAY_TEXT_SIZE - 1);
    value.index = 0;
    value.timeout = 0;
    value.status = CD53_DISPLAY_STATUS_OFF;
    return value;
}

static void CD53SetMainDisplayText(
    CD53Context_t *context,
    const char *str,
    int8_t timeout
) {
    strncpy(context->mainDisplay.text, str, CD53_DISPLAY_TEXT_SIZE - 1);
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
    strncpy(context->tempDisplay.text, str, CD53_DISPLAY_TEMP_TEXT_SIZE);
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
    if (context->btDeviceIndex == CD53_PAIRING_DEVICE_NONE) {
        context->btDeviceIndex = 0;
    }
    if (direction == 0x00) {
        if (context->btDeviceIndex < context->bt->pairedDevicesCount) {
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
    removeNonAscii(text, dev->deviceName);
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
    unsigned char changerStatus = pkt[4];
    if (changerStatus == IBusAction_CD53_SEEK) {
        unsigned char direction = pkt[5];
        if (context->mode == CD53_MODE_ACTIVE) {
            // No special menu, so act as next/previous track buttons
            char displayText[2];
            displayText[1] = '\0';
            if (pkt[5] == 0x00) {
                BC127CommandForward(context->bt);
                displayText[0] = IBusMIDSymbolNext;
            } else {
                BC127CommandBackward(context->bt);
                displayText[0] = IBusMIDSymbolBack;
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
                nextMenu = SETTINGS_MENU[3];
            } else if(context->settingIdx == CD53_SETTING_IDX_PAIRINGS && direction == 0x00) {
                nextMenu = SETTINGS_MENU[0];
            } else {
                if (direction == 0x00) {
                    nextMenu = SETTINGS_MENU[context->settingIdx + 1];
                } else {
                    nextMenu = SETTINGS_MENU[context->settingIdx - 1];
                }
            }
            if (nextMenu == CD53_SETTING_IDX_HFP) {
                if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
                    CD53SetMainDisplayText(context, "HFP: 0", 0);
                    context->settingValue = 0;
                } else {
                    CD53SetMainDisplayText(context, "HFP: 1", 0);
                    context->settingValue = 1;
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
                    context->settingValue = 0;
                } else {
                    CD53SetMainDisplayText(context, "Autoplay: 1", 0);
                    context->settingValue = 1;
                }
                context->settingIdx = CD53_SETTING_IDX_AUTOPLAY;
            }
            if (nextMenu== CD53_SETTING_IDX_PAIRINGS) {
                CD53SetMainDisplayText(context, "Clear Pairs", 0);
                context->settingIdx = CD53_SETTING_IDX_PAIRINGS;
                context->settingValue = 0;
            }
        } else if(context->mode == CD53_MODE_SETTINGS &&
                  context->settingMode == CD53_SETTING_MODE_SCROLL_VALUES
        ) {
            // Select different configuration options
            if (context->settingIdx == CD53_SETTING_IDX_HFP) {
                if (context->settingValue  == CONFIG_SETTING_OFF) {
                    CD53SetMainDisplayText(context, "HFP: 1", 0);
                    context->settingValue = CONFIG_SETTING_OFF;
                } else {
                    CD53SetMainDisplayText(context, "HFP: 0", 0);
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
            if (context->settingIdx == CD53_SETTING_IDX_PAIRINGS) {
                if (context->settingValue == 0) {
                    CD53SetMainDisplayText(context, "Press Ok", 0);
                    context->settingValue = 1;
                } else {
                    CD53SetMainDisplayText(context, "Clear Pairs", 0);
                    context->settingValue = 1;
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
        } else if (context->mode == CD53_MODE_DEVICE_SEL) {
            BC127PairedDevice_t *dev = &context->bt->pairedDevices[
                context->btDeviceIndex
            ];
            // Do nothing if the user selected the active device
            if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0) {
                // Immediately connect if there isn't an active device
                if (context->bt->activeDevice.deviceId == 0) {
                    BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
                } else {
                    // Close the current connection then open the new one
                    BC127CommandClose(
                        context->bt,
                        context->bt->activeDevice.deviceId
                    );
                    CD53SetTempDisplayText(context, "Connecting", 2);
                }
            } else {
                CD53SetTempDisplayText(context, "Connected", 2);
            }
            context->mode = CD53_MODE_ACTIVE;
        }
    } else if (pkt[5] == 0x02) {
        if (context->mode == CD53_MODE_ACTIVE) {
            // Toggle Metadata scrolling
            if (context->displayMetadata == CD53_DISPLAY_METADATA_ON) {
                CD53SetMainDisplayText(context, "Bluetooth", 0);
                context->displayMetadata = CD53_DISPLAY_METADATA_OFF;
            } else {
                CD53SetTempDisplayText(context, "Metadata On", 2);
                context->displayMetadata = CD53_DISPLAY_METADATA_ON;
                // We are sending a null pointer because we don't even need
                // the second parameter
                CD53BC127Metadata(context, 0x00);
            }
        } else if (context->mode == CD53_MODE_SETTINGS) {
            // Use as "Okay" button
            if (context->settingMode == CD53_SETTING_MODE_SCROLL_SETTINGS) {
                context->settingMode = CD53_SETTING_MODE_SCROLL_VALUES;
            } else {
                context->settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
                if (context->settingIdx != CD53_SETTING_IDX_PAIRINGS) {
                    ConfigSetSetting(
                        SETTINGS_TO_MENU[context->settingIdx],
                        context->settingValue
                    );
                    if (context->settingIdx == CD53_SETTING_IDX_HFP) {
                        BC127CommandSetProfiles(context->bt, 1, 1, 0, 0);
                        BC127CommandReset(context->bt);
                    }
                } else {
                    if (context->settingValue == 1) {
                        BC127CommandUnpair(context->bt);
                    }
                }
            }
            CD53RedisplayText(context);
        }
    } else if (pkt[5] == 0x04) {
        // Settings Menu
        if (context->mode != CD53_MODE_SETTINGS) {
            CD53SetTempDisplayText(context, "Settings", 2);
            if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
                CD53SetMainDisplayText(context, "HFP: 0", 0);
                context->settingValue = CONFIG_SETTING_OFF;
            } else {
                CD53SetMainDisplayText(context, "HFP: 1", 0);
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
            CD53SetTempDisplayText(context, "Devices", 2);
            if (context->bt->pairedDevicesCount == 0) {
                CD53SetTempDisplayText(context, "No Device", 4);
            } else {
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
                BC127CommandClose(
                    context->bt,
                    context->bt->activeDevice.deviceId
                );
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
    if (context->btDeviceIndex != CD53_PAIRING_DEVICE_NONE) {
        BC127PairedDevice_t *dev = &context->bt->pairedDevices[
            context->btDeviceIndex
        ];
        if (strlen(dev->macId) > 0) {
            BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
        }
        context->btDeviceIndex = CD53_PAIRING_DEVICE_NONE;
    }
}

void CD53BC127DeviceReady(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // The BT Device Reset -- Clear the Display
    context->mainDisplay = CD53DisplayValueInit("");
    // If we're in Bluetooth mode, display our banner
    if (context->ibus->cdChangerStatus > 0x01) {
        CD53SetMainDisplayText(context, "Bluetooth", 0);
    }
}

void CD53BC127Metadata(CD53Context_t *context, unsigned char *metadata)
{
    if (context->displayMetadata && context->mode == CD53_MODE_ACTIVE) {
        char text[CD53_DISPLAY_TEXT_SIZE];
        snprintf(
            text,
            CD53_DISPLAY_TEXT_SIZE,
            "%s - %s on %s",
            context->bt->title,
            context->bt->artist,
            context->bt->album
        );
        char cleanText[CD53_DISPLAY_TEXT_SIZE];
        removeNonAscii(cleanText, text);
        CD53SetMainDisplayText(context, cleanText, 3000 / CD53_DISPLAY_SCROLL_SPEED);
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void CD53BC127PlaybackStatus(void *ctx, unsigned char *status)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // Display "Paused" if we're in Bluetooth mode
    if (context->ibus->cdChangerStatus > 0x01) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
            CD53SetMainDisplayText(context, "Paused", 0);
        } else {
            CD53SetMainDisplayText(context, "Playing", 0);
        }
    }
}

void CD53IBusClearScreen(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
        // If we're displaying temp text, we need to override the screen
        // state again, since it will now be clear
        if (context->tempDisplay.status == CD53_DISPLAY_STATUS_ON) {
            context->tempDisplay.status = CD53_DISPLAY_STATUS_NEW;
        }
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void CD53IBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    unsigned char changerStatus = pkt[4];
    uint8_t btPlaybackStatus = context->bt->playbackStatus;
    if (changerStatus == 0x01) {
        // Stop Playing
        IBusCommandMIDTextClear(context->ibus);
        context->mode = CD53_MODE_OFF;
    } else if (changerStatus == 0x03) {
        // Start Playing
        if (context->mode == CD53_MODE_OFF) {
            if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                CD53SetMainDisplayText(context, "Bluetooth", 0);
            }
            BC127CommandStatus(context->bt);
            context->mode = CD53_MODE_ACTIVE;
        } else {
            if (context->seekMode == CD53_SEEK_MODE_FWD) {
                BC127CommandForwardSeekRelease(context->bt);
            } else if (context->seekMode == CD53_SEEK_MODE_REV) {
                BC127CommandBackwardSeekRelease(context->bt);
            }
            context->seekMode = CD53_SEEK_MODE_NONE;
        }
    } else if (changerStatus == 0x04) {
        if (pkt[5] == 0x00) {
            context->seekMode = CD53_SEEK_MODE_REV;
            BC127CommandBackwardSeekPress(context->bt);
        } else {
            context->seekMode = CD53_SEEK_MODE_FWD;
            BC127CommandForwardSeekPress(context->bt);
        }
    } else if (changerStatus == 0x07) {
        if (context->mode == CD53_MODE_ACTIVE) {
            if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
                TimerTriggerScheduledTask(context->displayUpdateTaskId);
            }
        }
    } else if (changerStatus == 0x08) {
        if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
        } else {
            CD53SetMainDisplayText(context, "Bluetooth", 0);
        }
    }
    if (changerStatus == IBusAction_CD53_CD_SEL ||
        changerStatus == IBusAction_CD53_SEEK
    ) {
        CD53HandleUIButtons (context, pkt);
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
                IBusCommandMIDText(
                    context->ibus,
                    context->tempDisplay.text
                );
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
                    IBusCommandMIDText(context->ibus, text);
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
                        IBusCommandMIDText(
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
