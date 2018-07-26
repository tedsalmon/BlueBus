/*
 * File: handler.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#include "handler.h"
ApplicationContext_t AppCtx;
const static uint16_t HANDLER_CDC_ANOUNCE_INT = 30000;

void HandlerInit(BC127_t *bt, IBus_t *ibus)
{
    AppCtx.bt = bt;
    AppCtx.ibus = ibus;
    AppCtx.btStartup = 1;
    memset(AppCtx.displayText, 0, HANDLER_DISPLAY_TEXT_SIZE);
    AppCtx.displayTextIdx = 0;
    AppCtx.cdChangerLastKeepAlive = TimerGetMillis();
    AppCtx.announceSelf = 0;
    EventRegisterCallback(
        BC127Event_Startup,
        &HandlerBC127Startup,
        &AppCtx
    );
    EventRegisterCallback(
        BC127Event_MetadataChange,
        &HandlerBC127Metadata,
        &AppCtx
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &HandlerBC127PlaybackStatus,
        &AppCtx
    );
    EventRegisterCallback(
        BC127Event_DeviceReady,
        &HandlerBC127DeviceReady,
        &AppCtx
    );
    EventRegisterCallback(
        IBusEvent_Startup,
        &HandlerIBusStartup,
        &AppCtx
    );
    EventRegisterCallback(
        IBusEvent_CDClearDisplay,
        &HandlerIBusCDClearScreen,
        &AppCtx
    );
    EventRegisterCallback(
        IBusEvent_CDKeepAlive,
        &HandlerIBusCDChangerKeepAlive,
        &AppCtx
    );
    EventRegisterCallback(
        IBusEvent_CDStatusRequest,
        &HandlerIBusCDChangerStatus,
        &AppCtx
    );
    AppCtx.displayUpdateTaskId = TimerRegisterScheduledTask(
        &HandlerTimerCD53Display,
        &AppCtx,
        500
    );
    TimerRegisterScheduledTask(
        &HandlerTimerCDChangerAnnounce,
        &AppCtx,
        30000
    );
}

void HandlerBC127DeviceReady(void *ctx, unsigned char *tmp)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    if (strlen(context->displayText) > 0) {
        // The BT Device Reset -- Clear the Display
        memset(context->displayText, 0, HANDLER_DISPLAY_TEXT_SIZE);
        // If we're in Bluetooth mode, display our banner
        if (context->ibus->cdChangerStatus > 0x01) {
            IBusCommandDisplayMIDText(context->ibus, "BlueBus");
        }
    }
}

void HandlerBC127Metadata(void *ctx, unsigned char *metadata)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    snprintf(
        context->displayText,
        HANDLER_DISPLAY_TEXT_SIZE,
        "%s - %s on %s",
        context->bt->title,
        context->bt->artist,
        context->bt->album
    );
    context->displayTextIdx = 0;
    // Immediately update the display
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

void HandlerBC127PlaybackStatus(void *ctx, unsigned char *status)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    // If this is the first Status update
    if (context->btStartup == 1) {
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
            // Request Metadata
            BC127CommandGetMetadata(context->bt);
        }
        context->btStartup = 0;
    }
    // Display "Paused" if we're in Bluetooth mode
    if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PAUSED &&
        context->ibus->cdChangerStatus > 0x01
    ) {
        IBusCommandDisplayMIDText(context->ibus, "Paused");
    } else if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING &&
               context->ibus->cdChangerStatus <= 0x01
    ){
        // We're playing but not in Bluetooth mode - stop playback
        BC127CommandPause(context->bt);
    }
}

void HandlerBC127Startup(void *ctx, unsigned char *tmp)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    BC127CommandStatus(context->bt);
}

void HandlerIBusStartup(void *ctx, unsigned char *tmp)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    IBusCommandSendCdChangerAnnounce(context->ibus);
    // Always assume that we're playing, that way the radio can tell us
    // if we shouldn't be, then we can deduce the radio status
    context->ibus->cdChangerStatus = 0x09;
}

void HandlerIBusCDClearScreen(void *ctx, unsigned char *pkt)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
}

void HandlerIBusCDChangerKeepAlive(void *ctx, unsigned char *pkt)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    IBusCommandSendCdChangerKeepAlive(context->ibus);
    context->cdChangerLastKeepAlive = TimerGetMillis();
}

void HandlerIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    unsigned char curAction = 0x00;
    unsigned char curStatus = 0x02;
    unsigned char changerStatus = pkt[4];
    if (changerStatus == 0x00) {
        // Asking for status
        curStatus = context->ibus->cdChangerStatus;
    } else if (changerStatus == 0x01) {
        // Stop Playing
        context->announceSelf = 0;
        IBusCommandDisplayMIDTextClear(context->ibus);
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
    } else if (changerStatus == 0x03) {
        // Start Playing
        curAction = 0x02;
        curStatus = 0x09;
        if (context->announceSelf == 0) {
            if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            }
            IBusCommandDisplayMIDText(context->ibus, "BlueBus");
            context->announceSelf = 1;
        }
    }
    if (changerStatus == IBusAction_CD53_SEEK) {
        if (pkt[5] == 0x00) {
            BC127CommandForward(context->bt);
            memset(context->displayText, 0, HANDLER_DISPLAY_TEXT_SIZE);
            IBusCommandDisplayMIDTextSymbol(context->ibus, (char) IBusMIDSymbolNext);
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
        } else if (pkt[5] == 0x02) {
            // Display Mode Change
            // Now Playing
            // OBC Data
            // Nothing
        } else if (pkt[5] == 0x05) {
            // Display & Allow user to select input device
        } else if (pkt[5] == 0x06) {
            // Toggle the discoverable state
            uint8_t state;
            if (context->bt->discoverable == BC127_STATE_ON) {
                context->displayTextTicks = 2;
                IBusCommandDisplayMIDText(context->ibus, "Pairing Off");
                state = BC127_STATE_OFF;
            } else {
                IBusCommandDisplayMIDText(context->ibus, "Pairing On");
                state = BC127_STATE_ON;
            }
            BC127CommandBtState(context->bt, context->bt->connectable, state);
        }
    }
    IBusCommandSendCdChangerStatus(context->ibus, &curStatus, &curAction);
    context->cdChangerLastKeepAlive = TimerGetMillis();
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

void HandlerTimerCD53Display(void *ctx)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    uint8_t displayTextLength = strlen(context->displayText);
    // If we're displaying something on the screen, avoid clearing it
    if (context->displayTextTicks > 0) {
        displayTextLength = 0;
        context->displayTextTicks--;
    }
    if (displayTextLength > 0 &&
        context->ibus->cdChangerStatus > 0x01 &&
        context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING
    ) {
        if (displayTextLength <= 11) {
            IBusCommandDisplayMIDText(context->ibus, context->displayText);
        } else {
            char text[12];
            strncpy(text, &context->displayText[context->displayTextIdx], 11);
            text[11] = '\0';
            IBusCommandDisplayMIDText(context->ibus, text);
            if (displayTextLength == (context->displayTextIdx + 11)) {
                context->displayTextIdx = 0;
            } else {
                context->displayTextIdx = context->displayTextIdx + 1;
            }
        }
    }
}

void HandlerTimerCDChangerAnnounce(void *ctx)
{
    ApplicationContext_t *context = (ApplicationContext_t *) ctx;
    uint32_t now = TimerGetMillis();
    if ((now - context->cdChangerLastKeepAlive) >= HANDLER_CDC_ANOUNCE_INT) {
        IBusCommandSendCdChangerAnnounce(context->ibus);
        context->cdChangerLastKeepAlive = now;
    }

}
