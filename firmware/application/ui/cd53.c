/*
 * File: cd53.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the CD53 UI Mode handler
 */
#include "cd53.h"
static CD53Context_t Context;

void CD53Init(BT_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.mode = CD53_MODE_OFF;
    Context.mainDisplay = UtilsDisplayValueInit("Bluetooth", CD53_DISPLAY_STATUS_OFF);
    Context.tempDisplay = UtilsDisplayValueInit("", CD53_DISPLAY_STATUS_OFF);
    Context.btDeviceIndex = CD53_PAIRING_DEVICE_NONE;
    Context.displayMetadata = CD53_DISPLAY_METADATA_ON;
    if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF) {
        Context.displayMetadata = CD53_DISPLAY_METADATA_OFF;
    }
    Context.radioType = ConfigGetUIMode();
    Context.mediaChangeState = CD53_MEDIA_STATE_OK;
    Context.menuContext = MenuSingleLineInit(ibus, bt, &CD53DisplayUpdateText, &Context);
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
        IBUS_EVENT_IKEIgnitionStatus,
        &CD53IBusIgnitionStatus,
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
    EventRegisterCallback(
        IBUS_EVENT_ScreenModeSet,
        &CD53GTScreenModeSet,
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
        IBUS_EVENT_IKEIgnitionStatus,
        &CD53IBusIgnitionStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_MFLButton,
        &CD53IBusMFLButton
    );
    EventUnregisterCallback(
        IBUS_EVENT_RAD_WRITE_DISPLAY,
        &CD53IBusRADWriteDisplay
    );
    EventUnregisterCallback(
        IBUS_EVENT_ScreenModeSet,
        &CD53GTScreenModeSet
    );
    TimerUnregisterScheduledTask(&CD53TimerDisplay);
    memset(&Context, 0, sizeof(CD53Context_t));
}

static void CD53SetMainDisplayText(
    CD53Context_t *context,
    const char *str,
    int8_t timeout
) {
    UtilsStrncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE);
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
    UtilsStrncpy(context->tempDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE);
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

/**
 * CD53DisplayUpdateText()
 *     Description:
 *         Handle updates from the menu driver
 *     Params:
 *         void *ctx - A void pointer to the CD53Context_t struct
 *         char *text - The text to update
 *         int8_t timeout - The timeout for the text
 *         uint8_t updateType - If we are updating the temp or main text
 *     Returns:
 *         void
 */
void CD53DisplayUpdateText(void *ctx, char *text, int8_t timeout, uint8_t updateType)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (updateType == MENU_SINGLELINE_DISPLAY_UPDATE_MAIN) {
        CD53SetMainDisplayText(context, text, timeout);
    } else if (updateType == MENU_SINGLELINE_DISPLAY_UPDATE_TEMP) {
        CD53SetTempDisplayText(context, text, timeout);
    }
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
    UtilsStrncpy(text, dev->deviceName, CD53_DISPLAY_TEXT_LEN);
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
    } else if (context->mode == CD53_MODE_SETTINGS) {
        MenuSingleLineSettingsScroll(&context->menuContext, direction);
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
            MenuSingleLineSettingsEditSave(&context->menuContext);
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
        // Do nothing if the display is off
        if (context->mode == CD53_MODE_ACTIVE_DISPLAY_OFF) {
            return;
        }
        // Settings Menu
        if (context->mode != CD53_MODE_SETTINGS) {
            MenuSingleLineSettings(&context->menuContext);
            context->mode = CD53_MODE_SETTINGS;
        } else {
            context->mode = CD53_MODE_ACTIVE;
            CD53SetMainDisplayText(context, "Bluetooth", 0);
            if (context->displayMetadata != CD53_DISPLAY_METADATA_OFF) {
                CD53BTMetadata(context, 0x00);
            }
        }
    } else if (pkt[IBUS_PKT_DB1] == IBUS_CDC_CMD_CD_CHANGE && pkt[IBUS_PKT_DB2] == 0x05) {
        // Do nothing if the display is off
        if (context->mode == CD53_MODE_ACTIVE_DISPLAY_OFF) {
            return;
        }
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
        // Do nothing if the display is off
        if (context->mode == CD53_MODE_ACTIVE_DISPLAY_OFF) {
            return;
        }
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
        } else if (context->mode != CD53_MODE_OFF &&
            context->mode != CD53_MODE_ACTIVE_DISPLAY_OFF
        ) {
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
        context->mode = CD53_MODE_ACTIVE;
        // Clear Caller ID
        if (context->displayMetadata == CD53_DISPLAY_METADATA_ON) {
            CD53BTMetadata(context, 0x00);
        } else {
            CD53SetMainDisplayText(context, "Bluetooth", 0);
        }
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

/**
 * CD53GTScreenModeSet()
 *     Description:
 *         Track the state that the Monochrome GT expects from the radio (MIR)
 *     Params:
 *         void *ctx - The context
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void CD53GTScreenModeSet(void *ctx, uint8_t *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // Check the screen priority (bit 0 of 0x45). RAD = 0, GT = 1
    if (CHECK_BIT(pkt[IBUS_PKT_DB1], 0) == 1) {
        context->mode = CD53_MODE_ACTIVE_DISPLAY_OFF;
    } else {
        context->mode = CD53_MODE_ACTIVE;
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

/**
 * CD53IBusIgnitionStatus()
 *     Description:
 *         Sanity check to ensure that we do not display text once the
 *         ignition is turned off. I had a philosophical debate about
 *         adding an additional event handler versus just `return` in
 *         CD53TimerDisplay(). It is just cleaner to update the state, if
 *         required, in a bespoke handler.
 *     Params:
 *         void *ctx - A void pointer to the CD53Context_t struct
 *         unsigned char *pkt - The update packet
 *     Returns:
 *         void
 */
void CD53IBusIgnitionStatus(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    uint8_t ignitionStatus = pkt[0];
    if (ignitionStatus == IBUS_IGNITION_OFF && context->mode != CD53_MODE_OFF) {
        IBusCommandTELIKEDisplayClear(context->ibus);
        context->mode = CD53_MODE_OFF;
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
            // Do not do this as it will stick the radio in TEL mode
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
        context->radioType = CONFIG_UI_MIR;
    }
    // Ensure that the display mode is 0xC4 so we know we did not write this
    // to the display
    if (context->mode != CD53_MODE_OFF && pkt[IBUS_PKT_DB1] == 0xC4) {
        CD53RedisplayText(context);
    }
}

void CD53TimerDisplay(void *ctx)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // Do not display text when the mode is OFF or DISPLAY_OFF
    if (context->mode == CD53_MODE_OFF ||
        context->mode == CD53_MODE_ACTIVE_DISPLAY_OFF
    ) {
        return;
    }
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
            } else if (context->radioType == CONFIG_UI_MIR) {
                IBusCommandGTWriteBusinessNavTitle(
                    context->ibus,
                    context->tempDisplay.text
                );
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
        } else if (context->mainDisplay.timeout == 0 ||
                   context->mainDisplay.timeout == CD53_TIMEOUT_SCROLL_STOP_NEXT_ITR
        ) {
            if (context->mainDisplay.length > CD53_DISPLAY_TEXT_LEN) {
                char text[CD53_DISPLAY_TEXT_LEN + 1] = {0};
                uint8_t textLength = CD53_DISPLAY_TEXT_LEN;
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
                // If we start with a space, it will be ignored by the display
                // Instead, use 0x9D which results in a true blank being displayed
                if (text[0] == 0x20) {
                    text[0] = IBUS_RAD_SPACE_CHAR_ALT;
                }
                if (context->radioType == CONFIG_UI_CD53) {
                    IBusCommandTELIKEDisplayWrite(context->ibus, text);
                } else if (context->radioType == CONFIG_UI_MIR) {
                    IBusCommandGTWriteBusinessNavTitle(context->ibus, text);
                }
                uint8_t metaMode = ConfigGetSetting(CONFIG_SETTING_METADATA_MODE);
                // Pause at the beginning of the text
                if (context->mainDisplay.index == 0) {
                    if (metaMode == MENU_SINGLELINE_SETTING_METADATA_MODE_STATIC ||
                        context->mainDisplay.timeout == CD53_TIMEOUT_SCROLL_STOP_NEXT_ITR
                    ) {
                        context->mainDisplay.timeout = CD53_TIMEOUT_SCROLL_STOP;
                    } else {
                        context->mainDisplay.timeout = 5;
                    }
                }
                if (idxEnd >= context->mainDisplay.length) {
                    // Pause at the end of the text or on the next iteration
                    // if we have Party Single Scroll mode enabled
                    context->mainDisplay.index = 0;
                    if (metaMode == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY_SINGLE) {
                        context->mainDisplay.timeout = CD53_TIMEOUT_SCROLL_STOP_NEXT_ITR;
                    } else{
                        context->mainDisplay.timeout = 2;
                    }
                } else {
                    if (context->mode == CD53_MODE_ACTIVE) {
                        if (metaMode == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
                            context->mainDisplay.timeout = 2;
                            context->mainDisplay.index += CD53_DISPLAY_TEXT_LEN;
                        } else if (metaMode == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY ||
                            metaMode == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY_SINGLE
                        ) {
                            context->mainDisplay.index++;
                        }
                    } else {
                        // Use Party Scroll for all modes except metadata
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
                    } else if (context->radioType == CONFIG_UI_MIR) {
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
