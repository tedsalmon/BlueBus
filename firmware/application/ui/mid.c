/*
 * File: mid.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#include "mid.h"
static MIDContext_t Context;

void MIDInit(BT_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.btDeviceIndex = 0;
    Context.mode = MID_MODE_OFF;
    Context.displayUpdate = MID_DISPLAY_NONE;
    Context.mainDisplay = UtilsDisplayValueInit("", MID_DISPLAY_STATUS_OFF);
    Context.tempDisplay = UtilsDisplayValueInit("", MID_DISPLAY_STATUS_OFF);
    Context.modeChangeStatus = MID_MODE_CHANGE_OFF;
    Context.menuContext = MenuSingleLineInit(ibus, bt, &MIDDisplayUpdateText, &Context);
    strncpy(Context.mainText, "Bluetooth", 10);
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &MIDBTDeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_METADATA_UPDATE,
        &MIDBTMetadataUpdate,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &MIDBTPlaybackStatus,
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
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &MIDBTDeviceDisconnected
    );
    EventUnregisterCallback(
        BT_EVENT_METADATA_UPDATE,
        &MIDBTMetadataUpdate
    );
    EventUnregisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &MIDBTPlaybackStatus
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
    memset(context->mainDisplay.text, 0, UTILS_DISPLAY_TEXT_SIZE);
    UtilsStrncpy(context->mainDisplay.text, text, UTILS_DISPLAY_TEXT_SIZE);
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
    UtilsStrncpy(context->tempDisplay.text, text, UTILS_DISPLAY_TEXT_SIZE);
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
        BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
        char text[16];
        strncpy(text, dev->deviceName, 15);
        text[15] = '\0';
        // Add a space and asterisks to the end of the device name
        // if it's the currently selected device
        if (memcmp(dev->macId, context->bt->activeDevice.macId, BT_LEN_MAC_ID) == 0) {
            uint8_t startIdx = strlen(text);
            if (startIdx > 13) {
                startIdx = 13;
            }
            text[startIdx++] = 0x20;
            text[startIdx++] = 0x2A;
            text[startIdx] = 0;
        }
        MIDSetMainDisplayText(context, text, 0);
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
    MIDBTMetadataUpdate((void *) context, 0x00);
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
    if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
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
    MenuSingleLineSettings(&context->menuContext);
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_PREV_VAL, "<   ");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_NEXT_VAL, "   >");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_DEVICES_R, "   ");
    IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_DEVICES_L, "   ");
}

/**
 * MIDDisplayUpdateText()
 *     Description:
 *         Handle updates from the menu driver
 *     Params:
 *         void *ctx - A void pointer to the MIDContext_t struct
 *         char *text - The text to update
 *         int8_t timeout - The timeout for the text
 *         uint8_t updateType - If we are updating the temp or main text
 *     Returns:
 *         void
 */
void MIDDisplayUpdateText(void *ctx, char *text, int8_t timeout, uint8_t updateType)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (updateType == MENU_SINGLELINE_DISPLAY_UPDATE_MAIN) {
        MIDSetMainDisplayText(context, text, timeout);
    } else if (updateType == MENU_SINGLELINE_DISPLAY_UPDATE_TEMP) {
        MIDSetTempDisplayText(context, text, timeout);
    }
}

void MIDBTDeviceDisconnected(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE) {
        MIDSetMainDisplayText(context, "", 0);
    }
}


void MIDBTMetadataUpdate(void *ctx, unsigned char *tmp)
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

void MIDBTPlaybackStatus(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE) {
        if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
            IBusCommandMIDMenuWriteSingle(context->ibus, 0, " >");
            BTCommandGetMetadata(context->bt);
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
            if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                BTCommandPause(context->bt);
                IBusCommandMIDMenuWriteSingle(context->ibus, 0, "|| ");
            } else {
                BTCommandPlay(context->bt);
                IBusCommandMIDMenuWriteSingle(context->ibus, 0, ">  ");
            }
        } else if (btnPressed == MID_BUTTON_META) {
            if (context->mode == MID_MODE_ACTIVE) {
                context->mode = MID_MODE_DISPLAY_OFF;
            } else {
                context->mode = MID_MODE_ACTIVE_NEW;
            }
        } else if (btnPressed == MID_BUTTON_SETTINGS_L ||
                   btnPressed == MID_BUTTON_SETTINGS_R
        ) {
            context->mode = MID_MODE_SETTINGS_NEW;
        }  else if (btnPressed == MID_BUTTON_DEVICES_L ||
                    btnPressed == MID_BUTTON_DEVICES_R
        ) {
            context->mode = MID_MODE_DEVICES_NEW;
        } else if (btnPressed == MID_BUTTON_PAIR) {
            // Toggle the discoverable state
            uint8_t state;
            int8_t timeout = 1500 / MID_DISPLAY_SCROLL_SPEED;
            if (context->bt->discoverable == BT_STATE_ON) {
                MIDSetTempDisplayText(context, "Pairing Off", timeout);
                state = BT_STATE_OFF;
            } else {
                MIDSetTempDisplayText(context, "Pairing On", timeout);
                state = BT_STATE_ON;
                if (context->bt->activeDevice.deviceId != 0) {
                    // To pair a new device, we must disconnect the active one
                    EventTriggerCallback(UIEvent_CloseConnection, 0x00);
                }
            }
            BTCommandSetDiscoverable(context->bt, state);
            context->mode = MID_MODE_DISPLAY_OFF;
        }
    } else if (context->mode == MID_MODE_SETTINGS) {
        if (btnPressed == MID_BUTTON_BACK) {
            context->mode = MID_MODE_ACTIVE_NEW;
        } else if (btnPressed == MID_BUTTON_EDIT_SAVE) {
            if (context->menuContext.settingMode == MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS) {
                IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, "Save");
            } else {
                IBusCommandMIDMenuWriteSingle(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
            }
            MenuSingleLineSettingsEditSave(&context->menuContext);
        }  else if (btnPressed == MID_BUTTON_PREV_VAL ||
                   btnPressed == MID_BUTTON_NEXT_VAL
        ) {
            uint8_t direction = 0x00;
            if (btnPressed == MID_BUTTON_PREV_VAL) {
                direction = 0x01;
            }
            MenuSingleLineSettingsScroll(&context->menuContext, direction);
        }
    } else if (context->mode == MID_MODE_DEVICES) {
        if (btnPressed == MID_BUTTON_BACK) {
            context->mode = MID_MODE_ACTIVE_NEW;
        } else if (btnPressed == MID_BUTTON_EDIT_SAVE) {
            if (context->bt->pairedDevicesCount > 0) {
                // Connect to device
                BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
                if (memcmp(dev->macId, context->bt->activeDevice.macId, BT_LEN_MAC_ID) != 0 &&
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
        IBusCommandMIDButtonPress(context->ibus, IBUS_DEVICE_RAD, MID_BUTTON_MODE_RELEASE);
        context->modeChangeStatus = MID_MODE_CHANGE_OFF;
    }
    // Handle Next and Previous
    if (context->ibus->cdChangerFunction != IBUS_CDC_FUNC_NOT_PLAYING) {
        if (btnPressed == IBus_MID_BTN_TEL_RIGHT_RELEASE) {
            BTCommandPlaybackTrackNext(context->bt);
        } else if (btnPressed == IBus_MID_BTN_TEL_LEFT_RELEASE) {
            BTCommandPlaybackTrackPrevious(context->bt);
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
            IBusCommandMIDButtonPress(context->ibus, IBUS_DEVICE_RAD, MID_BUTTON_MODE_PRESS);
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
            if (context->mainDisplay.length <= IBus_MID_MAX_CHARS) {
                context->mainDisplay.index = 0;
            }
        } else {
            // Display the main text if there isn't a timeout set
            if (context->mainDisplay.timeout > 0) {
                context->mainDisplay.timeout--;
            } else {
                if (context->mainDisplay.length > IBus_MID_MAX_CHARS) {
                    char text[IBus_MID_MAX_CHARS + 1] = {0};
                    uint8_t textLength = IBus_MID_MAX_CHARS;
                    // If we start with a space, it will be ignored by the display
                    // Skipping the space allows us to have "smooth" scrolling
                    if (context->mainDisplay.text[context->mainDisplay.index] == 0x20 &&
                        context->mainDisplay.index < context->mainDisplay.length
                    ) {
                        context->mainDisplay.index++;
                    }
                    uint8_t idxEnd = context->mainDisplay.index + textLength;
                    // Prevent strncpy() from going out of bounds
                    if (idxEnd >= context->mainDisplay.length) {
                        textLength = context->mainDisplay.length - context->mainDisplay.index;
                        idxEnd = context->mainDisplay.index + textLength;
                    }
                    UtilsStrncpy(
                        text,
                        &context->mainDisplay.text[context->mainDisplay.index],
                        textLength + 1
                    );
                    IBusCommandMIDDisplayText(context->ibus, text);
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
                            MID_SETTING_METADATA_MODE_CHUNK
                        ) {
                            context->mainDisplay.timeout = 2;
                            context->mainDisplay.index += IBus_MID_MAX_CHARS;
                        } else {
                            context->mainDisplay.index++;
                        }
                    }
                } else {
                    if (context->mainDisplay.index == 0) {
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
