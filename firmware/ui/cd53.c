/*
 * File: cd53.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the CD53 UI Mode handler
 */
#include "cd53.h"
static CD53Context_t Context;

void CD53Init(BC127_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.mode = CD53_MODE_OFF;
    Context.mainDisplay = CD53DisplayValueInit("Bluetooth");
    Context.tempDisplay = CD53DisplayValueInit("");
    Context.btDeviceIndex = CD53_PAIRING_DEVICE_NONE;
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
        500
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
    // Unlike the main display, we need to set the timouet beforehand, that way
    // the timer knows how many iterations to display the text for.
    context->tempDisplay.timeout = timeout;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
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
            BC127CommandProfileOpen(context->bt, dev->macId, "AVRCP");
            BC127CommandProfileOpen(context->bt, dev->macId, "HPF");
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

void CD53BC127Metadata(void *ctx, unsigned char *metadata)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
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
        }
    } else if (changerStatus == 0x07) {
        if (context->mode == CD53_MODE_ACTIVE) {
            if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
                TimerTriggerScheduledTask(context->displayUpdateTaskId);
            }
        } else {
            // We're using the "Scan" button as an "enter" key
            if (context->mode == CD53_MODE_DEVICE_SEL) {
                BC127PairedDevice_t *dev = &context->bt->pairedDevices[
                    context->btDeviceIndex
                ];
                // Do nothing if the user selected the active device
                if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0) {
                    // Immediately connect if there isn't an active device
                    if (context->bt->activeDevice.deviceId == 0) {
                        BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
                        BC127CommandProfileOpen(context->bt, dev->macId, "AVRCP");
                        BC127CommandProfileOpen(context->bt, dev->macId, "HPF");
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
            }
            context->mode = CD53_MODE_ACTIVE;
        }
    } else if (changerStatus == 0x08) {
        if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
        }
    }
    if (changerStatus == IBusAction_CD53_SEEK) {
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
    }
    if (changerStatus == IBusAction_CD53_CD_SEL) {
        if (pkt[5] == 0x01) {
            if (context->bt->activeDevice.deviceId != 0) {
                if (btPlaybackStatus == BC127_AVRCP_STATUS_PLAYING) {
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
        } else if (pkt[5] == 0x05) {
            if (context->bt->pairedDevicesCount == 0) {
                CD53SetTempDisplayText(context, "No Device", 4);
            } else {
                context->btDeviceIndex++;
                context->mode = CD53_MODE_DEVICE_SEL;
                BC127PairedDevice_t *dev = 0;
                while (dev == 0) {
                    if (context->btDeviceIndex < context->bt->pairedDevicesCount) {
                        dev = &context->bt->pairedDevices[context->btDeviceIndex];
                    } else {
                        context->btDeviceIndex = 0;
                    }
                }
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
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
        }
    }
}

void CD53TimerDisplay(void *ctx)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->ibus->cdChangerStatus > 0x01) {
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
                        context->mainDisplay.index++;
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
