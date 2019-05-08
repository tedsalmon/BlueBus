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
    Context.screenUpdated = 0;
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
        IBusEvent_RADMIDDisplayText,
        &MIDIIBusRADMIDDisplayText,
        &Context
    );
}

void MIDBC127MetadataUpdate(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode == MID_MODE_ACTIVE) {
        char text[255];
        snprintf(
            text,
            255,
            "%s - %s - %s",
            context->bt->title,
            context->bt->artist,
            context->bt->album
        );
        IBusCommandMIDDisplayText(context->ibus, text);
    }
}

void MIDBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    LogWarning("Playback status update");
    if (context->mode == MID_MODE_ACTIVE) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            IBusCommandMIDMenuText(context->ibus, 0, " >");
        } else {
            IBusCommandMIDMenuText(context->ibus, 0, "|| ");
        }
    }
}

void MIDIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    unsigned char changerStatus = pkt[4];
    if (changerStatus == 0x01) {
        IBusCommandMIDDisplayText(context->ibus, "                    ");
        // Stop Playing
        context->mode = MID_MODE_OFF;
    } else if (changerStatus == 0x03) {
        // Start Playing
        if (context->mode == MID_MODE_OFF) {
            IBusCommandMIDDisplayTitleText(context->ibus, "Bluetooth");
            IBusCommandMIDDisplayText(context->ibus, "                    ");
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                IBusCommandMIDMenuText(context->ibus, 0, "  >");
            } else {
                IBusCommandMIDMenuText(context->ibus, 0, "|| ");
            }
        IBusCommandMIDMenuText(context->ibus, 1, "Dis");
        IBusCommandMIDMenuText(context->ibus, 2, "Sett");
        IBusCommandMIDMenuText(context->ibus, 3, "ings");
        IBusCommandMIDMenuText(context->ibus, 4, "Devi");
        IBusCommandMIDMenuText(context->ibus, 5, "ces ");
        IBusCommandMIDMenuText(context->ibus, 8, "Pair");
            context->mode = MID_MODE_ACTIVE;
        }
    } else {
        context->screenUpdated = 1;
    }
}

void MIDIIBusRADMIDDisplayText(void *ctx, unsigned char *pkt)
{
    MIDContext_t *context = (MIDContext_t *) ctx;
    if (context->mode != MID_MODE_OFF && context->screenUpdated == 1) {
        IBusCommandMIDDisplayTitleText(context->ibus, "Bluetooth");
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            IBusCommandMIDMenuText(context->ibus, 0, " >");
        } else {
            IBusCommandMIDMenuText(context->ibus, 0, "||");
        }
        IBusCommandMIDMenuText(context->ibus, 1, "Dis");
        IBusCommandMIDMenuText(context->ibus, 2, "Sett");
        IBusCommandMIDMenuText(context->ibus, 3, "ings");
        IBusCommandMIDMenuText(context->ibus, 4, "Devi");
        IBusCommandMIDMenuText(context->ibus, 5, "ces ");
        IBusCommandMIDMenuText(context->ibus, 8, "Pair");
        context->screenUpdated = 0;
    }
}
