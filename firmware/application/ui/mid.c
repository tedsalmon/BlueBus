/*
 * File: mid.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#include "mid.h"
static MIDContext_t Context;

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
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &MIDTimerDisplay,
        &Context,
        MID_DISPLAY_TIMER_INT
    );
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
static void MIDMenuDevices(MIDContext_t *context)
{
    context->mode = MID_MODE_DEVICES;
    IBusCommandMIDDisplayTitleText(context->ibus, "Devices");
    MIDSetMainDisplayText(context, "                    ", 0);
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
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
        //context->settingValue = CONFIG_SETTING_OFF;
    } else {
        MIDSetMainDisplayText(context, "HFP: On", 0);
        //context->settingValue = CONFIG_SETTING_ON;
    }
    MIDSetMainDisplayText(context, "                    ", 0);
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_BACK, "Back");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_EDIT_SAVE, "Edit");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_PREV_VAL, "<   ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_NEXT_VAL, "   >");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_R, "   ");
    IBusCommandMIDMenuText(context->ibus, MID_BUTTON_DEVICES_L, "   ");
    //context->settingIdx = CD53_SETTING_IDX_HFP;
    //context->settingMode = CD53_SETTING_MODE_SCROLL_SETTINGS;
}

void MIDBC127MetadataUpdate(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE && strlen(context->bt->title) > 0) {
        char text[UTILS_DISPLAY_TEXT_SIZE];
        snprintf(
            text,
            UTILS_DISPLAY_TEXT_SIZE,
            "%s - %s on %s",
            context->bt->title,
            context->bt->artist,
            context->bt->album
        );
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
            
        } else if (btnPressed == 2 || btnPressed == 3) {
            IBusCommandMIDDisplayTitleText(context->ibus, "Settings");
            MIDMenuSettings(context);
        }  else if (btnPressed == 4 || btnPressed == 5) {
            IBusCommandMIDDisplayTitleText(context->ibus, "Devices");
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
                    BC127CommandClose(
                        context->bt,
                        context->bt->activeDevice.deviceId
                    );
                }
            }
            BC127CommandBtState(context->bt, context->bt->connectable, state);
        }
    } else if (context->mode == MID_MODE_SETTINGS) {
        if (btnPressed == MID_BUTTON_BACK) {
            MIDMenuMain(context);
        }
    } else if (context->mode == MID_MODE_DEVICES) {
        if (btnPressed == MID_BUTTON_BACK) {
            MIDMenuMain(context);
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
    if (pkt[4] == 0x00 && pkt[IBUS_PKT_LEN] > 0x09) {
        if (context->displayUpdate == MID_DISPLAY_REWRITE) {
            context->displayUpdate = MID_DISPLAY_REWRITE_NEXT;
        } else if (context->displayUpdate == MID_DISPLAY_REWRITE_NEXT) {
            context->displayUpdate = MID_DISPLAY_NONE;
            if (context->mode == MID_MODE_ACTIVE) {
                MIDMenuMain(context);
            } else if (context->mode == MID_MODE_SETTINGS) {
                MIDMenuSettings(context);
            } else if (context->mode == MID_MODE_DEVICES) {
                MIDMenuDevices(context);
            }
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
                            MID_METADATA_MODE_CHUNK
                        ) {
                            context->mainDisplay.timeout = 2;
                            context->mainDisplay.index += MID_DISPLAY_TEXT_SIZE;
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
