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
    Context.selectedPairingDevice = BMBT_PAIRING_DEVICE_NONE;
    EventRegisterCallback(
        BC127Event_DeviceConnected,
        &BMBTBC127DeviceConnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceDisconnected,
        &BMBTBC127DeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BC127Event_MetadataChange,
        &BMBTBC127Metadata,
        &Context
    );
    EventRegisterCallback(
        BC127Event_DeviceReady,
        &BMBTBC127Ready,
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
    IBusCommandGTWriteIndexMk4(context->ibus, 3, " ");
    IBusCommandGTWriteIndexMk4(context->ibus, 4, " ");
    if (context->bt->discoverable == BC127_STATE_ON) {
        IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: On");
    } else {
        IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: Off");
    }
    IBusCommandGTWriteIndexMk4(context->ibus, 6, "Select Device");
    IBusCommandGTWriteIndexMk4(context->ibus, 7, "Settings");
    IBusCommandGTWriteIndexMk4(context->ibus, 8, " ");
    IBusCommandGTWriteIndexMk4(context->ibus, 9, " ");
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_MENU);
    context->menu = BMBT_MENU_MAIN;
}

static void BMBTSetStaticScreen(BMBTContext_t *context, char *f1, char *f2, char *f3)
{
    if (strlen(f1)) {
        strncpy(f1, " ", 1);
    }
    if (strlen(f2)) {
        strncpy(f2, " ", 1);
    }
    if (strlen(f3)) {
        strncpy(f3, " ", 1);
    }
    IBusCommandGTWriteIndexStatic(context->ibus, 1, f1);
    IBusCommandGTWriteIndexStatic(context->ibus, 2, f2);
    IBusCommandGTWriteIndexStatic(context->ibus, 3, f3);
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_STATIC);
}

void BMBTBC127DeviceConnected(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    char name[33];
    char cleanName[12];
    removeNonAscii(name, context->bt->activeDevice.deviceName);
    strncpy(cleanName, name, 11);
    IBusCommandGTWriteZone(context->ibus, 6, cleanName);
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
}

void BMBTBC127DeviceDisconnected(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    IBusCommandGTWriteZone(context->ibus, 6, "No Device");
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
    if (context->selectedPairingDevice != BMBT_PAIRING_DEVICE_NONE) {
        BC127PairedDevice_t *dev = &context->bt->pairedDevices[
            context->selectedPairingDevice
        ];
        if (strlen(dev->macId) > 0) {
            BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
            BC127CommandProfileOpen(context->bt, dev->macId, "AVRCP");
            BC127CommandProfileOpen(context->bt, dev->macId, "HPF");
        }
        context->selectedPairingDevice = BMBT_PAIRING_DEVICE_NONE;
    }
}

void BMBTBC127Metadata(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE &&
        context->menu == BMBT_MENU_NOW_PLAYING
    ) {
        char title[BC127_METADATA_FIELD_SIZE];
        char artist[BC127_METADATA_FIELD_SIZE];
        char album[BC127_METADATA_FIELD_SIZE];
        removeNonAscii(title, context->bt->activeDevice.title);
        removeNonAscii(artist, context->bt->activeDevice.artist);
        removeNonAscii(album, context->bt->activeDevice.album);
        BMBTSetStaticScreen(context, title, artist, album);
    }
}

void BMBTBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE) {
        if (context->bt->activeDevice.playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
            IBusCommandGTWriteZone(context->ibus, 4, "||");
        } else {
            IBusCommandGTWriteZone(context->ibus, 4, "> ");
        }
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
    }
}

void BMBTBC127Ready(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    IBusCommandGTWriteZone(context->ibus, 6, "No Device");
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
    if (context->menu == BMBT_MENU_MAIN) {
        if (context->bt->discoverable == BC127_STATE_ON) {
            IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: Off");
        } else {
            IBusCommandGTWriteIndexMk4(context->ibus, 5, "Pairing: On");
        }
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_MENU);
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
            if (context->bt->activeDevice.playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
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
            if (context->bt->activeDevice.playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            }
            IBusCommandGTWriteTitle(context->ibus, "BlueBus");
            IBusCommandGTWriteZone(context->ibus, 4, "||");
            IBusCommandGTWriteZone(context->ibus, 5, "BT");
            if (context->bt->activeDevice.deviceId != 0) {
                char name[33];
                char cleanName[12];
                removeNonAscii(name, context->bt->activeDevice.deviceName);
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
                BC127PairedDevice_t *dev = 0;
                for (idx = 0; idx < context->bt->pairedDevicesCount; idx++) {
                    dev = &context->bt->pairedDevices[idx];
                    if (dev != 0) {
                        char name[33];
                        removeNonAscii(name, dev->deviceName);
                        char cleanText[12];
                        strncpy(cleanText, name, 11);
                        cleanText[11] = '\0';
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
                        IBusCommandGTWriteIndexMk4(context->ibus, screenIdx, cleanText);
                        screenIdx++;
                    }
                }
                while (screenIdx < 9) {
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
            if (selectedIdx == 9) {
                context->mode = BMBT_MODE_ACTIVE;
                BMBTMainMenu(context);
            } else {
                BC127PairedDevice_t *dev = &context->bt->pairedDevices[selectedIdx];
                if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0 &&
                    dev != 0
                ) {
                    // If we don't have a device connected, connect immediately
                    if (context->bt->activeDevice.deviceId == 0) {
                        BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
                        BC127CommandProfileOpen(context->bt, dev->macId, "AVRCP");
                        BC127CommandProfileOpen(context->bt, dev->macId, "HPF");
                    } else {
                        // Wait until the current device disconnects to
                        // connect the new one
                        BC127CommandClose(
                            context->bt,
                            context->bt->activeDevice.deviceId
                        );
                        context->selectedPairingDevice = selectedIdx;
                    }
                }
            }
        }
    }
}
