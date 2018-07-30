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
    Context.ibusAnounce = 0;
    memset(Context.displayText, 0, CD53_DISPLAY_TEXT_SIZE);
    Context.displayTextIdx = 0;
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
        BC127Event_DeviceReady,
        &CD53BC127DeviceReady,
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

static void CD53TriggerDisplay(CD53Context_t *context)
{
    context->displayTextStatus = 1;
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

static void CD53SetDisplayText(CD53Context_t *context, const char *str, uint8_t tickTimeout)
{
    strncpy(context->displayText, str, CD53_DISPLAY_TEXT_SIZE - 1);
    context->displayTextIdx = 0;
    CD53TriggerDisplay(context);
    context->displayTextTicks = tickTimeout;
}

void CD53BC127DeviceReady(void *ctx, unsigned char *tmp)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (strlen(context->displayText) > 0) {
        // The BT Device Reset -- Clear the Display
        memset(context->displayText, 0, CD53_DISPLAY_TEXT_SIZE);
        // If we're in Bluetooth mode, display our banner
        if (context->ibus->cdChangerStatus > 0x01) {
            CD53SetDisplayText(context, "BlueBus", 0);
        }
    }
}

void CD53BC127Metadata(void *ctx, unsigned char *metadata)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    snprintf(
        context->displayText,
        CD53_DISPLAY_TEXT_SIZE,
        "%s - %s on %s",
        context->bt->title,
        context->bt->artist,
        context->bt->album
    );
    context->displayTextIdx = 0;
    // Immediately update the display
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
    context->displayTextTicks = 5;
}

void CD53BC127PlaybackStatus(void *ctx, unsigned char *status)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    // Display "Paused" if we're in Bluetooth mode
    if (context->ibus->cdChangerStatus > 0x01) {
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PAUSED) {
            CD53SetDisplayText(context, "Paused", 0);
        } else {
            CD53SetDisplayText(context, "Playing", 0);
        }
    }
}

void CD53IBusClearScreen(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void CD53IBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    unsigned char changerStatus = pkt[4];
    if (changerStatus == 0x01) {
        // Stop Playing
        IBusCommandDisplayMIDTextClear(context->ibus);
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
        context->ibusAnounce = 0;
    } else if (changerStatus == 0x03) {
        // Start Playing
        if (context->ibusAnounce == 0) {
            if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                CD53SetDisplayText(context, "BlueBus", 0);
            }
            context->ibusAnounce = 1;
        }
    } else if (changerStatus == 0x07 || changerStatus == 0x08) {
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PAUSED) {
            CD53TriggerDisplay(context);
        }
    }
    if (changerStatus == IBusAction_CD53_SEEK) {
        if (pkt[5] == 0x00) {
            BC127CommandForward(context->bt);
            CD53SetDisplayText(context, &IBusMIDSymbolNext, 0);
        } else {
            BC127CommandBackward(context->bt);
        }
    }
    if (changerStatus == IBusAction_CD53_CD_SEL) {
        if (pkt[5] == 0x01) {
            if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                BC127CommandPlay(context->bt);
            }
        } else if (pkt[5] == 0x02 || pkt[5] == 0x03 || pkt[5] == 0x04) {
            CD53TriggerDisplay(context);
            // Display Mode Change
            // Now Playing
            // OBC Data
            // Nothing
        } else if (pkt[5] == 0x05) {
            CD53TriggerDisplay(context);
            // Display & Allow user to select input device
        } else if (pkt[5] == 0x06) {
            // Toggle the discoverable state
            uint8_t state;
            if (context->bt->discoverable == BC127_STATE_ON) {
                CD53SetDisplayText(context, "Pairing Off", 2);
                state = BC127_STATE_OFF;
            } else {
                CD53SetDisplayText(context, "Pairing On", 2);
                state = BC127_STATE_ON;
            }
            BC127CommandBtState(context->bt, context->bt->connectable, state);
        }
    }
}

void CD53TimerDisplay(void *ctx)
{
    CD53Context_t *context = (CD53Context_t *) ctx;
    uint8_t displayTextLength = strlen(context->displayText);
    // If we're displaying something on the screen, avoid clearing it
    if (context->displayTextTicks > 0) {
        displayTextLength = 0;
        context->displayTextTicks--;
    } else {
        if (displayTextLength > 0 && context->ibus->cdChangerStatus > 0x01) {
            if (displayTextLength <= 11) {
                if (context->displayTextStatus == 1) {
                    IBusCommandDisplayMIDText(context->ibus, context->displayText);
                }
            } else {
                char text[12];
                strncpy(text, &context->displayText[context->displayTextIdx], 11);
                text[11] = '\0';
                IBusCommandDisplayMIDText(context->ibus, text);
                // Pause at the beginning of the text
                if (context->displayTextIdx == 0) {
                    context->displayTextTicks = 5;
                }
                if (displayTextLength == (context->displayTextIdx + 11)) {
                    // Pause at the end of the text
                    context->displayTextTicks = 2;
                    context->displayTextIdx = 0;
                } else if (context->displayTextStatus == 0) {
                    context->displayTextIdx++;
                }
            }
        }
    }
    context->displayTextStatus = 0;
}
