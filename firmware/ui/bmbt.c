/*
 * File: bmbt.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the BoardMonitor UI Mode handler
 */
#include "bmbt.h"
static BMBTContext_t Context;

void BMBTInit(BC127_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.menu = BMBT_MENU_MAIN;
    Context.mode = BMBT_MODE_OFF;
    EventRegisterCallback(
        BC127Event_MetadataChange,
        &BMBTBC127Metadata,
        &Context
    );
    EventRegisterCallback(
        BC127Event_PlaybackStatusChange,
        &BMBTBC127PlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_BMBTButton,
        &BMBTIBusBMBTButtonPress,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_CDStatusRequest,
        &BMBTIBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_GTMenuSelect,
        BMBTIBusMenuSelect,
        &Context
    );
}

static void BMBTMainMenu(BMBTContext_t *context)
{
    IBusCommandGTWriteIndexMk4(context->ibus, 0, "Now Playing");
    IBusCommandGTWriteIndexMk4(context->ibus, 1, " ");
    IBusCommandGTWriteIndexMk4(context->ibus, 2, " ");
    if (context->bt->discoverable == BC127_STATE_ON) {
        IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: On");
    } else {
        IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: Off");
    }
    IBusCommandGTWriteIndexMk4(context->ibus, 6, "Select Device");
    IBusCommandGTWriteIndexMk4(context->ibus, 7, "Settings");
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_MENU);
    context->menu = BMBT_MENU_MAIN;
}

static void BMBTSetStaticScreen(BMBTContext_t *context, char *f1, char *f2, char *f3)
{
    IBusCommandGTWriteIndexStatic(context->ibus, 1, f1);
    IBusCommandGTWriteIndexStatic(context->ibus, 2, f2);
    IBusCommandGTWriteIndexStatic(context->ibus, 3, f3);
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_STATIC);
}

void BMBTBC127Metadata(void *ctx, unsigned char *metadata)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE &&
        context->menu == BMBT_MENU_NOW_PLAYING
    ) {
        char title[BC127_METADATA_FIELD_SIZE];
        char artist[BC127_METADATA_FIELD_SIZE];
        char album[BC127_METADATA_FIELD_SIZE];
        removeNonAscii(title, context->bt->title);
        removeNonAscii(artist, context->bt->artist);
        removeNonAscii(album, context->bt->album);
        BMBTSetStaticScreen(context, title, artist, album);
    }
}

void BMBTBC127PlaybackStatus(void *ctx, unsigned char *status)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE) {
        if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PAUSED) {
            IBusCommandGTWriteZone(context->ibus, 4, "||");
        } else {
            IBusCommandGTWriteZone(context->ibus, 4, "> ");
        }
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
    }
}

void BMBTIBusBMBTButtonPress(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE) {
        if (pkt[4] == IBusDevice_BMBT_Button_Next_Out) {
            BC127CommandForward(context->bt);
        }
        if (pkt[4] == IBusDevice_BMBT_Button_Prev_Out) {
            BC127CommandBackward(context->bt);
        }
        if (pkt[4] == IBusDevice_BMBT_Button_PlayPause_Out) {
            if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
                IBusCommandGTWriteZone(context->ibus, 4, "||");
            } else {
                BC127CommandPlay(context->bt);
                IBusCommandGTWriteZone(context->ibus, 4, "> ");
            }
            IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
        }
        if (pkt[4] == IBusDevice_BMBT_Button_Knob_Out) {
            if (context->menu == BMBT_MENU_NOW_PLAYING ||
                context->menu == BMBT_MENU_ABOUT
            ) {
                context->mode = BMBT_MODE_ACTIVE;
                BMBTMainMenu(context);
            }
        }
    }
}

void BMBTIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    unsigned char changerStatus = pkt[4];
    if (changerStatus == 0x01) {
        // Stop Playing
        BMBTSetStaticScreen(context, " ", " ", " ");
        context->mode = BMBT_MODE_OFF;
    } else if (changerStatus == 0x03) {
        // Start Playing
        if (context->mode == BMBT_MODE_OFF) {
            if (context->bt->avrcpStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            }
            IBusCommandGTWriteTitle(context->ibus, "BlueBus");
            IBusCommandGTWriteZone(context->ibus, 4, "||");
            IBusCommandGTWriteZone(context->ibus, 5, "BT");
            if (context->bt->activeDevice != 0) {
                char name[33];
                char cleanName[12];
                removeNonAscii(name, context->bt->activeDevice->deviceName);
                strncpy(cleanName, name, 11);
                IBusCommandGTWriteZone(context->ibus, 6, cleanName);
            } else {
                IBusCommandGTWriteZone(context->ibus, 6, "No Device");
            }
            IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
            BMBTMainMenu(context);
            context->mode = BMBT_MODE_ACTIVE;
            BC127CommandStatus(context->bt);
        }
    }
}

void BMBTIBusMenuSelect(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t selectedIdx = (uint8_t) pkt[6];
    // The depress action has 0x40 added to the index
    if (selectedIdx > 10) {
        selectedIdx = selectedIdx - 64;
        if (context->menu == BMBT_MENU_MAIN) {
            if (selectedIdx == 0) {
                context->menu = BMBT_MENU_NOW_PLAYING;
                BMBTBC127Metadata(ctx, 0);
            } else if (selectedIdx == 5) {
                uint8_t state;
                if (context->bt->discoverable == BC127_STATE_ON) {
                    IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: Off");
                    state = BC127_STATE_OFF;
                } else {
                    IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: On");
                    state = BC127_STATE_ON;
                }
                IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_MENU);
                BC127CommandDiscoverable(context->bt, state);
            } else if (selectedIdx == 6) {
                uint8_t idx;
                uint8_t screenIdx = 0;
                uint8_t connectedDevices = BC127GetConnectedDeviceCount(context->bt);
                BC127Connection_t *conn = 0;
                for (idx = 0; idx < connectedDevices; idx++) {
                    conn = &context->bt->connections[idx];
                    if (conn != 0) {
                        char name[33];
                        removeNonAscii(name, conn->deviceName);
                        char cleanName[12];
                        strncpy(cleanName, name, 11);
                        cleanName[11] = '\0';
                        IBusCommandGTWriteIndexMk4(context->ibus, screenIdx, cleanName);
                        screenIdx++;
                    }
                }
                while (screenIdx < 7) {
                    IBusCommandGTWriteIndexMk4(context->ibus, screenIdx, " ");
                    screenIdx++;
                }
                IBusCommandGTWriteIndexMk4(context->ibus, screenIdx++, "Back");
                IBusCommandGTWriteIndexMk4(context->ibus, screenIdx++, " ");
                IBusCommandGTWriteIndexMk4(context->ibus, screenIdx++, " ");
                IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_MENU);
                context->menu = BMBT_MENU_DEVICE_SELECTION;
            } else if (selectedIdx == 7) {
                context->menu = BMBT_MENU_ABOUT;
                BMBTSetStaticScreen(
                    context,
                    "BlueBus - SW: 1.0.0 HW: 1",
                    " ", " "
                );
            }
        } else if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            // Back Button
            if (selectedIdx == 7) {
                context->mode = BMBT_MODE_ACTIVE;
                BMBTMainMenu(context);
            }
        }
    }
}
