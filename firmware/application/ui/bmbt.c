/*
 * File: bmbt.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the BoardMonitor UI Mode handler
 */
#include "bmbt.h"
static BMBTContext_t Context;

static const uint8_t menuSettings[] = {
    BMBT_MENU_IDX_SETTINGS_ABOUT,
    BMBT_MENU_IDX_SETTINGS_AUDIO,
    BMBT_MENU_IDX_SETTINGS_CALLING,
    BMBT_MENU_IDX_SETTINGS_COMFORT,
    BMBT_MENU_IDX_SETTINGS_UI
};

static const uint16_t menuSettingsLabelIndices[] = {
    LOCALE_STRING_ABOUT,
    LOCALE_STRING_AUDIO,
    LOCALE_STRING_CALLING,
    LOCALE_STRING_COMFORT,
    LOCALE_STRING_UI
};

// Map Auto-zoom, max speed (km/h) per scale
static const uint16_t navZoomSpeeds[IBUS_SES_ZOOM_LEVELS] = {
     1,  // 125 - special case when stationary, do not edit the "1"
    10,  // 125
    60,  // 250
    90,  // 450
    120, // 900
    150, // 1
    999, // 2.5
    999  // 5
};

// Map Auto-zoom, scales
static const char *navZoomScaleImperial[IBUS_SES_ZOOM_LEVELS] = {
    "125yd",
    "125yd",
    "250yd",
    "450yd",
    "900yd",
    "1mi",
    "2.5mi",
    "5mi"
};

// Map Auto-zoom, scales
static const char *navZoomScaleMetric[IBUS_SES_ZOOM_LEVELS] = {
    "100m",
    "100m",
    "200m",
    "500m",
    "1km",
    "2km",
    "5km",
    "10km"
};

void BMBTInit(BT_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    Context.menu = BMBT_MENU_NONE;
    Context.status.playerMode = BMBT_MODE_INACTIVE;
    Context.status.displayMode = BMBT_DISPLAY_OFF;
    Context.status.navState = BMBT_NAV_STATE_ON;
    Context.status.radType = IBUS_RADIO_TYPE_BM53;
    Context.status.tvStatus = BMBT_TV_STATUS_OFF;
    Context.status.navIndexType = IBUS_CMD_GT_WRITE_INDEX_TMC;
    Context.timerHeaderIntervals = BMBT_MENU_HEADER_TIMER_OFF;
    Context.timerMenuIntervals = BMBT_MENU_HEADER_TIMER_OFF;
    Context.mainDisplay = UtilsDisplayValueInit(LocaleGetText(LOCALE_STRING_BLUETOOTH), BMBT_DISPLAY_OFF);
    Context.navZoom = -1;
    Context.navZoomTime = 0;

    EventRegisterCallback(
        BT_EVENT_DEVICE_CONNECTED,
        &BMBTBTDeviceConnected,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &BMBTBTDeviceDisconnected,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_METADATA_UPDATE,
        &BMBTBTMetadata,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_BOOT,
        &BMBTBTReady,
        &Context
    );
    EventRegisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &BMBTBTPlaybackStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_BMBTButton,
        &BMBTIBusBMBTButtonPress,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &BMBTIBusCDChangerStatus,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_GTChangeUIRequest,
        &BMBTIBusGTChangeUIRequest,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_GTMenuSelect,
        &BMBTIBusMenuSelect,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_SCREEN_BUFFER_FLUSH,
        &BMBTIBusScreenBufferFlush,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_SENSOR_VALUE_UPDATE,
        &BMBTIBusSensorValueUpdate,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_RADDisplayMenu,
        &BMBTRADDisplayMenu,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_RAD_WRITE_DISPLAY,
        &BMBTRADUpdateMainArea,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_ScreenModeSet,
        &BMBTGTScreenModeSet,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_ScreenModeUpdate,
        &BMBTRADScreenModeRequest,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_TV_STATUS,
        &BMBTTVStatusUpdate,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKE_VEHICLE_CONFIG,
        &BMBTIBusVehicleConfig,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKESpeedRPMUpdate,
        &BMBTIKESpeedRPMUpdate,
        &Context
    );
    Context.headerWriteTaskId = TimerRegisterScheduledTask(
        &BMBTTimerHeaderWrite,
        &Context,
        BMBT_HEADER_TIMER_WRITE_INT
    );
    Context.menuWriteTaskId = TimerRegisterScheduledTask(
        &BMBTTimerMenuWrite,
        &Context,
        BMBT_MENU_TIMER_WRITE_INT
    );
    Context.displayUpdateTaskId = TimerRegisterScheduledTask(
        &BMBTTimerScrollDisplay,
        &Context,
        BMBT_SCROLL_TEXT_TIMER
    );
}

/**
 * BMBTDestroy()
 *     Description:
 *         Unregister all event handlers, scheduled tasks and clear the context
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void BMBTDestroy()
{
    EventUnregisterCallback(
        BT_EVENT_DEVICE_CONNECTED,
        &BMBTBTDeviceConnected
    );
    EventUnregisterCallback(
        BT_EVENT_DEVICE_LINK_DISCONNECTED,
        &BMBTBTDeviceDisconnected
    );
    EventUnregisterCallback(
        BT_EVENT_METADATA_UPDATE,
        &BMBTBTMetadata
    );
    EventUnregisterCallback(
        BT_EVENT_BOOT,
        &BMBTBTReady
    );
    EventUnregisterCallback(
        BT_EVENT_PLAYBACK_STATUS_CHANGE,
        &BMBTBTPlaybackStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_BMBTButton,
        &BMBTIBusBMBTButtonPress
    );
    EventUnregisterCallback(
        IBUS_EVENT_CDStatusRequest,
        &BMBTIBusCDChangerStatus
    );
    EventUnregisterCallback(
        IBUS_EVENT_SCREEN_BUFFER_FLUSH,
        &BMBTIBusScreenBufferFlush
    );
    EventUnregisterCallback(
        IBUS_EVENT_SENSOR_VALUE_UPDATE,
        &BMBTIBusSensorValueUpdate
    );
    EventUnregisterCallback(
        IBUS_EVENT_GTChangeUIRequest,
        &BMBTIBusGTChangeUIRequest
    );
    EventUnregisterCallback(
        IBUS_EVENT_GTMenuSelect,
        &BMBTIBusMenuSelect
    );
    EventUnregisterCallback(
        IBUS_EVENT_RADDisplayMenu,
        &BMBTRADDisplayMenu
    );
    EventUnregisterCallback(
        IBUS_EVENT_RAD_WRITE_DISPLAY,
        &BMBTRADUpdateMainArea
    );
    EventUnregisterCallback(
        IBUS_EVENT_ScreenModeSet,
        &BMBTGTScreenModeSet
    );
    EventUnregisterCallback(
        IBUS_EVENT_ScreenModeUpdate,
        &BMBTRADScreenModeRequest
    );
    EventUnregisterCallback(
        IBUS_EVENT_TV_STATUS,
        &BMBTTVStatusUpdate
    );
    EventUnregisterCallback(
        IBUS_EVENT_IKE_VEHICLE_CONFIG,
        &BMBTIBusVehicleConfig
    );
    TimerUnregisterScheduledTask(&BMBTTimerHeaderWrite);
    TimerUnregisterScheduledTask(&BMBTTimerMenuWrite);
    TimerUnregisterScheduledTask(&BMBTTimerScrollDisplay);
    memset(&Context, 0, sizeof(BMBTContext_t));
}

/**
 * BMBTMenuRefresh()
 *     Description:
 *         Trigger the scheduled task to rewrite the main area. If the text
 *         fits on the screen, reset the index so it is written again
 *     Params:
 *         BMBTContext_t *context - The BMBT context
 *     Returns:
 *         void
 */
static void BMBTMainAreaRefresh(BMBTContext_t *context)
{
    if (context->mainDisplay.length <= 9) {
        context->mainDisplay.index = 0;
    }
    TimerTriggerScheduledTask(context->displayUpdateTaskId);
}

/**
 * BMBTMenuRefresh()
 *     Description:
 *         Wrapper to send a menu refresh call to the GT that handles the
 *         old and new style UIs as well as the static screen support
 *     Params:
 *         BMBTContext_t *context - The BMBT context
 *     Returns:
 *         void
 */
static void BMBTMenuRefresh(BMBTContext_t *context)
{
    if ((context->menu != BMBT_MENU_DASHBOARD &&
        context->menu != BMBT_MENU_DASHBOARD_FRESH) ||
        context->ibus->gtVersion != IBUS_GT_MKIV_STATIC
    ) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    } else {
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_STATIC);
    }
}

static void BMBTSetMainDisplayText(
    BMBTContext_t *context,
    const char *str,
    int8_t timeout,
    uint8_t autoUpdate
) {
    memset(context->mainDisplay.text, 0, UTILS_DISPLAY_TEXT_SIZE);
    UtilsStrncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE);
    context->mainDisplay.text[UTILS_DISPLAY_TEXT_SIZE - 1] = '\0';
    context->mainDisplay.length = strlen(context->mainDisplay.text);
    context->mainDisplay.index = 0;
    if (autoUpdate == 1) {
        TimerTriggerScheduledTask(context->displayUpdateTaskId);
    }
    context->mainDisplay.timeout = timeout;
}

/**
 * BMBTTriggerWriteHeader()
 *     Description:
 *         Trigger the counter that fires off our header field writing
 *         timer. If the counter has already been triggered, do nothing.
 *     Params:
 *         BMBTContext_t *context - The context
 *     Returns:
 *         void
 */
static void BMBTTriggerWriteHeader(BMBTContext_t *context)
{
    if (context->timerHeaderIntervals == BMBT_MENU_HEADER_TIMER_OFF) {
        TimerResetScheduledTask(context->headerWriteTaskId);
        context->timerHeaderIntervals = 0;
    }
}

/**
 * BMBTTriggerWriteMenu()
 *     Description:
 *         Trigger the counter that fires off our menu writing
 *         timer. If the counter has already been triggered, do nothing.
 *     Params:
 *         BMBTContext_t *context - The context
 *     Returns:
 *         void
 */
static void BMBTTriggerWriteMenu(BMBTContext_t *context)
{
    // If we can refresh the last menu back onto the screen,
    // do so immediately. Otherwise, trigger the menu write timer
    if (context->menu == BMBT_MENU_NONE ||
        context->menu == BMBT_MENU_DASHBOARD_FRESH ||
        context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI ||
        context->status.radType == IBUS_RADIO_TYPE_C43
    ) {
        if (context->timerMenuIntervals == BMBT_MENU_HEADER_TIMER_OFF) {
            TimerResetScheduledTask(context->menuWriteTaskId);
            context->timerMenuIntervals = 0;
        }
    } else {
        BMBTMenuRefresh(context);
    }
}

/**
 * BMBTHeaderWriteDeviceName()
 *     Description:
 *         Wrapper to extend the length of the device field to 20 characters
 *         with space padding if we are writing to the old style UI.
 *     Params:
 *         BMBTContext_t *context - The context
 *         char *text - The text to write
 *     Returns:
 *         void
 */
static void BMBTHeaderWriteDeviceName(BMBTContext_t *context, char *text)
{
    char cleanName[21];
    memset(cleanName, 0x20, 21);
    memcpy(cleanName, text, 21);
    if (context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI) {
        cleanName[20] = '\0';
    } else {
        cleanName[15] = '\0';
    }
    IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_DEV_NAME, cleanName);
}

/**
 * BMBTGTWriteIndex()
 *     Description:
 *         Wrapper to automatically push the nav type into the I-Bus Library
 *         Command so that we can save verbosity in these calls
 *     Params:
 *         BMBTContext_t *context - The context
 *         uint8_t index - The index to write to
 *         char *text - The text to write
 *         uint8_t clearIdxs - Number of additional rows to clear
 *     Returns:
 *         void
 */
static void BMBTGTWriteIndex(
    BMBTContext_t *context,
    uint8_t index,
    char *text,
    uint8_t clearIdxs
) {
    context->status.navIndexType = IBUS_CMD_GT_WRITE_INDEX_TMC;
    if (clearIdxs > 0) {
        uint8_t stringLength = strlen(text);
        uint8_t newTextLength = stringLength + clearIdxs + 1;
        char newText[newTextLength];
        memset(&newText, 0, newTextLength);
        strncpy(newText, text, stringLength);
        while (stringLength < newTextLength) {
            newText[stringLength] = 0x06;
            stringLength++;
        }
        newText[newTextLength - 1] = '\0';
        IBusCommandGTWriteIndexTMC(context->ibus, index, newText);
    } else {
        IBusCommandGTWriteIndexTMC(context->ibus, index, text);
    }
}

/**
 * BMBTGTWriteTitle()
 *     Description:
 *         Wrapper to automatically account for the nav type when
 *         writing the title area
 *     Params:
 *         BMBTContext_t *context - The context
 *         char *text - The text to write
 *     Returns:
 *         void
 */
static void BMBTGTWriteTitle(BMBTContext_t *context, char *text)
{
    if (context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI) {
        IBusCommandGTWriteTitleArea(context->ibus, text);
    } else {
        IBusCommandGTWriteTitleIndex(context->ibus, text);
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    }
}

static void BMBTHeaderWrite(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF ||
        context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED
    ) {
        BMBTGTWriteTitle(context, LocaleGetText(LOCALE_STRING_BLUETOOTH));
    } else {
        BMBTMainAreaRefresh(context);
    }
    if (strlen(context->bt->activeDevice.deviceName) > 0) {
        BMBTHeaderWriteDeviceName(context, context->bt->activeDevice.deviceName);
    } else {
        BMBTHeaderWriteDeviceName(context, LocaleGetText(LOCALE_STRING_NO_DEVICE));
    }
    if (context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED) {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
    } else {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "> ");
    }
    uint8_t tempMode = ConfigGetTempDisplay();
    // Clear the "CD1" Header
    IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_BT, "    ");
    IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    uint8_t valueType = 0;
    switch (tempMode) {
        case CONFIG_SETTING_TEMP_COOLANT:
            valueType = IBUS_SENSOR_VALUE_COOLANT_TEMP;
            break;
        case CONFIG_SETTING_TEMP_AMBIENT:
            valueType = IBUS_SENSOR_VALUE_AMBIENT_TEMP;
            break;
        case CONFIG_SETTING_TEMP_OIL:
            valueType = IBUS_SENSOR_VALUE_OIL_TEMP;
            break;
    }
    if (valueType > 0) {
        BMBTIBusSensorValueUpdate((void *)context, &valueType);
    }
}

static void BMBTMenuMain(BMBTContext_t *context)
{
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_DASHBOARD, LocaleGetText(LOCALE_STRING_DASHBOARD), 0);
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_DEVICE_SELECTION, LocaleGetText(LOCALE_STRING_DEVICES), 0);
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_SETTINGS, LocaleGetText(LOCALE_STRING_SETTINGS), 6);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_MAIN_MENU));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_MAIN;
}

static void BMBTMenuDashboardUpdateOBCValues(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_BMBT_DASHBOARD_OBC_ADDRESS) != CONFIG_SETTING_OFF) {
        char tempUnit = 'C';
        char ambtempstr[8] = {0};
        char oiltempstr[7] = {0};
        char cooltempstr[7] = {0};

        if (ConfigGetTempUnit() == CONFIG_SETTING_TEMP_FAHRENHEIT) {
            tempUnit = 'F';
        }

        int ambtemp = context->ibus->ambientTemperature;
        int oiltemp = context->ibus->oilTemperature;
        int cooltemp = context->ibus->coolantTemperature;

        if (tempUnit == 'F') {
            ambtemp = (ambtemp * 1.8 + 32 + 0.5);
            if (oiltemp > 0) {
                oiltemp = (oiltemp * 1.8 + 32 + 0.5);
            }
            if (cooltemp > 0) {
                cooltemp = (cooltemp * 1.8 + 32 + 0.5);
            }
        }

        char temperature[29] = {0};

        if (context->ibus->ambientTemperatureCalculated[0] != 0x00) {
            snprintf(ambtempstr, 8, "A:%s", context->ibus->ambientTemperatureCalculated);
        } else {
            snprintf(ambtempstr, 8, "A:%+d", ambtemp);
        }
        if (cooltemp > 0) {
            snprintf(cooltempstr, 7, "C:%d,", cooltemp);
        }
        if (oiltemp > 0) {
            snprintf(oiltempstr, 7, "O:%d,", oiltemp);
            snprintf(temperature, 29, "%s%s%s\xB0%c", oiltempstr, cooltempstr, ambtempstr, tempUnit);
        } else {
            snprintf(temperature, 29, "Temp\xB0%c: %s%s", tempUnit, cooltempstr, ambtempstr);
        }

        if (context->ibus->gtVersion == IBUS_GT_MKIV_STATIC) {
            IBusCommandGTWriteIndexStatic(context->ibus, 0x45, temperature);
        } else {
            IBusCommandGTWriteIndex(context->ibus, 4, temperature);
        }
    } else {
        // Clear the OBC index if we're not configured for it
        char emptyIndex[2] = {0x06, 0x00};
        if (context->ibus->gtVersion == IBUS_GT_MKIV_STATIC) {
            IBusCommandGTWriteIndexStatic(context->ibus, 0x45, emptyIndex);
        } else {
            IBusCommandGTWriteIndex(context->ibus, 4, emptyIndex);
        }
    }
}

static void BMBTMenuDashboardUpdate(BMBTContext_t *context, char *f1, char *f2, char *f3)
{
    if (strlen(f1) == 0) {
        strncpy(f1, " ", 2);
    }

    // Prevent duplication of fields
    if (strlen(f2) == 0 && strlen(f3) == 0) {
        strncpy(f2, " ", 2);
        strncpy(f3, " ", 2);
    } else if (strlen(f2) == 0 && strlen(f3) != 0) {
        char *tmp = f2;
        f2 = f3;
        f3 = tmp;
        strncpy(f3, " ", 2);
        if (strncmp(f1, f2, BT_METADATA_FIELD_SIZE) == 0) {
            strncpy(f2, " ", 2);
        }
    } else if (strlen(f3) == 0) {
        strncpy(f3, " ", 2);
        if (strncmp(f1, f2, BT_METADATA_FIELD_SIZE) == 0) {
            strncpy(f2, " ", 2);
        }
    } else if (strlen(f2) != 0 && strncmp(f2, f3, BT_METADATA_FIELD_SIZE) == 0) {
        strncpy(f3, " ", 2);
        if (strncmp(f1, f2, BT_METADATA_FIELD_SIZE) == 0) {
            strncpy(f2, " ", 2);
        }
    }

    if (context->ibus->gtVersion == IBUS_GT_MKIV_STATIC) {
        IBusCommandGTWriteIndexStatic(context->ibus, 0x41, f1);
        IBusCommandGTWriteIndexStatic(context->ibus, 0x42, f2);
        IBusCommandGTWriteIndexStatic(context->ibus, 0x43, f3);
        BMBTMenuDashboardUpdateOBCValues(context);
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_STATIC);
    } else {
        IBusCommandGTWriteIndex(context->ibus, 0, f1);
        IBusCommandGTWriteIndex(context->ibus, 1, f2);
        // Clear the rest of the screen by adding seven 0x06 chars to the end
        // of the last message written to the screen, so the GT clears those
        // seven indices. The 8th index is simply to hold the null terminator.
        uint8_t f3Len = strlen(f3);
        uint8_t newLength = f3Len + 8;
        char newF3[newLength];
        memset(newF3, 0x06, newLength);
        strncpy(newF3, f3, f3Len);
        newF3[newLength - 1] = 0x00;
        IBusCommandGTWriteIndex(context->ibus, 2, newF3);
        BMBTMenuDashboardUpdateOBCValues(context);
        context->status.navIndexType = IBUS_CMD_GT_WRITE_INDEX;
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

static void BMBTMenuDashboard(BMBTContext_t *context)
{
    char title[BT_METADATA_FIELD_SIZE] = {0};
    char artist[BT_METADATA_FIELD_SIZE] = {0};
    char album[BT_METADATA_FIELD_SIZE] = {0};
    UtilsStrncpy(title, context->bt->title, BT_METADATA_FIELD_SIZE);
    uint8_t metadataKnown = 1;
    if (context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED) {
        if (strlen(title) == 0) {
            UtilsStrncpy(
                title,
                LocaleGetText(LOCALE_STRING_NOT_PLAYING),
                BT_METADATA_FIELD_SIZE
            );
            metadataKnown = 0;
        }
    } else {
        // Set "Unknown" text for title and artist when missing but ignore
        // missing album or artist information as many streaming apps do not provide it
        if (strlen(title) == 0) {
            UtilsStrncpy(
                title,
                LocaleGetText(LOCALE_STRING_UNKNOWN_TITLE),
                BT_METADATA_FIELD_SIZE
            );
        }
    }
    // Only copy the data if we need it
    if (metadataKnown == 1) {
        UtilsStrncpy(artist, context->bt->artist, BT_METADATA_FIELD_SIZE);
        UtilsStrncpy(album, context->bt->album, BT_METADATA_FIELD_SIZE);
    }
    BMBTMenuDashboardUpdate(context, title, artist, album);
    context->menu = BMBT_MENU_DASHBOARD;
}

static void BMBTMenuDeviceSelection(BMBTContext_t *context)
{
    uint8_t idx;
    uint8_t screenIdx = 2;
    if (context->bt->discoverable == BT_STATE_ON) {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_ON), 0);
    } else {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_OFF), 0);
    }
    BTPairedDevice_t *dev = 0;
    uint8_t devicesCount = 0;
    for (idx = 0; idx < context->bt->pairedDevicesCount; idx++) {
        dev = &context->bt->pairedDevices[idx];
        if (dev != 0) {
            devicesCount++;
        }
    }
    if (devicesCount == 0) {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_CLEAR_PAIRING, LocaleGetText(LOCALE_STRING_CLEAR_PAIRINGS), 4);
    } else {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_CLEAR_PAIRING, LocaleGetText(LOCALE_STRING_CLEAR_PAIRINGS), 0);
    }
    for (idx = 0; idx < context->bt->pairedDevicesCount; idx++) {
        dev = &context->bt->pairedDevices[idx];
        if (dev != 0) {
            if (devicesCount > 0) {
                devicesCount--;
            }
            char deviceName[23] = {0};
            strncpy(deviceName, dev->deviceName, 11);
            deviceName[22] = '\0';
            // Add a space and asterisks to the end of the device name
            // if it's the currently selected device
            if (memcmp(dev->macId, context->bt->activeDevice.macId, BT_LEN_MAC_ID) == 0) {
                uint8_t startIdx = strlen(deviceName);
                if (startIdx > 20) {
                    startIdx = 20;
                }
                deviceName[startIdx++] = 0x20;
                deviceName[startIdx++] = 0x2A;
            }
            if (devicesCount == 0) {
                uint8_t feedCount = 6 - screenIdx;
                BMBTGTWriteIndex(context, screenIdx, deviceName, feedCount);
            } else {
                BMBTGTWriteIndex(context, screenIdx, deviceName, 0);
            }
            screenIdx++;
        }
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_DEVICES));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_DEVICE_SELECTION;
}

static void BMBTMenuSettings(BMBTContext_t *context)
{
    uint8_t menuSettingsSize = sizeof(menuSettings);
    uint8_t idx;
    for (idx = 0; idx < menuSettingsSize; idx++) {
        uint8_t feedCount = 0;
        if (idx == (menuSettingsSize - 1)) {
            feedCount = BMBT_MENU_IDX_BACK - menuSettingsSize;
        }
        BMBTGTWriteIndex(
            context,
            menuSettings[idx],
            LocaleGetText(menuSettingsLabelIndices[idx]),
            feedCount
        );
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS;
}

static void BMBTMenuSettingsAbout(BMBTContext_t *context)
{
    char version[9];
    ConfigGetFirmwareVersionString(version);
    char versionString[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(versionString, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_FW), version);
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_ABOUT_FW_VERSION,
        versionString,
        0
    );
    char buildString[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(buildString, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_BUILT), ConfigGetBuildWeek(), ConfigGetBuildYear());
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_ABOUT_BUILD_DATE,
        buildString,
        0
    );
    char serialNumberString[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(serialNumberString, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_SN), ConfigGetSerialNumber());
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_ABOUT_SERIAL,
        serialNumberString,
        2
    );
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_ABOUT));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_ABOUT;
}

static void BMBTMenuSettingsAudio(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_AUTOPLAY,
            LocaleGetText(LOCALE_STRING_AUTOPLAY_OFF),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_AUTOPLAY,
            LocaleGetText(LOCALE_STRING_AUTOPLAY_ON),
            0
        );
    }
    uint8_t currentVolume = ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL);
    char volText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    if (currentVolume > 0x30) {
        uint8_t gain = (currentVolume - 0x30) / 2;
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_NEG_DB), gain);
    } else if (currentVolume == 0) {
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_24_DB));
    } else if (currentVolume == 0x30) {
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_0_DB));
    } else {
        uint8_t gain = (0x30 - currentVolume) / 2;
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_POS_DB), gain);
    }
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_AUDIO_DAC_GAIN,
        volText,
        0
    );
    uint8_t dspInput = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
    if (dspInput == CONFIG_SETTING_DSP_INPUT_SPDIF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT,
            LocaleGetText(LOCALE_STRING_DSP_DIGITAL),
            0
        );
    } else if (dspInput == CONFIG_SETTING_DSP_INPUT_ANALOG) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT,
            LocaleGetText(LOCALE_STRING_DSP_ANALOG),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT,
            LocaleGetText(LOCALE_STRING_DSP_DEFAULT),
            0
        );
    }
    if (ConfigGetSetting(CONFIG_SETTING_MANAGE_VOLUME) == CONFIG_SETTING_ON) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_MANAGE_VOL,
            LocaleGetText(LOCALE_STRING_MANAGE_VOL_ON),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_MANAGE_VOL,
            LocaleGetText(LOCALE_STRING_MANAGE_VOL_OFF),
            0
        );
    }
    if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_ON) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_REV_VOL,
            LocaleGetText(LOCALE_STRING_REV_VOL_LOW_ON),
            2
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_REV_VOL,
            LocaleGetText(LOCALE_STRING_REV_VOL_LOW_OFF),
            2
        );
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_AUDIO));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_AUDIO;
}

static void BMBTMenuSettingsComfort(BMBTContext_t *context)
{
    uint8_t comfortLock = ConfigGetComfortLock();
    if (comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_LOCK,
            LocaleGetText(LOCALE_STRING_LOCK_10KMH),
            0
        );
    } else if (comfortLock == CONFIG_SETTING_COMFORT_LOCK_20KM) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_LOCK,
            LocaleGetText(LOCALE_STRING_LOCK_20KMH),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_LOCK,
            LocaleGetText(LOCALE_STRING_LOCK_OFF),
            0
        );
    }
    uint8_t comfortUnlock = ConfigGetComfortUnlock();
    if (comfortUnlock == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_UNLOCK,
            LocaleGetText(LOCALE_STRING_UNLOCK_POS_1),
            0
        );
    } else if (comfortUnlock == CONFIG_SETTING_COMFORT_UNLOCK_POS_0) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_UNLOCK,
            LocaleGetText(LOCALE_STRING_UNLOCK_POS_0),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_UNLOCK,
            LocaleGetText(LOCALE_STRING_UNLOCK_OFF),
            0
        );
    }
    uint8_t blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
    if (blinkCount == 0) {
        blinkCount = 1;
    }
    char blinkerText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(blinkerText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_BLINKERS), blinkCount);
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_COMFORT_BLINKERS,
        blinkerText,
        0
    );
    if (ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS) == CONFIG_SETTING_ON) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_PARKING_LAMPS,
            LocaleGetText(LOCALE_STRING_PARK_LAMPS_ON),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_PARKING_LAMPS,
            LocaleGetText(LOCALE_STRING_PARK_LAMPS_OFF),
            0
        );
    }
    uint8_t autozoom = ConfigGetSetting(CONFIG_SETTING_COMFORT_AUTOZOOM);
    if (autozoom >= IBUS_SES_ZOOM_LEVELS) {
        autozoom = CONFIG_SETTING_OFF;
    }
    char autoZoomText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    if (autozoom == CONFIG_SETTING_OFF) {
        snprintf(
            autoZoomText,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_AUTOZOOM),
            "Off"
        );
    } else {
        if (ConfigGetDistUnit() == 0) {
            snprintf(
                autoZoomText,
                BMBT_MENU_STRING_MAX_SIZE,
                LocaleGetText(LOCALE_STRING_AUTOZOOM),
                navZoomScaleMetric[autozoom]
            );
        } else {
            snprintf(
                autoZoomText,
                BMBT_MENU_STRING_MAX_SIZE,
                LocaleGetText(LOCALE_STRING_AUTOZOOM),
                navZoomScaleImperial[autozoom]
            );
        }
    }
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_COMFORT_AUTOZOOM,
        autoZoomText,
        0
    );

    uint8_t autotime = ConfigGetTimeSource();
    char autotime_text[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(
        autotime_text,
        BMBT_MENU_STRING_MAX_SIZE,
        LocaleGetText(LOCALE_STRING_AUTOTIME),
        (
            (autotime == CONFIG_SETTING_TIME_PHONE) ? "iPhone":
            (autotime == CONFIG_SETTING_TIME_GPS) ? "GPS":
            LocaleGetText(LOCALE_STRING_AUTOTIME_MANUAL)
        )
    );

    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_COMFORT_TIME,
        autotime_text,
        0
    );

    uint8_t pdc = ConfigGetSetting(CONFIG_SETTING_COMFORT_PDC);
    char pdc_text[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(
        pdc_text,
        BMBT_MENU_STRING_MAX_SIZE,
        LocaleGetText(LOCALE_STRING_PDC),
        (
            (pdc == CONFIG_SETTING_OFF) ? "Off":
            (pdc == CONFIG_SETTING_PDC_CLUSTER)? "Cluster":
            (pdc == CONFIG_SETTING_PDC_RADIO)? "Screen":
            "Dual"
        )
    );

    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_COMFORT_PDC,
        pdc_text,
        0
    );


    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_COMFORT));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_COMFORT;
}

static void BMBTMenuSettingsComfortTime(BMBTContext_t *context)
{
    char text[BMBT_MENU_STRING_MAX_SIZE] = {0};
    uint8_t autotime = ConfigGetTimeSource();
    snprintf(
        text,
        BMBT_MENU_STRING_MAX_SIZE,
        LocaleGetText(LOCALE_STRING_AUTOTIME),
        (
            (autotime == CONFIG_SETTING_TIME_PHONE) ? "iPhone":
            (autotime == CONFIG_SETTING_TIME_GPS) ? "GPS":
            LocaleGetText(LOCALE_STRING_AUTOTIME_MANUAL)
        )
    );

    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_SOURCE,
        text,
        (autotime == CONFIG_SETTING_TIME_GPS)? 0:6
    );

    if (autotime == CONFIG_SETTING_TIME_GPS) {
        uint8_t dst = ConfigGetTimeDST();
        int16_t off = ConfigGetTimeOffset();

        snprintf(
            text,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_AUTOTIME_DST),
            (dst == 0) ? "---":"+01:00"
        );
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_DST,
            text,
            0
        );

        snprintf(
            text,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_AUTOTIME_OFFSET),
            off / 60,
            abs(off) % 60
        );
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_OFFSET,
            text,
            1
        );

        if (context->ibus->gpsTime != 0) {
            struct tm *ptm;
            ptm = gmtime(&context->ibus->gpsTime);

            snprintf(
            text,
            BMBT_MENU_STRING_MAX_SIZE,
            "GPSD: %2d.%2d.%4d",
            ptm->tm_mday,
            ptm->tm_mon+1,
            ptm->tm_year+1900
        );

        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_GPSDATE,
            text,
            0
        );

        snprintf(
            text,
            BMBT_MENU_STRING_MAX_SIZE,
            "GPST: %02d:%02d",
            ptm->tm_hour,
            ptm->tm_min
        );

        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_GPSTIME,
                text,
                1
            );
        } else {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_GPSDATE,
                LocaleGetText(LOCALE_STRING_AUTOTIME_NOSIGNAL),
                2
            );
        }
    }

    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_COMFORT_TIME));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_COMFORT_TIME;
}

static void BMBTMenuSettingsCalling(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_HFP,
            LocaleGetText(LOCALE_STRING_HANDSFREE_OFF),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_HFP,
            LocaleGetText(LOCALE_STRING_HANDSFREE_ON),
            0
        );
    }
    uint8_t micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
    char micGainText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    if (context->bt->type == BT_BTM_TYPE_BC127) {
        if (micGain > 21) {
            micGain = 0;
        }
        snprintf(
            micGainText,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_MIC_GAIN),
            (int8_t) BTBC127MicGainTable[micGain]
        );
    } else {
        if (micGain > 0x0F) {
            micGain = 0;
        }
        snprintf(
            micGainText,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_MIC_GAIN),
            (int8_t) BTBM83MicGainTable[micGain]
        );
    }
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN,
        micGainText,
        0
    );
    uint8_t volOffsetSkip = 0;
    if (context->bt->type == BT_BTM_TYPE_BC127) {
        volOffsetSkip = 3;
    }
    int8_t volumeOffset = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
    char volOffsetText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(
        volOffsetText,
        BMBT_MENU_STRING_MAX_SIZE,
        LocaleGetText(LOCALE_STRING_VOL_OFFSET),
        volumeOffset
    );
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_CALLING_VOL_OFFSET,
        volOffsetText,
        volOffsetSkip
    );
    // Hide TCU Mode option on HW Version 1. It is not necessary there.
    if (context->bt->type != BT_BTM_TYPE_BC127) {
        uint8_t telMode = ConfigGetSetting(CONFIG_SETTING_TEL_MODE);
        if (telMode == CONFIG_SETTING_TEL_MODE_TCU) {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_TCU),
                2
            );
        } else if (telMode == CONFIG_SETTING_TEL_MODE_NO_MUTE) {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_NO_MUTE),
                2
            );
        } else {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_DEFAULT),
                2
            );
        }
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_CALLING));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_CALLING;
}

static void BMBTMenuSettingsUI(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU,
            LocaleGetText(LOCALE_STRING_MENU_MAIN),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU,
            LocaleGetText(LOCALE_STRING_MENU_DASHBOARD),
            0
        );
    }
    uint8_t metadataMode = ConfigGetSetting(CONFIG_SETTING_METADATA_MODE);
    if (metadataMode == BMBT_METADATA_MODE_PARTY) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_METADATA_MODE,
            LocaleGetText(LOCALE_STRING_METADATA_PARTY),
            0
        );
    } else if (metadataMode == BMBT_METADATA_MODE_CHUNK) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_METADATA_MODE,
            LocaleGetText(LOCALE_STRING_METADATA_CHUNK),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_METADATA_MODE,
            LocaleGetText(LOCALE_STRING_METADATA_OFF),
            0
        );
    }
    uint8_t tempMode = ConfigGetTempDisplay();
    if (tempMode == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_TEMPS,
            LocaleGetText(LOCALE_STRING_TEMPS_OFF),
            0
        );
    } else if (tempMode == CONFIG_SETTING_TEMP_COOLANT) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_TEMPS,
            LocaleGetText(LOCALE_STRING_TEMPS_COOLANT),
            0
        );
    } else if (tempMode == CONFIG_SETTING_TEMP_AMBIENT) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_TEMPS,
            LocaleGetText(LOCALE_STRING_TEMPS_AMBIENT),
            0
        );
    } else if (tempMode == CONFIG_SETTING_TEMP_OIL) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_TEMPS,
            LocaleGetText(LOCALE_STRING_TEMPS_OIL),
            0
        );
    }
    uint8_t dashboardOBC = ConfigGetSetting(CONFIG_SETTING_BMBT_DASHBOARD_OBC);
    if (dashboardOBC == CONFIG_SETTING_ON) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_DASH_OBC,
            LocaleGetText(LOCALE_STRING_DASH_OBC_ON),
            0
        );
    } else if (dashboardOBC == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_DASH_OBC,
            LocaleGetText(LOCALE_STRING_DASH_OBC_OFF),
            0
        );
    }
    if (ConfigGetSetting(CONFIG_SETTING_MONITOR_OFF) == CONFIG_SETTING_ON) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_MONITOR_OFF,
            LocaleGetText(LOCALE_STRING_BMBT_OFF_ON),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_MONITOR_OFF,
            LocaleGetText(LOCALE_STRING_BMBT_OFF_OFF),
            0
        );
    }
    uint8_t selectedLanguage = ConfigGetSetting(CONFIG_SETTING_LANGUAGE);
    char localeName[3] = {0};
    switch (selectedLanguage) {
        case CONFIG_SETTING_LANGUAGE_DUTCH:
            strncpy(localeName, "NL", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_ENGLISH:
            strncpy(localeName, "EN", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_ESTONIAN:
            strncpy(localeName, "ET", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_GERMAN:
            strncpy(localeName, "DE", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_ITALIAN:
            strncpy(localeName, "IT", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_RUSSIAN:
            strncpy(localeName, "RU", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_SPANISH:
            strncpy(localeName, "ES", 2);
            break;
        default:
            strncpy(localeName, "EN", 2);
            break;
    }
    char langStr[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(
        langStr,
        BMBT_MENU_STRING_MAX_SIZE,
        LocaleGetText(LOCALE_STRING_LANG),
        localeName
    );
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_UI_LANGUAGE,
        langStr,
        1
    );
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_UI));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_UI;
}

static void BMBTSettingsUpdateAbout(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
}

static void BMBTSettingsUpdateAudio(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO_DAC_GAIN) {
        uint8_t currentVolume = ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL);
        currentVolume = currentVolume + 2;
        if (currentVolume > 96) {
            currentVolume = 0;
        }
        ConfigSetSetting(CONFIG_SETTING_DAC_AUDIO_VOL, currentVolume);
        char volText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        if (currentVolume > 0x30) {
            uint8_t gain = (currentVolume - 0x30) / 2;
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_NEG_DB), gain);
        } else if (currentVolume == 0) {
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_24_DB));
        } else if (currentVolume == 0x30) {
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_0_DB));
        } else {
            uint8_t gain = (0x30 - currentVolume) / 2;
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_POS_DB), gain);
        }
        BMBTGTWriteIndex(context, selectedIdx, volText, 0);
        PCM51XXSetVolume(currentVolume);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT) {
        uint8_t dspInput = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
        if (dspInput == CONFIG_SETTING_OFF) {
            ConfigSetSetting(CONFIG_SETTING_DSP_INPUT_SRC, CONFIG_SETTING_DSP_INPUT_SPDIF);
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_DSP_DIGITAL), 0);
        } else if (dspInput == CONFIG_SETTING_DSP_INPUT_SPDIF) {
            ConfigSetSetting(CONFIG_SETTING_DSP_INPUT_SRC, CONFIG_SETTING_DSP_INPUT_ANALOG);
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_DSP_ANALOG), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_DSP_INPUT_SRC, CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_DSP_DEFAULT), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO_AUTOPLAY) {
        if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_OFF) {
            ConfigSetSetting(CONFIG_SETTING_AUTOPLAY, CONFIG_SETTING_ON);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_AUTOPLAY_ON), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_AUTOPLAY, CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_AUTOPLAY_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO_MANAGE_VOL) {
        if (ConfigGetSetting(CONFIG_SETTING_MANAGE_VOLUME) == CONFIG_SETTING_OFF) {
            ConfigSetSetting(CONFIG_SETTING_MANAGE_VOLUME, CONFIG_SETTING_ON);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MANAGE_VOL_ON), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_MANAGE_VOLUME, CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MANAGE_VOL_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO_REV_VOL) {
        if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_OFF) {
            ConfigSetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV, CONFIG_SETTING_ON);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_REV_VOL_LOW_ON), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV, CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_REV_VOL_LOW_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

static void BMBTSettingsUpdateComfort(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_BLINKERS) {
        uint8_t value = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
        if (value == 0) {
            value = 1;
        } else if (value == 8) {
            value = 0;
        }
        value = value + 1;
        ConfigSetSetting(CONFIG_SETTING_COMFORT_BLINKERS, value);
        char blinkerText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        snprintf(blinkerText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_BLINKERS), value);
        BMBTGTWriteIndex(context, selectedIdx, blinkerText, 0);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_PARKING_LAMPS) {
        uint8_t value = ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS);
        if (value == CONFIG_SETTING_OFF) {
            value = CONFIG_SETTING_ON;
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_PARK_LAMPS_ON), 0);
        } else {
            value = CONFIG_SETTING_OFF;
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_PARK_LAMPS_OFF), 0);
        }
        // Request cluster indicators so we can trigger the new light setting
        // when the response (0x5B) is received
        IBusCommandLMGetClusterIndicators(context->ibus);
        ConfigSetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS, value);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_LOCK) {
        uint8_t comfortLock = ConfigGetComfortLock();
        if (comfortLock == CONFIG_SETTING_OFF ||
            comfortLock > CONFIG_SETTING_COMFORT_LOCK_20KM
        ) {
            ConfigSetComfortLock(CONFIG_SETTING_COMFORT_LOCK_10KM);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_LOCK_10KMH), 0);
        } else if (comfortLock == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            ConfigSetComfortLock(CONFIG_SETTING_COMFORT_LOCK_20KM);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_LOCK_20KMH), 0);
        } else {
            ConfigSetComfortLock(CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_LOCK_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_UNLOCK) {
        uint8_t comfortUnlock = ConfigGetComfortUnlock();
        if (comfortUnlock == CONFIG_SETTING_OFF ||
            comfortUnlock > CONFIG_SETTING_COMFORT_UNLOCK_POS_0
        ) {
            ConfigSetComfortUnlock(CONFIG_SETTING_COMFORT_UNLOCK_POS_1);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_UNLOCK_POS_1), 0);
        } else if (comfortUnlock == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            ConfigSetComfortUnlock(CONFIG_SETTING_COMFORT_UNLOCK_POS_0);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_UNLOCK_POS_0), 0);
        } else {
            ConfigSetComfortUnlock(CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_UNLOCK_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_AUTOZOOM) {
        uint8_t autozoom = ConfigGetSetting(CONFIG_SETTING_COMFORT_AUTOZOOM);
        autozoom++;
        if (autozoom >= IBUS_SES_ZOOM_LEVELS) {
            autozoom = CONFIG_SETTING_OFF;
        }
        ConfigSetSetting(CONFIG_SETTING_COMFORT_AUTOZOOM, autozoom);
        char autoZoomText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        if (autozoom == CONFIG_SETTING_OFF) {
            snprintf(
                autoZoomText,
                BMBT_MENU_STRING_MAX_SIZE,
                LocaleGetText(LOCALE_STRING_AUTOZOOM),
                "Off"
            );
        } else {
            if (ConfigGetDistUnit() == 0) {
                snprintf(
                    autoZoomText,
                    BMBT_MENU_STRING_MAX_SIZE,
                    LocaleGetText(LOCALE_STRING_AUTOZOOM),
                    navZoomScaleMetric[autozoom]
                );
            } else {
                snprintf(
                    autoZoomText,
                    BMBT_MENU_STRING_MAX_SIZE,
                    LocaleGetText(LOCALE_STRING_AUTOZOOM),
                    navZoomScaleImperial[autozoom]
                );
            }
        }
        BMBTGTWriteIndex(context, selectedIdx, autoZoomText, 0);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME) {
        BMBTMenuSettingsComfortTime(context);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_PDC) {
        uint8_t pdc = ConfigGetSetting(CONFIG_SETTING_COMFORT_PDC);
        if (pdc == CONFIG_SETTING_OFF) {
            pdc=CONFIG_SETTING_PDC_CLUSTER;
        } else if (pdc == CONFIG_SETTING_PDC_CLUSTER) {
//            pdc=CONFIG_SETTING_PDC_RADIO;
            pdc=CONFIG_SETTING_OFF;
        } else if (pdc == CONFIG_SETTING_PDC_RADIO) {
            pdc=CONFIG_SETTING_PDC_BOTH;
        } else {
            pdc=CONFIG_SETTING_OFF;
        }

        ConfigSetSetting(CONFIG_SETTING_COMFORT_PDC, pdc);

        char pdc_text[BMBT_MENU_STRING_MAX_SIZE] = {0};
        snprintf(
            pdc_text,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_PDC),
            (
                (pdc == CONFIG_SETTING_OFF) ? "Off":
                (pdc == CONFIG_SETTING_PDC_CLUSTER)? "Cluster":
                (pdc == CONFIG_SETTING_PDC_RADIO)? "Screen":
                "Dual"
            )
        );

        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_PDC,
            pdc_text,
            0
        );
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

static void BMBTSettingsUpdateComfortTime(BMBTContext_t *context, uint8_t selectedIdx)
{
    uint8_t time_source = ConfigGetTimeSource();
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_SOURCE) {
        switch (time_source) {
            case CONFIG_SETTING_OFF:
                if (context->bt->type == BT_BTM_TYPE_BC127) {
                    time_source = CONFIG_SETTING_TIME_PHONE;
                    BC127CommandAT(context->bt, "+CCLK?");
                } else {
                    time_source = CONFIG_SETTING_TIME_GPS;
                };
                break;
            case CONFIG_SETTING_TIME_PHONE:
                time_source = CONFIG_SETTING_TIME_GPS;
                break;
            case CONFIG_SETTING_TIME_GPS:
            default:
                time_source = CONFIG_SETTING_OFF;
        };
        ConfigSetTimeSource(time_source);
        if (time_source == CONFIG_SETTING_TIME_GPS) {
// calculate the default offset if not previously set and reasonable GPS & IKE times are available
            uint8_t time_dst = ConfigGetTimeDST();
            int16_t time_offset = ConfigGetTimeOffsetIndex();

            if ((time_dst == 0) && (time_offset == 0) && (context->ibus->gpsTime != 0) && (context->ibus->localTime != 0)) {
                LogDebug(LOG_SOURCE_SYSTEM, "Calc DST & Zone offsets, GPS: %s", ctime(&context->ibus->gpsTime));
                LogDebug(LOG_SOURCE_SYSTEM, "Calc DST & Zone offsets, LOCAL: %s", ctime(&context->ibus->localTime));

                struct tm *local_time = gmtime(&context->ibus->localTime);

                if ((local_time->tm_hour!=0)||(local_time->tm_min!=0)) {

                    uint8_t hour = local_time->tm_hour;
                    uint8_t min = local_time->tm_min;

                    local_time = gmtime(&context->ibus->gpsTime);

                    local_time->tm_hour = hour;
                    local_time->tm_min = min;

                    if ((local_time->tm_mon>=3)&&(local_time->tm_mon<=9)) {
                        // assume Apr to Oct is summer time for sake of preset of DST
                        time_dst = CONFIG_SETTING_TIME_DST;
                        local_time->tm_hour -= 1;
                    }

                    time_t localTime = mktime(local_time);

                    time_offset = (difftime(localTime,context->ibus->gpsTime) / 60);

                    if (time_offset > 12*60) {
                        time_offset -= 24*60;
                    } else if (time_offset < -12*60) {
                        time_offset += 24*60;
                    }
                    LogDebug(LOG_SOURCE_SYSTEM, "Calc DST & Zone offsets, DST=%d, OFFSET=%+d min", (time_dst==CONFIG_SETTING_TIME_DST), time_offset);

                    ConfigSetTimeOffset(time_offset);
                    ConfigGetTimeDST(time_dst);
                }
            }
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_DST) {
        uint8_t time_dst = ConfigGetTimeDST();
        if (time_dst == CONFIG_SETTING_OFF) {
            time_dst = CONFIG_SETTING_TIME_DST;
        } else {
            time_dst = CONFIG_SETTING_OFF;
        }
        ConfigSetTimeDST(time_dst);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_OFFSET) {
        uint8_t offset = ConfigGetTimeOffsetIndex();
        if (offset<31) {
            offset++;
        } else {
            offset = 1;
        }
        ConfigSetTimeOffsetIndex(offset);
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettingsComfort(context);
    };

    if (((selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_OFFSET) ||
        (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_DST) ||
        (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT_TIME_SOURCE)) &&
        (time_source == CONFIG_SETTING_TIME_GPS) &&
        (context->ibus->gpsTime != 0)) {

        uint8_t dt[6]={0};
        struct tm *gps_time = gmtime(&context->ibus->gpsTime);

        gps_time->tm_min += ConfigGetTimeOffset() + ((ConfigGetTimeDST()!=0)?60:0);
        mktime(gps_time);

        dt[DATETIME_YEAR] = gps_time->tm_year + 1900 - 2000;
        dt[DATETIME_MON] = gps_time->tm_mon + 1;
        dt[DATETIME_DAY] = gps_time->tm_mday;
        dt[DATETIME_HOUR] = gps_time->tm_hour;
        dt[DATETIME_MIN] = gps_time->tm_min;
        dt[DATETIME_SEC] = gps_time->tm_sec;

        if (dt[DATETIME_YEAR] > 20 &&
            dt[DATETIME_MON] >= 1 && dt[DATETIME_MON] <= 12 &&
            dt[DATETIME_DAY] >= 1 && dt[DATETIME_DAY] <= 31 &&
            dt[DATETIME_HOUR] >= 0 && dt[DATETIME_HOUR] <= 23 &&
            dt[DATETIME_MIN] >= 0 && dt[DATETIME_MIN] <= 59 &&
            dt[DATETIME_SEC] >= 0 && dt[DATETIME_SEC] <= 59
        ) {
            IBusCommandIKESetDate(context->ibus, dt[DATETIME_YEAR], dt[DATETIME_MON], dt[DATETIME_DAY]);
            IBusCommandIKESetTime(context->ibus, dt[DATETIME_HOUR], dt[DATETIME_MIN]);
        }
    }

    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        BMBTMenuSettingsComfortTime(context);
    }
}

static void BMBTSettingsUpdateCalling(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_HFP) {
        uint8_t value = ConfigGetSetting(CONFIG_SETTING_HFP);
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (value == 0x00) {
                ConfigSetSetting(CONFIG_SETTING_HFP, 0x01);
                BC127CommandSetProfiles(context->bt, 1, 1, 1, 1);
                BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_ON), 0);
                BC127CommandProfileOpen(context->bt, "HFP");
            } else {
                ConfigSetSetting(CONFIG_SETTING_HFP, 0x00);
                BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_OFF), 0);
                BC127CommandClose(context->bt, context->bt->activeDevice.hfpId);
            }
        } else {
            if (value == 0x01) {
                BM83CommandDisconnect(context->bt, BM83_CMD_DISCONNECT_PARAM_HF);
                ConfigSetSetting(CONFIG_SETTING_HFP, 0x00);
                BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_OFF), 0);
            } else {
                BTPairedDevice_t *device = 0;
                uint8_t i = 0;
                for (i = 0; i < BT_MAC_ID_LEN; i++) {
                    BTPairedDevice_t *tmpDev = &context->bt->pairedDevices[i];
                    if (memcmp(context->bt->activeDevice.macId, tmpDev->macId, BT_MAC_ID_LEN) == 0) {
                        device = tmpDev;
                    }
                }
                if (device != 0) {
                    BM83CommandConnect(
                        context->bt,
                        device,
                        BM83_DATA_LINK_BACK_PROFILES_HF
                    );
                }
                ConfigSetSetting(CONFIG_SETTING_HFP, 0x01);
                BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_ON), 0);
            }
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN) {
        uint8_t micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
        micGain = micGain + 1;
        char micGainText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (micGain > 21) {
                micGain = 0;
            }

            uint8_t micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
            uint8_t micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
            BC127CommandSetMicGain(
                context->bt,
                micGain,
                micBias,
                micPreamp
            );
            snprintf(micGainText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_MIC_GAIN), (int8_t) BTBC127MicGainTable[micGain]);
        } else {
            if (micGain > 0x0F) {
                micGain = 0;
            }
            if (micGain == 0x00) {
                // Reset the gain
                uint8_t start = 0x0F;
                while (start > 0) {
                    BM83CommandMicGainDown(context->bt);
                    start--;
                }
            } else {
                BM83CommandMicGainUp(context->bt);
            }
            snprintf(micGainText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_MIC_GAIN), (int8_t) BTBM83MicGainTable[micGain]);
        }
        ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, micGain);
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN,
            micGainText,
            0
        );
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_VOL_OFFSET) {
        int8_t volumeOffset = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        volumeOffset = volumeOffset + 1;
        char volOffsetText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        if (volumeOffset > CONFIG_SETTING_TEL_VOL_OFFSET_MAX) {
            volumeOffset = 0;
        }
        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, volumeOffset);
        snprintf(
            volOffsetText,
            BMBT_MENU_STRING_MAX_SIZE,
            LocaleGetText(LOCALE_STRING_VOL_OFFSET),
            volumeOffset
        );
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_VOL_OFFSET,
            volOffsetText,
            0
        );
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_MODE) {
        uint8_t telMode = ConfigGetSetting(CONFIG_SETTING_TEL_MODE);
        if (telMode == CONFIG_SETTING_TEL_MODE_DEFAULT) {
            ConfigSetSetting(CONFIG_SETTING_TEL_MODE, CONFIG_SETTING_TEL_MODE_TCU);
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_TCU),
                0
            );
        } else if (telMode == CONFIG_SETTING_TEL_MODE_TCU) {
            ConfigSetSetting(CONFIG_SETTING_TEL_MODE, CONFIG_SETTING_TEL_MODE_NO_MUTE);
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_NO_MUTE),
                0
            );
        } else {
            ConfigSetSetting(CONFIG_SETTING_TEL_MODE, CONFIG_SETTING_TEL_MODE_DEFAULT);
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_DEFAULT),
                0
            );
        }
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

static void BMBTSettingsUpdateUI(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_METADATA_MODE) {
        uint8_t value = ConfigGetSetting(CONFIG_SETTING_METADATA_MODE);
        if (value == 0x00) {
            value = BMBT_METADATA_MODE_PARTY;
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_METADATA_PARTY), 0);
        } else if (value == 0x01) {
            value = BMBT_METADATA_MODE_CHUNK;
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_METADATA_CHUNK), 0);
        } else {
            value = BMBT_METADATA_MODE_OFF;
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_METADATA_OFF), 0);
        }
        ConfigSetSetting(CONFIG_SETTING_METADATA_MODE, value);
        if (value != BMBT_METADATA_MODE_OFF &&
            strlen(context->bt->title) > 0 &&
            context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING
        ) {
            char text[UTILS_DISPLAY_TEXT_SIZE] = {0};
            snprintf(
                text,
                UTILS_DISPLAY_TEXT_SIZE,
                "%s - %s - %s",
                context->bt->title,
                context->bt->artist,
                context->bt->album
            );
            BMBTSetMainDisplayText(context, text, 0, 0);
        } else if (value == BMBT_METADATA_MODE_OFF) {
            IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
            BMBTGTWriteTitle(context, LocaleGetText(LOCALE_STRING_BLUETOOTH));
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU) {
        if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x00) {
            ConfigSetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU, 0x01);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MENU_DASHBOARD), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU, 0x00);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MENU_MAIN), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_TEMPS) {
        uint8_t tempMode = ConfigGetTempDisplay();
        if (tempMode == CONFIG_SETTING_OFF) {
            ConfigSetTempDisplay(CONFIG_SETTING_TEMP_COOLANT);
            uint8_t valueType = IBUS_SENSOR_VALUE_COOLANT_TEMP;
            BMBTIBusSensorValueUpdate((void *)context, &valueType);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_COOLANT), 0);
        } else if (tempMode == CONFIG_SETTING_TEMP_COOLANT) {
            ConfigSetTempDisplay(CONFIG_SETTING_TEMP_AMBIENT);
            uint8_t valueType = IBUS_SENSOR_VALUE_AMBIENT_TEMP;
            BMBTIBusSensorValueUpdate((void *)context, &valueType);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_AMBIENT), 0);
        } else if (
                tempMode == CONFIG_SETTING_TEMP_AMBIENT &&
                context->ibus->vehicleType != IBUS_VEHICLE_TYPE_E46_Z4
        ) {
            ConfigSetTempDisplay(CONFIG_SETTING_TEMP_OIL);
            uint8_t valueType = IBUS_SENSOR_VALUE_OIL_TEMP;
            BMBTIBusSensorValueUpdate((void *)context, &valueType);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_OIL), 0);
        } else {
            // Clear the header area
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_TEMPS, "      ");
            IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
            ConfigSetTempDisplay(CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_DASH_OBC) {
        if (ConfigGetSetting(CONFIG_SETTING_BMBT_DASHBOARD_OBC_ADDRESS) == CONFIG_SETTING_ON) {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_UI_DASH_OBC,
                LocaleGetText(LOCALE_STRING_DASH_OBC_OFF),
                0
            );
            ConfigSetSetting(
                CONFIG_SETTING_BMBT_DASHBOARD_OBC_ADDRESS,
                CONFIG_SETTING_OFF
            );
        } else {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_UI_DASH_OBC,
                LocaleGetText(LOCALE_STRING_DASH_OBC_ON),
                0
            );
            ConfigSetSetting(
                CONFIG_SETTING_BMBT_DASHBOARD_OBC_ADDRESS,
                CONFIG_SETTING_ON
            );
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_MONITOR_OFF) {
        if (ConfigGetSetting(CONFIG_SETTING_MONITOR_OFF) == CONFIG_SETTING_ON) {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_UI_MONITOR_OFF,
                LocaleGetText(LOCALE_STRING_BMBT_OFF_OFF),
                0
            );
            ConfigSetSetting(CONFIG_SETTING_MONITOR_OFF, CONFIG_SETTING_OFF);
        } else {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_UI_MONITOR_OFF,
                LocaleGetText(LOCALE_STRING_BMBT_OFF_ON),
                0
            );
            ConfigSetSetting(CONFIG_SETTING_MONITOR_OFF, CONFIG_SETTING_ON);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_LANGUAGE) {
        uint8_t selectedLanguage = ConfigGetSetting(CONFIG_SETTING_LANGUAGE);
        if (selectedLanguage == CONFIG_SETTING_LANGUAGE_DUTCH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_ENGLISH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_ENGLISH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_ESTONIAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_ESTONIAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_GERMAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_GERMAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_RUSSIAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_RUSSIAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_SPANISH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_SPANISH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_DUTCH;
        } else {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_ENGLISH;
        }
        ConfigSetSetting(CONFIG_SETTING_LANGUAGE, selectedLanguage);
        // Change the UI Language
        BMBTMenuSettingsUI(context);
        BMBTTriggerWriteHeader(context);
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

/**
 * BMBTBTDeviceConnected()
 *     Description:
 *         Handle screen updates when a device connects
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBTDeviceConnected(void *ctx, uint8_t *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        context->status.displayMode == BMBT_DISPLAY_ON
    ) {
        BMBTHeaderWriteDeviceName(context, context->bt->activeDevice.deviceName);
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
        if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            BMBTMenuDeviceSelection(context);
        }
    }
}

/**
 * BMBTBTDeviceDisconnected()
 *     Description:
 *         Handle screen updates when a device disconnects
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBTDeviceDisconnected(void *ctx, uint8_t *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        context->status.displayMode == BMBT_DISPLAY_ON
    ) {
        BMBTHeaderWriteDeviceName(context, LocaleGetText(LOCALE_STRING_NO_DEVICE));
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
        if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            BMBTMenuDeviceSelection(context);
        }
    }
}

/**
 * BMBTBTMetadata()
 *     Description:
 *         Handle metadata updates from the BT module
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBTMetadata(void *ctx, uint8_t *data)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        context->status.displayMode == BMBT_DISPLAY_ON
    ) {
        if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) != CONFIG_SETTING_OFF) {
            char text[UTILS_DISPLAY_TEXT_SIZE] = {0};
            snprintf(
                text,
                UTILS_DISPLAY_TEXT_SIZE,
                "%s - %s - %s",
                context->bt->title,
                context->bt->artist,
                context->bt->album
            );
            BMBTSetMainDisplayText(context, text, 0, 1);
        }
        if (context->menu == BMBT_MENU_DASHBOARD ||
            context->menu == BMBT_MENU_DASHBOARD_FRESH
        ) {
            BMBTMenuDashboard(context);
        }
    }
}

/**
 * BMBTBTPlaybackStatus()
 *     Description:
 *         Handle the BT Module playback state changes
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBTPlaybackStatus(void *ctx, uint8_t *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.displayMode == BMBT_DISPLAY_ON) {
        if (context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED) {
            BMBTSetMainDisplayText(context, LocaleGetText(LOCALE_STRING_BLUETOOTH), 0, 1);
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
        } else {
            BMBTMainAreaRefresh(context);
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "> ");
        }
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    }
}

/**
 * BMBTBTReady()
 *     Description:
 *         Handle the BT module rebooting gracefully
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBTReady(void *ctx, uint8_t *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    BMBTHeaderWriteDeviceName(context, LocaleGetText(LOCALE_STRING_NO_DEVICE));
    if (context->status.displayMode == BMBT_DISPLAY_ON) {
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    }
}

/**
 * BMBTIBusBMBTButtonPress()
 *     Description:
 *         Handle button presses on the BoardMonitor
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusBMBTButtonPress(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE) {
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_PlayPause ||
            pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_Num1
        ) {
            if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                BTCommandPause(context->bt);
            } else {
                BTCommandPlay(context->bt);
            }
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_Knob) {
            if (context->status.displayMode == BMBT_DISPLAY_ON &&
                context->menu == BMBT_MENU_DASHBOARD &&
                context->ibus->gtVersion == IBUS_GT_MKIV_STATIC
            ) {
                BMBTMenuMain(context);
            }
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_Display) {
            if (context->status.playerMode == BMBT_MODE_ACTIVE) {
                if (context->status.displayMode == BMBT_DISPLAY_OFF) {
                    context->status.displayMode = BMBT_DISPLAY_ON;
                    if (context->menu != BMBT_MENU_DASHBOARD_FRESH) {
                        context->menu = BMBT_MENU_NONE;
                    }
                    IBusCommandRADDisableMenu(context->ibus);
                } else {
                    context->status.displayMode = BMBT_DISPLAY_OFF;
                }
            }
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_Mode) {
            context->status.playerMode = BMBT_MODE_INACTIVE;
        }
        // Handle the SEL and Info buttons gracefully
        if (pkt[3] == IBUS_CMD_BMBT_BUTTON0 && pkt[1] == 0x05) {
            if (pkt[5] == IBUS_DEVICE_BMBT_Button_Info) {
                context->status.displayMode = BMBT_DISPLAY_INFO;
            } else if (pkt[5] == IBUS_DEVICE_BMBT_Button_SEL) {
                context->status.displayMode = BMBT_DISPLAY_TONE_SEL;
            }
        }
    }
    // Handle calls at any time
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_TEL_Release) {
            if (context->bt->callStatus == BT_CALL_ACTIVE) {
                BTCommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BT_CALL_INCOMING) {
                BTCommandCallAccept(context->bt);
            } else if (context->bt->callStatus == BT_CALL_OUTGOING) {
                BTCommandCallEnd(context->bt);
            }
        } else if (context->bt->callStatus == BT_CALL_INACTIVE &&
                   pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_TEL_Hold
        ) {
            BTCommandToggleVoiceRecognition(context->bt);
        }
    }
}

/**
 * BMBTIBusCDChangerStatus()
 *     Description:
 *         Track the CD Changer state so we know if we can write to the
 *         screen or not, as well as handle playback state
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusCDChangerStatus(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t requestedCommand = pkt[IBUS_PKT_DB1];
    if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        // Stop Playing
        if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
            BTCommandPause(context->bt);
        }
        if (context->status.tvStatus == BMBT_TV_STATUS_OFF) {
            IBusCommandRADEnableMenu(context->ibus);
        }
        context->menu = BMBT_MENU_NONE;
        context->status.playerMode = BMBT_MODE_INACTIVE;
        context->status.displayMode = BMBT_DISPLAY_OFF;
        BMBTSetMainDisplayText(context, LocaleGetText(LOCALE_STRING_BLUETOOTH), 0, 0);
    } else if (requestedCommand == IBUS_CDC_CMD_START_PLAYING ||
        (context->ibus->cdChangerFunction == IBUS_CDC_FUNC_PLAYING &&
         context->status.playerMode == BMBT_MODE_INACTIVE)
    ) {
        BMBTSetMainDisplayText(context, LocaleGetText(LOCALE_STRING_BLUETOOTH), 0, 0);
        if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == CONFIG_SETTING_ON) {
            BTCommandPlay(context->bt);
        } else if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING &&
                   context->status.playerMode == BMBT_MODE_INACTIVE
        ) {
            BTCommandPause(context->bt);
        }
        IBusCommandRADDisableMenu(context->ibus);
        context->status.playerMode = BMBT_MODE_ACTIVE;
        BMBTTriggerWriteHeader(context);
        BMBTTriggerWriteMenu(context);
    } else if (requestedCommand == IBUS_CDC_CMD_RANDOM_MODE &&
               context->status.displayMode == BMBT_DISPLAY_OFF
    ) {
        // This adds support for GTs that run without a radio overlay
        context->status.displayMode = BMBT_DISPLAY_ON;
        if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF ||
            context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED
        ) {
            BMBTGTWriteTitle(context, LocaleGetText(LOCALE_STRING_BLUETOOTH));
        } else {
            BMBTMainAreaRefresh(context);
        }
        BMBTTriggerWriteHeader(context);
        BMBTTriggerWriteMenu(context);
    }
}

/**
 * BMBTIBusGTChangeUIRequest()
 *     Description:
 *         Display the Telephone UI when the GT requests it
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusGTChangeUIRequest(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[IBUS_PKT_DB1] == 0x02 && pkt[5] == 0x0C) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            IBusCommandTELSetGTDisplayMenu(context->ibus);
            IBusCommandTELSetGTDisplayNumber(context->ibus, context->bt->dialBuffer);
        }
    }
}

/**
 * BMBTIKESpeedRPMUpdate()
 *     Description:
 *         Handle Map Auto Zoom as the speed changes
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIKESpeedRPMUpdate(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;

    uint8_t autozoom = ConfigGetSetting(CONFIG_SETTING_COMFORT_AUTOZOOM);
    if (autozoom == CONFIG_SETTING_OFF) {
        return;
    }
    uint16_t speed = pkt[IBUS_PKT_DB1] * 2;
    uint8_t zoomLevel;

    if (autozoom == 1) {
        zoomLevel = 0;
    } else if (autozoom >= IBUS_SES_ZOOM_LEVELS - 1) {
        zoomLevel = IBUS_SES_ZOOM_LEVELS - 1;
    } else {
        zoomLevel = autozoom;
    }

    while (zoomLevel < IBUS_SES_ZOOM_LEVELS - 1 &&
           speed > navZoomSpeeds[zoomLevel]
    ) {
        zoomLevel++;
    }

    if (context->navZoom > zoomLevel &&
        (speed + BMBT_AUTOZOOM_TOLERANCE) > navZoomSpeeds[context->navZoom - 1]
    ) {
        zoomLevel = context->navZoom;
    } else if (context->navZoom != zoomLevel) {
        uint32_t now = TimerGetMillis();
        LogDebug(
            LOG_SOURCE_UI,
            "Autozoom: kmh=%i, currentZoom=%i, wishedZoom=%i, timeDiff=%lu",
            speed,
            context->navZoom,
            zoomLevel,
            now - context->navZoomTime
        );
        if (context->navZoomTime + BMBT_AUTOZOOM_DELAY < now) {
            context->navZoom = zoomLevel;
            context->navZoomTime = now;
            IBusCommandSESSetMapZoom(context->ibus, zoomLevel);
        }
    }
}

void BMBTIBusMenuSelect(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t selectedIdx = (uint8_t) pkt[6];
    if (context->status.radType == IBUS_RADIO_TYPE_BM24) {
        if (selectedIdx < 10) {
            selectedIdx = 0xFF;
        } else {
            selectedIdx = selectedIdx - 0x40;
        }
    }
    if (selectedIdx < 10 && context->status.displayMode == BMBT_DISPLAY_ON) {
        if (context->menu == BMBT_MENU_MAIN) {
            if (selectedIdx == BMBT_MENU_IDX_DASHBOARD) {
                BMBTMenuDashboard(context);
            } else if (selectedIdx == BMBT_MENU_IDX_DEVICE_SELECTION) {
                BMBTMenuDeviceSelection(context);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS) {
                BMBTMenuSettings(context);
            }
        } else if (context->menu == BMBT_MENU_DASHBOARD) {
            BMBTMenuMain(context);
        } else if (context->menu == BMBT_MENU_DEVICE_SELECTION) {
            if (selectedIdx == BMBT_MENU_IDX_PAIRING_MODE) {
                uint8_t state;
                if (context->bt->discoverable == BT_STATE_ON) {
                    BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_OFF), 0);
                    state = BT_STATE_OFF;
                } else {
                    BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_ON), 0);
                    state = BT_STATE_ON;
                    if (context->bt->activeDevice.deviceId != 0) {
                        // To pair a new device, we must disconnect the active one
                        EventTriggerCallback(UIEvent_CloseConnection, 0x00);
                    }
                }
                IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
                BTCommandSetDiscoverable(context->bt, state);
            } else if (selectedIdx == BMBT_MENU_IDX_CLEAR_PAIRING) {
                if (context->bt->type == BT_BTM_TYPE_BC127) {
                    BC127CommandUnpair(context->bt);
                } else {
                    BM83CommandRestore(context->bt);
                    ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x00);
                    ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE, 0x00);
                }
                BTClearPairedDevices(context->bt, BT_TYPE_CLEAR_ALL);
                ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE_MAC,0x00);
                BMBTMenuDeviceSelection(context);
            } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
                // Back Button
                BMBTMenuMain(context);
            } else {
                uint8_t selectedDeviceId = selectedIdx - BMBT_MENU_IDX_FIRST_DEVICE;
                uint8_t deviceId = 0;
                uint8_t devicesPos = 0;
                uint8_t idx;
                BTPairedDevice_t *dev = 0;

                for (idx = 0; idx < context->bt->pairedDevicesCount; idx++) {
                    dev = &context->bt->pairedDevices[idx];
                    if (dev != 0) {
                        if ( devicesPos == selectedDeviceId ) {
                            deviceId = idx;
                            break;
                        }
                        devicesPos++;
                    }
                }

                if ((dev != 0) &&
                    (memcmp(dev->macId, context->bt->activeDevice.macId, BT_LEN_MAC_ID) != 0 )
                ) {
                    // Trigger device selection event
                    EventTriggerCallback(
                        UIEvent_InitiateConnection,
                        (uint8_t *)&deviceId
                    );
                }
            }
        } else if (context->menu == BMBT_MENU_SETTINGS) {
            if (selectedIdx == BMBT_MENU_IDX_SETTINGS_ABOUT) {
                BMBTMenuSettingsAbout(context);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO) {
                BMBTMenuSettingsAudio(context);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_COMFORT) {
                BMBTMenuSettingsComfort(context);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING) {
                BMBTMenuSettingsCalling(context);
            } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI) {
                BMBTMenuSettingsUI(context);
            } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
                BMBTMenuMain(context);
            }
        } else if (context->menu == BMBT_MENU_SETTINGS_ABOUT) {
            BMBTSettingsUpdateAbout(context, selectedIdx);
        } else if (context->menu == BMBT_MENU_SETTINGS_AUDIO) {
            BMBTSettingsUpdateAudio(context, selectedIdx);
        } else if (context->menu == BMBT_MENU_SETTINGS_COMFORT) {
            BMBTSettingsUpdateComfort(context, selectedIdx);
        } else if (context->menu == BMBT_MENU_SETTINGS_COMFORT_TIME) {
            BMBTSettingsUpdateComfortTime(context, selectedIdx);
        } else if (context->menu == BMBT_MENU_SETTINGS_CALLING) {
            BMBTSettingsUpdateCalling(context, selectedIdx);
        } else if (context->menu == BMBT_MENU_SETTINGS_UI) {
            BMBTSettingsUpdateUI(context, selectedIdx);
        }
    }
}

/**
 * BMBTIBusScreenBufferFlush()
 *     Description:
 *         Respond to screen flushes that may not have been made by us
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - The I/K-Bus packet
 *     Returns:
 *         void
 */
void BMBTIBusScreenBufferFlush(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    // Ignore Zone (Header) updates
    if (pkt[IBUS_PKT_DB1] != IBUS_CMD_GT_WRITE_ZONE) {
        if (pkt[IBUS_PKT_DB1] != context->status.navIndexType) {
            IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
        }
    }
}


/**
 * BMBTIBusSensorValueUpdate()
 *     Description:
 *         Respond to sensor value updates emitted on the I/K-Bus
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *type - The update type
 *     Returns:
 *         void
 */
void BMBTIBusSensorValueUpdate(void *ctx, uint8_t *type)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    uint8_t updateType = *type;
    int temp = 0;
    char tempUnit = 'C';
    char redraw = 0;
    char temperature[8] = {0};
    char config = ConfigGetTempDisplay();

    if (context->ibus->ambientTemperatureCalculated[0] == 0) {
       IBusCommandIKEOBCControl(
            context->ibus,
            IBUS_IKE_OBC_PROPERTY_TEMPERATURE,
            IBUS_IKE_OBC_PROPERTY_REQUEST_TEXT
        );
    }

    if (context->status.displayMode == BMBT_DISPLAY_ON) {

        if (ConfigGetTempUnit() == CONFIG_SETTING_TEMP_FAHRENHEIT) {
            tempUnit = 'F';
        }

        if (config == CONFIG_SETTING_TEMP_AMBIENT &&
            (updateType == IBUS_SENSOR_VALUE_AMBIENT_TEMP_CALCULATED ||
             updateType == IBUS_SENSOR_VALUE_AMBIENT_TEMP ||
             updateType == IBUS_SENSOR_VALUE_TEMP_UNIT) &&
            context->ibus->ambientTemperatureCalculated[0] != 0
        ) {
            snprintf(temperature, 8, "%s\xB0%c", context->ibus->ambientTemperatureCalculated, tempUnit);
            redraw = 1;
        } else {
            if (config == CONFIG_SETTING_TEMP_COOLANT &&
                (updateType == IBUS_SENSOR_VALUE_COOLANT_TEMP ||
                 updateType == IBUS_SENSOR_VALUE_TEMP_UNIT)
            ) {
                temp = context->ibus->coolantTemperature;
                if (temp != 0) {
                    redraw = 1;
                }
            } else if (config == CONFIG_SETTING_TEMP_AMBIENT &&
                (updateType == IBUS_SENSOR_VALUE_AMBIENT_TEMP ||
                 updateType == IBUS_SENSOR_VALUE_AMBIENT_TEMP_CALCULATED ||
                 updateType == IBUS_SENSOR_VALUE_TEMP_UNIT)
            ) {
                temp = context->ibus->ambientTemperature;
                redraw = 1;
            } else if (config == CONFIG_SETTING_TEMP_OIL &&
                (updateType == IBUS_SENSOR_VALUE_OIL_TEMP ||
                 updateType == IBUS_SENSOR_VALUE_TEMP_UNIT)
            ) {
                temp = context->ibus->oilTemperature;
                if (temp != 0) {
                    redraw = 1;
                }
            }
            if (redraw == 1) {
                if (tempUnit == 'F') {
                    temp = temp * 1.8 + 32 + 0.5;
                }
                if (config == CONFIG_SETTING_TEMP_AMBIENT) {
                    if (tempUnit == 'F') {
                        snprintf(temperature, 7, "%+d\xB0%c", temp, tempUnit);
                    } else {
                        snprintf(temperature, 8, "%+d.0\xB0%c", temp, tempUnit);
                    }
                } else {
                    snprintf(temperature, 6, "%d\xB0%c", temp, tempUnit);
                }
            }
        }

        if (redraw == 1) {
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_TEMPS, temperature);
            IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
        }

        if (context->menu == BMBT_MENU_DASHBOARD ||
            context->menu == BMBT_MENU_DASHBOARD_FRESH
        ) {
            BMBTMenuDashboardUpdateOBCValues(context);
            if (context->ibus->gtVersion == IBUS_GT_MKIV_STATIC) {
                IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_STATIC);
            } else {
                IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
            }
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
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTRADDisplayMenu(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    context->status.displayMode = BMBT_DISPLAY_TONE_SEL;
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
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTRADUpdateMainArea(void *ctx, uint8_t *pkt)
{
    // Main area updates to the IKE should not trigger BMBT refreshes
    // This message is only intended to support the CD54 more consistently
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_IKE) {
        return;
    }
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[IBUS_PKT_DB1] == IBUS_C43_TITLE_MODE) {
        context->status.radType = IBUS_RADIO_TYPE_C43;
    }
    uint8_t textIsOurs = 0;
    if (pkt[5] == 0x30) {
        // The BM24 uses this layout to write out multiple headers
        // at the same time. Ensure the text is ours
        if (pkt[6] != 0x20 && pkt[7] != 0x20 && pkt[8] != 0x07) {
            textIsOurs = 1;
        } else {
            context->status.radType = IBUS_RADIO_TYPE_BM24;
        }
    }
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        textIsOurs == 0
    ) {
        uint8_t pktLen = (uint8_t) pkt[1] + 2;
        uint8_t textLen = pktLen - 7;
        char text[textLen];
        memset(&text, 0, textLen);
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
        if (UtilsStricmp("NO TAPE", text) == 0 ||
            UtilsStricmp("NO CD", text) == 0
        ) {
            context->status.displayMode = BMBT_DISPLAY_OFF;
        } else {
            // Clear the radio display if we have a C43 in a "new UI" nav
            if (pkt[IBUS_PKT_DB1] == IBUS_C43_TITLE_MODE &&
                context->ibus->gtVersion >= IBUS_GT_MKIII_NEW_UI
            ) {
                IBusCommandRADClearMenu(context->ibus);
            }
            if (context->status.displayMode == BMBT_DISPLAY_OFF) {
                context->status.displayMode = BMBT_DISPLAY_ON;
            } else if (UtilsStricmp("NO DISC", text) == 0) {
                BMBTTriggerWriteMenu(context);
            }
            if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF ||
                context->bt->playbackStatus == BT_AVRCP_STATUS_PAUSED
            ) {
                BMBTGTWriteTitle(context, LocaleGetText(LOCALE_STRING_BLUETOOTH));
            } else {
                BMBTMainAreaRefresh(context);
            }
            BMBTTriggerWriteHeader(context);
            BMBTTriggerWriteMenu(context);
        }
    }
}

/**
 * BMBTRADScreenModeRequest()
 *     Description:
 *         This callback tracks the screen mode that is stipulated by the RAD.
 *     Params:
 *         void *ctx - The context
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTRADScreenModeRequest(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE) {
        if (pkt[IBUS_PKT_DB1] == 0x01 || pkt[IBUS_PKT_DB1] == IBUS_RAD_PRIORITY_GT) {
            if (context->menu == BMBT_MENU_DASHBOARD) {
                context->menu = BMBT_MENU_DASHBOARD_FRESH;
            } else {
                context->menu = BMBT_MENU_NONE;
            }
            context->status.displayMode = BMBT_DISPLAY_OFF;
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_RAD_HIDE_BODY &&
            context->status.navState == BMBT_NAV_STATE_BOOT
        ) {
            IBusCommandRADDisableMenu(context->ibus);
            context->status.navState = BMBT_NAV_STATE_ON;
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_RAD_HIDE_BODY &&
            (context->status.displayMode == BMBT_DISPLAY_ON ||
             context->status.displayMode == BMBT_DISPLAY_INFO)
        ) {
            BMBTTriggerWriteMenu(context);
        } else if (pkt[IBUS_PKT_DB1] == IBUS_GT_TONE_MENU_OFF ||
             pkt[IBUS_PKT_DB1] == IBUS_GT_SEL_MENU_OFF
        ) {
            context->status.displayMode = BMBT_DISPLAY_ON;
        }
    }
}

/**
 * BMBTGTScreenModeSet()
 *     Description:
 *         Track the state that the GT expects from the radio
 *     Params:
 *         void *ctx - The context
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTGTScreenModeSet(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[IBUS_PKT_DB1] == BMBT_NAV_BOOT) {
        context->menu = BMBT_MENU_NONE;
        if (context->status.playerMode == BMBT_MODE_ACTIVE) {
            context->status.navState = BMBT_NAV_STATE_BOOT;
        }
    }
}


/**
 * BMBTTVStatusUpdate()
 *     Description:
 *         Listen for the GT -> RAD Television status message
 *     Params:
 *         void *ctx - The context
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTTVStatusUpdate(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    context->status.tvStatus = pkt[IBUS_PKT_DB1];
}

/**
 * BMBTIBusVehicleConfig()
 *     Description:
 *        Update the temperature unit configuration if it changes
 *     Params:
 *         void *ctx - The context
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTIBusVehicleConfig(void *ctx, uint8_t *pkt)
{
    // Update the temperature unit
    uint8_t tempUnit = IBusGetConfigTemp(pkt);
    if (tempUnit != ConfigGetTempUnit()) {
        ConfigSetTempUnit(tempUnit);
        uint8_t valueType = IBUS_SENSOR_VALUE_TEMP_UNIT;
        BMBTIBusSensorValueUpdate(ctx, &valueType);
    }

    tempUnit = IBusGetConfigDistance(pkt);
    if (tempUnit != ConfigGetDistUnit()) {
        ConfigSetDistUnit(tempUnit);
    }
}

/**
 * BMBTTimerHeaderWrite()
 *     Description:
 *         Write out the header after a given timeout so the radio does not
 *         fight us when writing to the screen
 *     Params:
 *         void *ctx - The context
 *     Returns:
 *         void
 */
void BMBTTimerHeaderWrite(void *ctx)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        context->status.displayMode == BMBT_DISPLAY_ON
    ) {
        if (context->timerHeaderIntervals != BMBT_MENU_HEADER_TIMER_OFF) {
            uint16_t time = context->timerHeaderIntervals * BMBT_HEADER_TIMER_WRITE_INT;
            if (time >= BMBT_HEADER_TIMER_WRITE_TIMEOUT) {
                BMBTHeaderWrite(context);
                // Increment the intervals so we aren't called again
                context->timerHeaderIntervals = BMBT_MENU_HEADER_TIMER_OFF;
            } else {
                context->timerHeaderIntervals++;
            }
        }
    }
}

/**
 * BMBTTimerMenuWrite()
 *     Description:
 *         Write out the menu after a given timeout so the radio does not
 *         fight us when re-writing the menu to the screen
 *     Params:
 *         void *ctx - The context
 *     Returns:
 *         void
 */
void BMBTTimerMenuWrite(void *ctx)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        context->status.displayMode == BMBT_DISPLAY_ON
    ) {
        if (context->timerMenuIntervals != BMBT_MENU_HEADER_TIMER_OFF) {
            uint16_t time = context->timerMenuIntervals * BMBT_MENU_TIMER_WRITE_INT;
            if (time == BMBT_MENU_TIMER_WRITE_TIMEOUT) {
                switch (context->menu) {
                    case BMBT_MENU_MAIN:
                        BMBTMenuMain(context);
                        break;
                    case BMBT_MENU_DASHBOARD:
                    case BMBT_MENU_DASHBOARD_FRESH:
                        BMBTMenuDashboard(context);
                        break;
                    case BMBT_MENU_DEVICE_SELECTION:
                        BMBTMenuDeviceSelection(context);
                        break;
                    case BMBT_MENU_SETTINGS:
                        BMBTMenuSettings(context);
                        break;
                    case BMBT_MENU_SETTINGS_AUDIO:
                        BMBTMenuSettingsAudio(context);
                        break;
                    case BMBT_MENU_SETTINGS_COMFORT:
                        BMBTMenuSettingsComfort(context);
                        break;
                    case BMBT_MENU_SETTINGS_COMFORT_TIME:
                        BMBTMenuSettingsComfortTime(context);
                        break;
                    case BMBT_MENU_SETTINGS_CALLING:
                        BMBTMenuSettingsCalling(context);
                        break;
                    case BMBT_MENU_SETTINGS_UI:
                        BMBTMenuSettingsUI(context);
                        break;
                    case BMBT_MENU_NONE:
                        if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x01) {
                            BMBTMenuDashboard(context);
                        } else {
                            BMBTMenuMain(context);
                        }
                        break;
                }
                // Increment the intervals so we aren't called again
                context->timerMenuIntervals = BMBT_MENU_HEADER_TIMER_OFF;
            } else {
                context->timerMenuIntervals++;
            }
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
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        context->status.displayMode == BMBT_DISPLAY_ON &&
        ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) != CONFIG_SETTING_OFF
    ) {
        // Display the main text if there isn't a timeout set
        if (context->mainDisplay.timeout > 0) {
            context->mainDisplay.timeout--;
        } else {
            if (context->mainDisplay.length > BMBT_MAIN_AREA_LEN) {
                char text[BMBT_DISPLAY_TEXT_LEN + 1] = {0};
                uint8_t textLength = BMBT_DISPLAY_TEXT_LEN;
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
                BMBTGTWriteTitle(context, text);
                // Pause at the beginning of the text
                if (context->mainDisplay.index == 0) {
                    context->mainDisplay.timeout = 5;
                }
                if (idxEnd >= context->mainDisplay.length) {
                    // Pause at the end of the text
                    context->mainDisplay.timeout = 2;
                    context->mainDisplay.index = 0;
                } else {
                    if (ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) ==
                        BMBT_METADATA_MODE_CHUNK
                    ) {
                        context->mainDisplay.timeout = 2;
                        context->mainDisplay.index += BMBT_MAIN_AREA_LEN;
                    } else {
                        context->mainDisplay.index++;
                    }
                }
            } else {
                if (context->mainDisplay.index == 0) {
                    BMBTGTWriteTitle(context, context->mainDisplay.text);
                }
                context->mainDisplay.index = 1;
            }
        }
    }
}
