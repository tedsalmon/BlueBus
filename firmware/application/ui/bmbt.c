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
    Context.menu = BMBT_MENU_NONE;
    Context.mode = BMBT_MODE_OFF;
    Context.displayMode = BMBT_DISPLAY_OFF;
    Context.writtenIndices = 3;
    Context.timerMenuIntervals = 0;
    Context.selectedPairingDevice = BMBT_PAIRING_DEVICE_NONE;
    Context.activelyPairedDevice = BMBT_PAIRING_DEVICE_NONE;
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
        BC127Event_Boot,
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
        &BMBTIBusMenuSelect,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_RADUpdateMainArea,
        &BMBTRADUpdateMainArea,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_ScreenModeUpdate,
        &BMBTScreenModeUpdate,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_ScreenModeSet,
        &BMBTScreenModeSet,
        &Context
    );
    TimerRegisterScheduledTask(
        &BMBTTimerMenuWrite,
        &Context,
        BMBT_MENU_TIMER_WRITE_INT
    );
}

static void BMBTSetStaticScreen(BMBTContext_t *context, char *f1, char *f2, char *f3)
{
    if (strlen(f1) == 0) {
        strncpy(f1, " ", 1);
    }
    if (strlen(f2) == 0) {
        strncpy(f2, " ", 1);
    }
    if (strlen(f3) == 0) {
        strncpy(f3, " ", 1);
    }
    IBusCommandGTWriteIndexStatic(context->ibus, 1, f1);
    IBusCommandGTWriteIndexStatic(context->ibus, 2, f2);
    IBusCommandGTWriteIndexStatic(context->ibus, 3, f3);
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_STATIC);
}

static void BMBTMainMenu(BMBTContext_t *context)
{
    IBusCommandGTWriteIndexTitle(context->ibus, "Main Menu");
    IBusCommandGTWriteIndexMk4(
        context->ibus,
        BMBT_MENU_IDX_DASHBOARD,
        "Dashboard"
    );
    IBusCommandGTWriteIndexMk4(
        context->ibus,
        BMBT_MENU_IDX_DEVICE_SELECTION,
        "Select Device"
    );
    IBusCommandGTWriteIndexMk4(
        context->ibus,
        BMBT_MENU_IDX_SETTINGS,
        "Settings"
    );
    uint8_t index = 3;
    while (index < context->writtenIndices) {
        IBusCommandGTWriteIndexMk4(context->ibus, index, " ");
        index++;
    }
    IBusCommandGTWriteIndexMk4(context->ibus, BMBT_MENU_IDX_BACK, " ");
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_INDEX);
    context->writtenIndices = 3;
    context->menu = BMBT_MENU_MAIN;
}

static void BMBTDashboardMenu(BMBTContext_t *context)
{
    char title[BC127_METADATA_FIELD_SIZE];
    char artist[BC127_METADATA_FIELD_SIZE];
    char album[BC127_METADATA_FIELD_SIZE];
    memset(title, 0, BC127_METADATA_FIELD_SIZE);
    memset(artist, 0, BC127_METADATA_FIELD_SIZE);
    memset(album, 0, BC127_METADATA_FIELD_SIZE);
    removeNonAscii(title, context->bt->title);
    removeNonAscii(artist, context->bt->artist);
    removeNonAscii(album, context->bt->album);
    if (strlen(title) == 0 &&
        context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED
    ) {
        strncpy(title, "- Not Playing -", 16);
    }
    BMBTSetStaticScreen(context, title, artist, album);
    context->menu = BMBT_MENU_DASHBOARD;
}

static void BMBTDeviceSelectionMenu(BMBTContext_t *context)
{
    IBusCommandGTWriteIndexTitle(context->ibus, "Device Selection");
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
    if (context->bt->discoverable == BC127_STATE_ON) {
        IBusCommandGTWriteIndexMk4(context->ibus, screenIdx++, "Pairing: On");
    } else {
        IBusCommandGTWriteIndexMk4(context->ibus, screenIdx++, "Pairing: Off");
    }
    IBusCommandGTWriteIndexMk4(context->ibus, BMBT_MENU_IDX_BACK, "Back");
    uint8_t index = screenIdx;
    while (index < context->writtenIndices) {
        IBusCommandGTWriteIndexMk4(context->ibus, index, " ");
        index++;
    }
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_INDEX);
    context->writtenIndices = screenIdx;
    context->menu = BMBT_MENU_DEVICE_SELECTION;
}

static void BMBTSettingsMenu(BMBTContext_t *context)
{
    IBusCommandGTWriteIndexTitle(context->ibus, "Settings");
    IBusCommandGTWriteIndexMk4(
        context->ibus,
        BMBT_MENU_IDX_SETTINGS_SCROLL_META,
        "Scroll Metadata"
    );
    IBusCommandGTWriteIndexMk4(
        context->ibus,
        BMBT_MENU_IDX_SETTINGS_RESET_BT,
        "Reset Bluetooth"
    );
    IBusCommandGTWriteIndexMk4(
        context->ibus,
        BMBT_MENU_IDX_SETTINGS_RESET_PAIRED_DEVICE_LIST,
        "Reset BT PDL"
    );
    IBusCommandGTWriteIndexMk4(context->ibus, BMBT_MENU_IDX_BACK, "Back");
    uint8_t index = 3;
    while (index < context->writtenIndices) {
        IBusCommandGTWriteIndexMk4(context->ibus, index, " ");
        index++;
    }
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_INDEX);
    context->writtenIndices = index;
    context->menu = BMBT_MENU_SETTINGS;
}

static void BMBTWriteHeader(BMBTContext_t *context)
{
    IBusCommandGTWriteTitle(context->ibus, "Bluetooth");
    if (context->bt->activeDevice.deviceId != 0) {
        char name[33];
        char cleanName[12];
        removeNonAscii(name, context->bt->activeDevice.deviceName);
        strncpy(cleanName, name, 11);
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, cleanName);
    } else {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, "No Device");
    }
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
    } else {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "> ");
    }
    IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_BT, "BT  ");
    IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
}

static void BMBTWriteMenu(BMBTContext_t *context)
{
    switch (context->menu) {
        case BMBT_MENU_MAIN:
            BMBTMainMenu(context);
            break;
        case BMBT_MENU_DASHBOARD:
            BMBTDashboardMenu(context);
            break;
        case BMBT_MENU_DEVICE_SELECTION:
            BMBTDeviceSelectionMenu(context);
            break;
        case BMBT_MENU_SETTINGS:
            BMBTSettingsMenu(context);
            break;
        case BMBT_MENU_NONE:
            BMBTMainMenu(context);
            break;
    }
}

static void BMBTMenuRefresh(BMBTContext_t *context)
{
    if (context->menu != BMBT_MENU_DASHBOARD) {
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_INDEX);
    } else {
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_STATIC);
    }
}

void BMBTBC127DeviceConnected(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->displayMode == BMBT_DISPLAY_ON) {
        char name[33];
        char cleanName[12];
        removeNonAscii(name, context->bt->activeDevice.deviceName);
        strncpy(cleanName, name, 11);
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, cleanName);
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
        if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            BMBTDeviceSelectionMenu(context);
        }
    }
}

void BMBTBC127DeviceDisconnected(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->displayMode == BMBT_DISPLAY_ON) {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, "No Device");
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
        if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            BMBTDeviceSelectionMenu(context);
        }
    }
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
        context->menu == BMBT_MENU_DASHBOARD &&
        context->displayMode == BMBT_DISPLAY_ON
    ) {
        BMBTDashboardMenu(context);
    }
}

void BMBTBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->displayMode == BMBT_DISPLAY_ON) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
        } else {
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "> ");
        }
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
    }
}

void BMBTBC127Ready(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, "No Device");
    if (context->displayMode == BMBT_DISPLAY_ON) {
        IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_ZONE);
        // Write the pairing status
    }
}

void BMBTIBusBMBTButtonPress(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE) {
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_Next) {
            BC127CommandForward(context->bt);
        }
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_Prev) {
            BC127CommandBackward(context->bt);
        }
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_PlayPause) {
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                BC127CommandPlay(context->bt);
            }
        }
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_Knob) {
            if (context->menu == BMBT_MENU_DASHBOARD) {
                BMBTMainMenu(context);
            }
        }
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_Display) {
            if (context->mode == BMBT_MODE_ACTIVE &&
                context->displayMode == BMBT_DISPLAY_OFF
            ) {
                context->displayMode = BMBT_DISPLAY_ON;
                context->menu = BMBT_MENU_NONE;
                // Maybe this will stop the constant screen clearing when
                // we enter the menu again?
                IBusCommandRADDisableMenu(context->ibus);
            }
        }
    }
}

void BMBTIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    unsigned char changerAction = pkt[4];
    if (changerAction == IBUS_CDC_STOP_PLAYING) {
        // Stop Playing
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
        }
        context->menu = BMBT_MENU_NONE;
        context->mode = BMBT_MODE_OFF;
        context->displayMode = BMBT_DISPLAY_OFF;
        IBusCommandRADEnableMenu(context->ibus);
    } else if (changerAction == IBUS_CDC_START_PLAYING) {
        // Start Playing
        if (context->mode == BMBT_MODE_OFF) {
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            }
            IBusCommandRADDisableMenu(context->ibus);
            context->mode = BMBT_MODE_ACTIVE;
            context->displayMode = BMBT_DISPLAY_ON;
        }
    }
}

void BMBTIBusMenuSelect(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t selectedIdx = (uint8_t) pkt[6];
    if (selectedIdx > 10 && context->displayMode == BMBT_DISPLAY_ON) {
        selectedIdx = selectedIdx - 64;
        if (context->menu == BMBT_MENU_MAIN) {
            if (selectedIdx == BMBT_MENU_IDX_DASHBOARD) {
                BMBTDashboardMenu(context);
            } else if (selectedIdx == BMBT_MENU_IDX_DEVICE_SELECTION) {
                BMBTDeviceSelectionMenu(context);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS) {
                BMBTSettingsMenu(context);
            }
        } else if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            uint8_t index = context->bt->pairedDevicesCount + 1;
            if (selectedIdx == index - 1) {
                uint8_t state;
                if (context->bt->discoverable == BC127_STATE_ON) {
                    IBusCommandGTWriteIndexMk4(context->ibus, selectedIdx, "Pairing: Off");
                    state = BC127_STATE_OFF;
                } else {
                    IBusCommandGTWriteIndexMk4(context->ibus, selectedIdx, "Pairing: On");
                    state = BC127_STATE_ON;
                    if (context->bt->activeDevice.deviceId != 0) {
                        // To pair a new device, we must disconnect the active one
                        BC127CommandClose(
                            context->bt,
                            context->bt->activeDevice.deviceId
                        );
                    }
                }
                IBusCommandGTUpdate(context->ibus, IBusAction_GT_WRITE_INDEX);
                BC127CommandBtState(context->bt, context->bt->connectable, state);
            } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
                // Back Button
                BMBTMainMenu(context);
            } else {
                BC127PairedDevice_t *dev = &context->bt->pairedDevices[selectedIdx];
                if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0 &&
                    dev != 0
                ) {
                    // If we don't have a device connected, connect immediately
                    if (context->bt->activeDevice.deviceId == 0) {
                        BC127CommandProfileOpen(context->bt, dev->macId, "A2DP");
                        BC127CommandProfileOpen(context->bt, dev->macId, "HPF");
                    } else {
                        // Wait until the current device disconnects to
                        // connect the new one
                        BC127CommandClose(
                            context->bt,
                            context->bt->activeDevice.deviceId
                        );
                        context->selectedPairingDevice = selectedIdx;
                        context->activelyPairedDevice = selectedIdx;
                    }
                }
            }
        } else if (context->menu == BMBT_MENU_SETTINGS) {
            if (selectedIdx == BMBT_MENU_IDX_SETTINGS_SCROLL_META) {
                // Do nothing, yet
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_RESET_BT) {
                BC127CommandReset(context->bt);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_RESET_PAIRED_DEVICE_LIST) {
                BC127CommandClose(context->bt, BC127_CLOSE_ALL);
                BC127CommandUnpair(context->bt);
            } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
                BMBTMainMenu(context);
            }
        }
    }
}

void BMBTRADUpdateMainArea(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t pktLen = (uint8_t) pkt[1] + 2;
    uint8_t textLen = pktLen - 7;
    char text[textLen];
    uint8_t idx = 0;
    while (idx < textLen) {
        text[idx] = pkt[idx + 6];
        idx++;
    }
    idx--;
    while (text[idx] == 0x20 || text[idx] == 0x00) {
        text[idx] = '\0';
        idx--;
    }
    text[textLen - 1] = '\0';
    // Main area is being updated with a CDC Text:
    //     Rewrite the header and set display mode on
    if (strcmp("CDC 1-01", text) == 0 || strcmp("TR 01-001", text) == 0) {
        context->mode = BMBT_MODE_ACTIVE;
        BMBTWriteHeader(context);
        if (context->displayMode == BMBT_DISPLAY_ON) {
            // Write the current menu back out
            if (context->menu != BMBT_MENU_NONE) {
                BMBTMenuRefresh(context);
            }
        } else {
            context->displayMode = BMBT_DISPLAY_ON;
        }
    } else if (strcmp("NO DISC", text) == 0 || strcmp("No Disc", text) == 0) {
        context->mode = BMBT_MODE_ACTIVE;
        context->displayMode = BMBT_DISPLAY_ON;
        IBusCommandGTWriteTitle(context->ibus, "Bluetooth");
    } else if (pkt[pktLen - 2] == IBUS_RAD_MAIN_AREA_WATERMARK) {
        context->displayMode = BMBT_DISPLAY_ON;
    } else {
        context->menu = BMBT_MENU_NONE;
        context->displayMode = BMBT_DISPLAY_OFF;
    }
}

void BMBTScreenModeUpdate(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[4] == 0x01 || pkt[4] == 0x02) {
        context->menu = BMBT_MENU_NONE;
        context->displayMode = BMBT_DISPLAY_OFF;
    }
    if (pkt[4] == 0x0C &&
        context->mode == BMBT_MODE_ACTIVE &&
        context->displayMode == BMBT_DISPLAY_ON
    ) {
        // Write the current menu back out
        if (context->menu == BMBT_MENU_NONE) {
            IBusCommandRADDisableMenu(context->ibus);
            BMBTWriteMenu(context);
        } else {
            BMBTMenuRefresh(context);
        }
    }
}

void BMBTScreenModeSet(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    // The GT sends this screen mode post-boot to tell the radio it can display
    // We set the menu to none so that on the next screen clear, we write the
    // screen
    if (pkt[4] == 0x10) {
        context->menu = BMBT_MENU_NONE;
    }
}

/**
 * If more than the elapsed time has passed since the screen was set to CDC
 * mode but we haven't gotten a screen clear from the radio, write out the
 * menu, otherwise we may never write the menu out.
 */
void BMBTTimerMenuWrite(void *ctx)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->menu == BMBT_MENU_NONE && context->displayMode == BMBT_DISPLAY_ON) {
        uint16_t time = context->timerMenuIntervals * BMBT_MENU_TIMER_WRITE_INT;
        if (time >= BMBT_MENU_TIMER_WRITE_TIMEOUT) {
            LogDebug("Writing Menu from Timer");
            IBusCommandRADDisableMenu(context->ibus);
            BMBTWriteMenu(context);
            context->timerMenuIntervals = 0;
        } else {
            context->timerMenuIntervals++;
        }
    }
}
