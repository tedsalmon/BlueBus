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
    Context.navType = ConfigGetNavType();
    Context.navIndexType = IBUS_CMD_GT_WRITE_INDEX_TMC;
    Context.radType = IBUS_RADIO_TYPE_BM53;
    Context.writtenIndices = 3;
    Context.timerMenuIntervals = 0;
    Context.selectedPairingDevice = BMBT_PAIRING_DEVICE_NONE;
    Context.activelyPairedDevice = BMBT_PAIRING_DEVICE_NONE;
    Context.mainDisplay = UtilsDisplayValueInit("Bluetooth", BMBT_DISPLAY_OFF);
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
        IBusEvent_GTDiagResponse,
        &BMBTIBusGTDiagnostics,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_GTMenuSelect,
        &BMBTIBusMenuSelect,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_RADC43ScreenModeUpdate,
        &BMBTRADC43ScreenModeUpdate,
        &Context
    );
    EventRegisterCallback(
        IBusEvent_RADDisplayMenu,
        &BMBTRADDisplayMenu,
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
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &BMBTTimerScrollDisplay,
        &Context,
        BMBT_SCROLL_TEXT_TIMER
    );
    IBusCommandDIAGetIdentity(ibus, IBUS_DEVICE_GT);
}

static void BMBTSetMainDisplayText(
    BMBTContext_t *context,
    const char *str,
    int8_t timeout,
    uint8_t autoUpdate
) {
    strncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE - 1);
    context->mainDisplay.length = strlen(context->mainDisplay.text);
    context->mainDisplay.index = 0;
    if (autoUpdate == 1) {
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
    context->mainDisplay.timeout = timeout;
}

/**
 * BMBTGTWriteIndex()
 *     Description:
 *         Wrapper to automatically push the nav type into the IBus Library
 *         Command so that we can save verbosity in these calls
 *     Params:
 *         BMBTContext_t *context - The context
 *         uint8_t index - The index to write to
 *         char *text - The text to write
 *     Returns:
 *         void
 */
static void BMBTGTWriteIndex(BMBTContext_t *context, uint8_t index, char *text)
{
    context->navIndexType = IBUS_CMD_GT_WRITE_INDEX_TMC;
    IBusCommandGTWriteIndexTMC(context->ibus, index, text, context->navType);
}

static void BMBTSetDashboard(BMBTContext_t *context, char *f1, char *f2, char *f3)
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
    if (context->navType == IBUS_GT_MKIV_STATIC) {
        IBusCommandGTWriteIndexStatic(context->ibus, 1, f1);
        IBusCommandGTWriteIndexStatic(context->ibus, 2, f2);
        IBusCommandGTWriteIndexStatic(context->ibus, 3, f3);
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_STATIC);
    } else {
        IBusCommandGTWriteIndex(context->ibus, 0, f1, context->navType);
        IBusCommandGTWriteIndex(context->ibus, 1, f2, context->navType);
        IBusCommandGTWriteIndex(context->ibus, 2, f3, context->navType);
        context->navIndexType = IBUS_CMD_GT_WRITE_INDEX;
        uint8_t index = 3;
        while (index < context->writtenIndices) {
            BMBTGTWriteIndex(context, index, " ");
            index++;
        }
        context->writtenIndices = 3;
        IBusCommandGTUpdate(context->ibus, context->navIndexType);
    }
}

static void BMBTMainMenu(BMBTContext_t *context)
{
    IBusCommandGTWriteIndexTitle(context->ibus, "Main Menu");
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_DASHBOARD, "Dashboard");
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_DEVICE_SELECTION, "Select Device");
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_SETTINGS, "Settings");
    uint8_t index = 3;
    while (index < context->writtenIndices) {
        BMBTGTWriteIndex(context, index, " ");
        index++;
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, " ");
    IBusCommandGTUpdate(context->ibus, context->navIndexType);
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
    UtilsRemoveNonAscii(title, context->bt->title);
    UtilsRemoveNonAscii(artist, context->bt->artist);
    UtilsRemoveNonAscii(album, context->bt->album);
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
        if (strlen(title) == 0) {
            strncpy(title, "- Not Playing -", 16);
            strncpy(artist, " ", 2);
            strncpy(album, " ", 2);
        }
    } else {
        if (strlen(title) == 0) {
            strncpy(title, "Unknown Title", 14);
        }
        if (strlen(artist) == 0) {
            strncpy(artist, "Unknown Artist", 15);
        }
        if (strlen(album) == 0) {
            strncpy(album, "Unknown Album", 14);
        }
    }
    BMBTSetDashboard(context, title, artist, album);
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
            UtilsRemoveNonAscii(name, dev->deviceName);
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
            BMBTGTWriteIndex(context, screenIdx, cleanText);
            screenIdx++;
        }
    }
    if (context->bt->discoverable == BC127_STATE_ON) {
        BMBTGTWriteIndex(context, screenIdx++, "Pairing: On");
    } else {
        BMBTGTWriteIndex(context, screenIdx++, "Pairing: Off");
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, "Back");
    uint8_t index = screenIdx;
    while (index < context->writtenIndices) {
        BMBTGTWriteIndex(context, index, " ");
        index++;
    }
    IBusCommandGTUpdate(context->ibus, context->navIndexType);
    context->writtenIndices = screenIdx;
    context->menu = BMBT_MENU_DEVICE_SELECTION;
}

static void BMBTSettingsMenu(BMBTContext_t *context)
{
    IBusCommandGTWriteIndexTitle(context->ibus, "Settings");
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_SETTINGS_HFP, "HFP: Off");
    } else {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_SETTINGS_HFP, "HFP: On");
    }
    unsigned char metadataMode = ConfigGetSetting(CONFIG_SETTING_METADATA_MODE);
    if (metadataMode == BMBT_METADATA_MODE_PARTY) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_METADATA_MODE,
            "Metadata: Party"
        );
    } else if (metadataMode == BMBT_METADATA_MODE_CHUNK) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_METADATA_MODE,
            "Metadata: Chunk"
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_METADATA_MODE,
            "Metadata: Off"
        );
    }
    if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == 0x00) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUTOPLAY,
            "Autoplay: Off"
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUTOPLAY,
            "Autoplay: On"
        );
    }
    if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x00) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_DEFAULT_MENU,
            "Def Menu: Main"
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_DEFAULT_MENU,
            "Def Menu: Dash"
        );
    }
    unsigned char vehicleType = ConfigGetVehicleType();
    if (vehicleType == 0x00 || vehicleType == 0xFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_VEHICLE_TYPE,
            "Car: Unset"
        );
    } else if (vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_VEHICLE_TYPE,
            "Car: E38/E39"
        );
    } else if (vehicleType == IBUS_VEHICLE_TYPE_E39_LATE) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_VEHICLE_TYPE,
            "Car: 03+ E39"
        );
    } else if (vehicleType == IBUS_VEHICLE_TYPE_E46) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_VEHICLE_TYPE,
            "Car: E46"
        );
    } else if (vehicleType == IBUS_VEHICLE_TYPE_E46_LCI_Z4) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_VEHICLE_TYPE,
            "Car: E46 LCI/Z4"
        );
    }
    unsigned char blinkCount = ConfigGetSetting(CONFIG_SETTING_OT_BLINKERS);
    if (blinkCount == 0x03) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_BLINKERS,
            "OT Blinkers: 3"
        );
    } else if (blinkCount == 0x05) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_BLINKERS,
            "OT Blinkers: 5"
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_BLINKERS,
            "OT Blinkers: 1"
        );
    }
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_RESET_PAIRINGS,
        "Reset Pairings"
    );
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, "Back");
    uint8_t idx = 7;
    while (idx < context->writtenIndices) {
        BMBTGTWriteIndex(context, idx, " ");
        idx++;
    }
    IBusCommandGTUpdate(context->ibus, context->navIndexType);
    context->writtenIndices = idx;
    context->menu = BMBT_MENU_SETTINGS;
}

static void BMBTWriteHeader(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF ||
        context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED
    ) {
        IBusCommandGTWriteTitle(context->ibus, "Bluetooth");
    } else {
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
    if (context->bt->activeDevice.deviceId != 0) {
        char name[33];
        char cleanName[12];
        UtilsRemoveNonAscii(name, context->bt->activeDevice.deviceName);
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
    IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
}

static void BMBTWriteMenu(BMBTContext_t *context)
{
    switch (context->menu) {
        case BMBT_MENU_MAIN:
            BMBTMainMenu(context);
            break;
        case BMBT_MENU_DASHBOARD:
        case BMBT_MENU_DASHBOARD_FRESH:
            BMBTDashboardMenu(context);
            break;
        case BMBT_MENU_DEVICE_SELECTION:
            BMBTDeviceSelectionMenu(context);
            break;
        case BMBT_MENU_SETTINGS:
            BMBTSettingsMenu(context);
            break;
        case BMBT_MENU_NONE:
            if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x01) {
                BMBTDashboardMenu(context);
            } else {
                BMBTMainMenu(context);
            }
            break;
    }
}

static void BMBTMenuRefresh(BMBTContext_t *context)
{
    if ((context->menu != BMBT_MENU_DASHBOARD &&
        context->menu != BMBT_MENU_DASHBOARD_FRESH) ||
        context->navType != IBUS_GT_MKIV_STATIC
    ) {
        IBusCommandGTUpdate(context->ibus, context->navIndexType);
    } else {
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_STATIC);
    }
}

void BMBTBC127DeviceConnected(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->displayMode == BMBT_DISPLAY_ON) {
        char name[33];
        char cleanName[12];
        UtilsRemoveNonAscii(name, context->bt->activeDevice.deviceName);
        strncpy(cleanName, name, 11);
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, cleanName);
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
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
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
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
        }
        context->selectedPairingDevice = BMBT_PAIRING_DEVICE_NONE;
    }
}

void BMBTBC127Metadata(void *ctx, unsigned char *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE &&
        context->displayMode == BMBT_DISPLAY_ON
    ) {
        if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) != CONFIG_SETTING_OFF) {
            char text[UTILS_DISPLAY_TEXT_SIZE];
            snprintf(
                text,
                UTILS_DISPLAY_TEXT_SIZE,
                "%s - %s - %s",
                context->bt->title,
                context->bt->artist,
                context->bt->album
            );
            char cleanText[UTILS_DISPLAY_TEXT_SIZE];
            UtilsRemoveNonAscii(cleanText, text);
            BMBTSetMainDisplayText(context, cleanText, 0, 1);
        }
        if (context->menu == BMBT_MENU_DASHBOARD) {
            BMBTDashboardMenu(context);
        }
    }
}

void BMBTBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->displayMode == BMBT_DISPLAY_ON) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
            BMBTSetMainDisplayText(context, "Bluetooth", 0, 1);
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
        } else {
            TimerTriggerScheduledTask(context->displayUpdateTaskId);
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "> ");
        }
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    }
}

void BMBTBC127Ready(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, "No Device");
    if (context->displayMode == BMBT_DISPLAY_ON) {
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    }
}

void BMBTIBusBMBTButtonPress(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE) {
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_PlayPause) {
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                BC127CommandPlay(context->bt);
            }
        }
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_Knob) {
            if (context->displayMode == BMBT_DISPLAY_ON &&
                context->menu == BMBT_MENU_DASHBOARD &&
                context->navType == IBUS_GT_MKIV_STATIC
            ) {
                BMBTMainMenu(context);
            }
        }
        if (pkt[4] == IBUS_DEVICE_BMBT_Button_Display) {
            if (context->mode == BMBT_MODE_ACTIVE &&
                context->displayMode == BMBT_DISPLAY_OFF
            ) {
                context->displayMode = BMBT_DISPLAY_ON;
                if (context->menu != BMBT_MENU_DASHBOARD_FRESH) {
                    context->menu = BMBT_MENU_NONE;
                }
                // Stop the constant screen clearing when re-entering the menu
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
        BMBTSetMainDisplayText(context, "Bluetooth", 0, 0);
        IBusCommandRADEnableMenu(context->ibus);
    } else if (changerAction == IBUS_CDC_START_PLAYING ||
               changerAction == IBUS_CDC_START_PLAYING_BM54
    ) {
        // Start Playing
        if (context->mode == BMBT_MODE_OFF) {
            if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON) {
                BC127CommandPlay(context->bt);
            } else if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            }
            context->mode = BMBT_MODE_ACTIVE;
            context->displayMode = BMBT_DISPLAY_ON;
        }
    }
}

void BMBTIBusGTDiagnostics(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t navType = IBusGetNavType(pkt);
    if (navType != ConfigGetNavType()) {
        // Write it to the EEPROM
        ConfigSetNavType(navType);
        context->navType = navType;
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
        } else if (context->menu == BMBT_MENU_DASHBOARD) {
            BMBTMainMenu(context);
        } else if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            uint8_t index = context->bt->pairedDevicesCount + 1;
            if (selectedIdx == index - 1) {
                uint8_t state;
                if (context->bt->discoverable == BC127_STATE_ON) {
                    BMBTGTWriteIndex(context, selectedIdx, "Pairing: Off");
                    state = BC127_STATE_OFF;
                } else {
                    BMBTGTWriteIndex(context, selectedIdx, "Pairing: On");
                    state = BC127_STATE_ON;
                    if (context->bt->activeDevice.deviceId != 0) {
                        // To pair a new device, we must disconnect the active one
                        BC127CommandClose(
                            context->bt,
                            context->bt->activeDevice.deviceId
                        );
                    }
                }
                IBusCommandGTUpdate(context->ibus, context->navIndexType);
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
            if (selectedIdx == BMBT_MENU_IDX_SETTINGS_HFP) {
                unsigned char value = ConfigGetSetting(CONFIG_SETTING_HFP);
                if (value == 0x00) {
                    ConfigSetSetting(CONFIG_SETTING_HFP, 0x01);
                    BMBTGTWriteIndex(context, selectedIdx, "HFP: On");
                    BC127CommandSetProfiles(context->bt, 1, 1, 0, 1);
                } else {
                    BC127CommandSetProfiles(context->bt, 1, 1, 0, 0);
                    BMBTGTWriteIndex(context, selectedIdx, "HFP: Off");
                    ConfigSetSetting(CONFIG_SETTING_HFP, 0x00);
                }
                BC127CommandReset(context->bt);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_METADATA_MODE) {
                unsigned char value = ConfigGetSetting(
                    CONFIG_SETTING_METADATA_MODE
                );
                if (value == 0x00) {
                    value = BMBT_METADATA_MODE_PARTY;
                    BMBTGTWriteIndex(context, selectedIdx, "Metadata: Party");
                } else if (value == 0x01) {
                    value = BMBT_METADATA_MODE_CHUNK;
                    BMBTGTWriteIndex(context, selectedIdx, "Metadata: Chunk");
                } else {
                    value = BMBT_METADATA_MODE_OFF;
                    BMBTGTWriteIndex(context, selectedIdx, "Metadata: Off");
                }
                ConfigSetSetting(CONFIG_SETTING_METADATA_MODE, value);
                if (value != BMBT_METADATA_MODE_OFF &&
                    strlen(context->bt->title) > 0 &&
                    context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING
                ) {
                    char text[UTILS_DISPLAY_TEXT_SIZE];
                    snprintf(
                        text,
                        UTILS_DISPLAY_TEXT_SIZE,
                        "%s - %s - %s",
                        context->bt->title,
                        context->bt->artist,
                        context->bt->album
                    );
                    char cleanText[UTILS_DISPLAY_TEXT_SIZE];
                    UtilsRemoveNonAscii(cleanText, text);
                    BMBTSetMainDisplayText(context, cleanText, 0, 0);
                } else if (value == BMBT_METADATA_MODE_OFF) {
                    IBusCommandGTUpdate(context->ibus, context->navIndexType);
                    IBusCommandGTWriteTitle(context->ibus, "Bluetooth");
                }
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUTOPLAY) {
                if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == 0x00) {
                    ConfigSetSetting(CONFIG_SETTING_AUTOPLAY, 0x01);
                    BMBTGTWriteIndex(context, selectedIdx, "Autoplay: On");
                } else {
                    ConfigSetSetting(CONFIG_SETTING_AUTOPLAY, 0x00);
                    BMBTGTWriteIndex(context, selectedIdx, "Autoplay: Off");
                }
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_DEFAULT_MENU) {
                if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x00) {
                    ConfigSetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU, 0x01);
                    BMBTGTWriteIndex(context, selectedIdx, "Def Menu: Dash");
                } else {
                    ConfigSetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU, 0x00);
                    BMBTGTWriteIndex(context, selectedIdx, "Def Menu: Main");
                }
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_VEHICLE_TYPE) {
                unsigned char value = ConfigGetVehicleType();
                if (value == 0 || value == 0xFF || value == IBUS_VEHICLE_TYPE_E46_LCI_Z4) {
                    ConfigSetVehicleType(IBUS_VEHICLE_TYPE_E38_E39_E53);
                    BMBTGTWriteIndex(context, selectedIdx, "Car: E38/E39");
                } else if (value == IBUS_VEHICLE_TYPE_E38_E39_E53) {
                    ConfigSetVehicleType(IBUS_VEHICLE_TYPE_E39_LATE);
                    BMBTGTWriteIndex(context, selectedIdx, "Car: 03+ E39");
                } else if (value == IBUS_VEHICLE_TYPE_E39_LATE) {
                    ConfigSetVehicleType(IBUS_VEHICLE_TYPE_E46);
                    BMBTGTWriteIndex(context, selectedIdx, "Car: E46");
                } else if (value == IBUS_VEHICLE_TYPE_E46) {
                    ConfigSetVehicleType(IBUS_VEHICLE_TYPE_E46_LCI_Z4);
                    BMBTGTWriteIndex(context, selectedIdx, "Car: E46 LCI/Z4");
                }
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_BLINKERS) {
                unsigned char value = ConfigGetSetting(CONFIG_SETTING_OT_BLINKERS);
                if (value == 0) {
                    ConfigSetSetting(CONFIG_SETTING_OT_BLINKERS, 3);
                    BMBTGTWriteIndex(context, selectedIdx, "OT Blinkers: 3");
                } else if (value == 3) {
                    ConfigSetSetting(CONFIG_SETTING_OT_BLINKERS, 5);
                    BMBTGTWriteIndex(context, selectedIdx, "OT Blinkers: 5");
                } else {
                    ConfigSetSetting(CONFIG_SETTING_OT_BLINKERS, 0);
                    BMBTGTWriteIndex(context, selectedIdx, "OT Blinkers: 1");
                }
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_RESET_PAIRINGS) {
                BC127CommandUnpair(context->bt);
                // Set the BT module connectable
                BC127CommandBtState(
                    context->bt,
                    BC127_STATE_ON,
                    context->bt->discoverable
                );
                BC127CommandClose(context->bt, BC127_CLOSE_ALL);
                BC127ClearPairedDevices(context->bt);
            } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
                BMBTMainMenu(context);
            }
            IBusCommandGTUpdate(context->ibus, context->navIndexType);
        }
    }
}

void BMBTRADC43ScreenModeUpdate(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    // If I am honest, I don't know how define the byte below (0x06)
    // other than it seems to "unlock" the menus
    if (pkt[6] == 0x06 && context->radType == IBUS_RADIO_TYPE_C43) {
        IBusCommandRADClearMenu(context->ibus);
        BMBTWriteHeader(context);
        if (context->menu != BMBT_MENU_NONE) {
            BMBTMenuRefresh(context);
        } else {
            BMBTWriteMenu(context);
        }
    }
}

/**
 * BMBTRADDisplayMenu()
 *     Description:
 *         This callback is triggered when the radio writes the "TONE" or
 *         "SEL" menus. This way, we can stop acting upon UI inputs until the
 *         screen is restored to our menu.
 *     Params:
 *         void *ctx - The context
 *         unsigned char *pkt - The IBus Message recieved
 *     Returns:
 *         void
 */
void BMBTRADDisplayMenu(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    context->displayMode = BMBT_DISPLAY_OFF;
}

/**
 * BMBTRADUpdateMainArea()
 *     Description:
 *         This callback is triggered when the radio writes the main area of
 *         the screen (Where the mode is usually displayed). In here, we can
 *         register our state as well as overwrite what the radio has written
 *         to the screen so the UI is always usable.
 *     Params:
 *         void *ctx - The context
 *         unsigned char *pkt - The IBus Message recieved
 *     Returns:
 *         void
 */
void BMBTRADUpdateMainArea(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t pktLen = (uint8_t) pkt[1] + 2;
    uint8_t textLen = pktLen - 7;
    char text[textLen];
    uint8_t idx = 0;
    uint8_t strIdx = 0;
    // Copy the text from the packet but avoid any preceding spaces
    while (strIdx < textLen) {
        char byte = pkt[strIdx + 6];
        if (byte > 0x20 || idx > 0) {
            text[idx] = byte;
            idx++;
        }
        strIdx++;
    }
    idx--;
    while (text[idx] == 0x20 || text[idx] == 0x00) {
        text[idx] = '\0';
        idx--;
    }
    text[textLen - 1] = '\0';
    if (pkt[4] == IBUS_C43_TITLE_MODE) {
        context->radType = IBUS_RADIO_TYPE_C43;
    }
    // Main area is being updated with a CDC Text:
    //     Rewrite the header and set display mode on
    if (strcmp("CDC 1-01", text) == 0 ||
        strcmp("TR 01-001", text) == 0 ||
        strcmp("TR 001-001", text) == 0 ||
        strcmp("CD 1-01", text) == 0 ||
        strcmp("CDC 1-0", text) == 0
    ) {
        context->mode = BMBT_MODE_ACTIVE;
        BMBTWriteHeader(context);
        if (context->displayMode == BMBT_DISPLAY_ON) {
            // Write the current menu back out
            if (context->menu != BMBT_MENU_NONE &&
                context->menu != BMBT_MENU_DASHBOARD_FRESH
            ) {
                if (context->radType == IBUS_RADIO_TYPE_C43) {
                    IBusCommandRADClearMenu(context->ibus);
                    BMBTWriteHeader(context);
                }
                BMBTMenuRefresh(context);
            } else {
                BMBTWriteMenu(context);
            }
        } else {
            // Disable the radio's menu alterations
            // after the screen has been written to so that we're not ignored
            IBusCommandRADDisableMenu(context->ibus);
            context->displayMode = BMBT_DISPLAY_ON;
        }
    } else if (strcmp("NO DISC", text) == 0 || strcmp("No Disc", text) == 0) {
        context->mode = BMBT_MODE_ACTIVE;
        context->displayMode = BMBT_DISPLAY_ON;
        if (context->radType == IBUS_RADIO_TYPE_C43) {
            IBusCommandRADClearMenu(context->ibus);
            BMBTWriteHeader(context);
            BMBTMenuRefresh(context);
        } else {
            if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF ||
                context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED
            ) {
                IBusCommandGTWriteTitle(context->ibus, "Bluetooth");
            } else {
                TimerTriggerScheduledTask(context->displayUpdateTaskId);
            }
        }
    } else if (pkt[pktLen - 2] == IBUS_RAD_MAIN_AREA_WATERMARK) {
        context->displayMode = BMBT_DISPLAY_ON;
    } else {
        context->menu = BMBT_MENU_NONE;
        context->displayMode = BMBT_DISPLAY_OFF;
    }
}

/**
 * BMBTScreenModeUpdate()
 *     Description:
 *         This callback tracks the screen mode that is broadcast by the GT.
 *         We use this to know if we can write to the screen or not.
 *     Params:
 *         void *ctx - The context
 *         unsigned char *pkt - The IBus Message recieved
 *     Returns:
 *         void
 */
void BMBTScreenModeUpdate(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[4] == 0x01 || pkt[4] == IBUS_GT_RADIO_SCREEN_OFF) {
        if (context->menu == BMBT_MENU_DASHBOARD) {
            context->menu = BMBT_MENU_DASHBOARD_FRESH;
        } else {
            context->menu = BMBT_MENU_NONE;
        }
        context->displayMode = BMBT_DISPLAY_OFF;
    }
    if (pkt[4] == IBUS_GT_TONE_SELECT_MENU_OFF &&
        context->mode == BMBT_MODE_ACTIVE &&
        context->displayMode == BMBT_DISPLAY_ON
    ) {
        // Write the current menu back out
        if (context->menu == BMBT_MENU_NONE ||
            context->menu == BMBT_MENU_DASHBOARD_FRESH
        ) {
            BMBTWriteMenu(context);
        } else {
            BMBTMenuRefresh(context);
        }
    }
}

/**
 * BMBTScreenModeSet()
 *     Description:
 *         The GT sends this screen mode post-boot to tell the radio it can
 *         display to the UI. We set the menu to none so that on the next
 *         screen clear, we know to write the UI
 *     Params:
 *         void *ctx - The context
 *         unsigned char *pkt - The IBus Message recieved
 *     Returns:
 *         void
 */
void BMBTScreenModeSet(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[4] == BMBT_NAV_BOOT) {
        context->menu = BMBT_MENU_NONE;
    }
}

/**
 * BMBTTimerMenuWrite()
 *     Description:
 *         If more than the elapsed time has passed since the screen was set to
 *         CDC mode but we haven't gotten a screen clear from the radio, write
 *         out the menu, otherwise we may never write the menu out.
 *     Params:
 *         void *ctx - The context
 *     Returns:
 *         void
 */
void BMBTTimerMenuWrite(void *ctx)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if ((context->menu == BMBT_MENU_NONE ||
         context->menu == BMBT_MENU_DASHBOARD_FRESH) &&
        context->displayMode == BMBT_DISPLAY_ON &&
        context->ibus->ignitionStatus == IBUS_IGNITION_ON
    ) {
        uint16_t time = context->timerMenuIntervals * BMBT_MENU_TIMER_WRITE_INT;
        if (time >= BMBT_MENU_TIMER_WRITE_TIMEOUT) {
            IBusCommandRADDisableMenu(context->ibus);
            BMBTWriteMenu(context);
            context->timerMenuIntervals = 0;
        } else {
            context->timerMenuIntervals++;
        }
    }
}

/**
 * BMBTTimerScrollDisplay()
 *     Description:
 *         Write the scrolling display
 *     Params:
 *         void *ctx - The context
 *     Returns:
 *         void
 */
void BMBTTimerScrollDisplay(void *ctx)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->mode == BMBT_MODE_ACTIVE &&
        context->displayMode == BMBT_DISPLAY_ON &&
        ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) != CONFIG_SETTING_OFF
    ) {
        // Display the main text if there isn't a timeout set
        if (context->mainDisplay.timeout > 0) {
            context->mainDisplay.timeout--;
        } else {
            if (context->mainDisplay.length > 9) {
                char text[10];
                strncpy(
                    text,
                    &context->mainDisplay.text[context->mainDisplay.index],
                    9
                );
                text[9] = '\0';
                IBusCommandGTWriteTitle(context->ibus, text);
                // Pause at the beginning of the text
                if (context->mainDisplay.index == 0) {
                    context->mainDisplay.timeout = 5;
                }
                uint8_t idxEnd = context->mainDisplay.index + 9;
                if (idxEnd >= context->mainDisplay.length) {
                    // Pause at the end of the text
                    context->mainDisplay.timeout = 2;
                    context->mainDisplay.index = 0;
                } else {
                    if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) ==
                        BMBT_METADATA_MODE_CHUNK
                    ) {
                        context->mainDisplay.timeout = 2;
                        context->mainDisplay.index += 9;
                    } else {
                        context->mainDisplay.index++;
                    }
                }
            } else {
                if (context->mainDisplay.index == 0) {
                    IBusCommandGTWriteTitle(
                        context->ibus,
                        context->mainDisplay.text
                    );
                }
                context->mainDisplay.index = 1;
            }
        }
    }
}
