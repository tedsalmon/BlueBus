/*
 * File: bmbt.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the BoardMonitor UI Mode handler
 */
#include "bmbt.h"
static BMBTContext_t Context;

uint8_t menuSettings[] = {
    BMBT_MENU_IDX_SETTINGS_ABOUT,
    BMBT_MENU_IDX_SETTINGS_AUDIO,
    BMBT_MENU_IDX_SETTINGS_CALLING,
    BMBT_MENU_IDX_SETTINGS_COMFORT,
    BMBT_MENU_IDX_SETTINGS_UI
};

uint16_t menuSettingsLabelIndices[] = {
    LOCALE_STRING_ABOUT,
    LOCALE_STRING_AUDIO,
    LOCALE_STRING_CALLING,
    LOCALE_STRING_COMFORT,
    LOCALE_STRING_UI
};

void BMBTInit(BC127_t *bt, IBus_t *ibus)
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
        IBUS_EVENT_RADUpdateMainArea,
        &BMBTRADUpdateMainArea,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_ScreenModeSet,
        &BMBTScreenModeSet,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_ScreenModeUpdate,
        &BMBTScreenModeUpdate,
        &Context
    );
    EventRegisterCallback(
        IBUS_EVENT_TV_STATUS,
        &BMBTTVStatusUpdate,
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
        BC127Event_DeviceConnected,
        &BMBTBC127DeviceConnected
    );
    EventUnregisterCallback(
        BC127Event_DeviceDisconnected,
        &BMBTBC127DeviceDisconnected
    );
    EventUnregisterCallback(
        BC127Event_MetadataChange,
        &BMBTBC127Metadata
    );
    EventUnregisterCallback(
        BC127Event_Boot,
        &BMBTBC127Ready
    );
    EventUnregisterCallback(
        BC127Event_PlaybackStatusChange,
        &BMBTBC127PlaybackStatus
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
        IBUS_EVENT_RADUpdateMainArea,
        &BMBTRADUpdateMainArea
    );
    EventUnregisterCallback(
        IBUS_EVENT_ScreenModeSet,
        &BMBTScreenModeSet
    );
    EventUnregisterCallback(
        IBUS_EVENT_ScreenModeUpdate,
        &BMBTScreenModeUpdate
    );
    EventUnregisterCallback(
        IBUS_EVENT_TV_STATUS,
        &BMBTTVStatusUpdate
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
    strncpy(context->mainDisplay.text, str, UTILS_DISPLAY_TEXT_SIZE);
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
        context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED
    ) {
        BMBTGTWriteTitle(context, LocaleGetText(LOCALE_STRING_BLUETOOTH));
    } else {
        BMBTMainAreaRefresh(context);
    }
    if (context->bt->activeDevice.deviceId != 0) {
        BMBTHeaderWriteDeviceName(context, context->bt->activeDevice.deviceName);
    } else {
        BMBTHeaderWriteDeviceName(context, LocaleGetText(LOCALE_STRING_NO_DEVICE));
    }
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "||");
    } else {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_PB_STAT, "> ");
    }

    unsigned char tempMode = ConfigGetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS);
    
    if ((ConfigGetSetting(CONFIG_SETTING_METADATA_MODE) == CONFIG_SETTING_OFF)&&(tempMode!=CONFIG_SETTING_OFF)) {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_BT, "    ");
    } else {
        IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_BT, "BT  ");
    }
    IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);

    unsigned char valueType = 0;
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
    if (valueType>0) {
        EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
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

static void BMBTMenuDashboardUpdate(BMBTContext_t *context, char *f1, char *f2, char *f3)
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
    if (context->ibus->gtVersion == IBUS_GT_MKIV_STATIC) {
        IBusCommandGTWriteIndexStatic(context->ibus, 0x41, f1);
        IBusCommandGTWriteIndexStatic(context->ibus, 0x42, f2);
        IBusCommandGTWriteIndexStatic(context->ibus, 0x43, f3);

        if (ConfigGetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS) != CONFIG_SETTING_OFF) {
            char tempUnit = 'C';
            char ambtempstr[7] = {0};
            char oiltempstr[8] = {0};
            char cooltempstr[8] = {0};
            
            if (ConfigGetSetting(CONFIG_SETTING_TEMPERATURE) == CONFIG_SETTING_TEMP_FAHRENHEIT) {
                tempUnit = 'F';
            }

            int ambtemp = context->ibus->ambientTemperature;
            int oiltemp = context->ibus->oilTemperature;
            int cooltemp = context->ibus->coolantTemperature;

            if (tempUnit == 'F') {
                ambtemp = (ambtemp * 1.8 + 32+0.5);
                if (oiltemp > 0) {
                    oiltemp = (oiltemp * 1.8 + 32+0.5);
                }
                if (cooltemp > 0) {
                    cooltemp = (cooltemp * 1.8 + 32+0.5);
                }
            }

            snprintf(ambtempstr, 7, "A:%+d", ambtemp);
            if (cooltemp > 0) { 
                snprintf(cooltempstr, 8, "C:%d, ", cooltemp);
            }
            if (oiltemp > 0) {
                snprintf(oiltempstr, 8, "O:%d, ", oiltemp);
            };
            char temperature[29]={0};

            snprintf(temperature, 29, "Temp: %s%s%s\xB0%c", oiltempstr, cooltempstr, ambtempstr, tempUnit);
            IBusCommandGTWriteIndexStatic(context->ibus, 0x45, temperature);
        }

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

        if (ConfigGetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS) != CONFIG_SETTING_OFF) {
            char tempUnit = 'C';
            char ambtempstr[7] = {0};
            char oiltempstr[8] = {0};
            char cooltempstr[8] = {0};
            
            if (ConfigGetSetting(CONFIG_SETTING_TEMPERATURE) == CONFIG_SETTING_TEMP_FAHRENHEIT) {
                tempUnit = 'F';
            }

            int ambtemp = context->ibus->ambientTemperature;
            int oiltemp = context->ibus->oilTemperature;
            int cooltemp = context->ibus->coolantTemperature;

            if (tempUnit == 'F') {
                ambtemp = (ambtemp * 1.8 + 32+0.5);
                if (oiltemp > 0) {
                    oiltemp = (oiltemp * 1.8 + 32+0.5);
                }
                if (cooltemp > 0) {
                    cooltemp = (cooltemp * 1.8 + 32+0.5);
                }
            }

            snprintf(ambtempstr, 7, "A:%d", ambtemp);
            if (cooltemp > 0) { 
                snprintf(cooltempstr, 8, "C:%d, ", cooltemp);
            }
            if (oiltemp > 0) {
                snprintf(oiltempstr, 8, "O:%d, ", oiltemp);
            };
            char temperature[29]={0};

            snprintf(temperature, 29, "Temp: %s%s%s\xB0%c", oiltempstr, cooltempstr, ambtempstr, tempUnit);
            IBusCommandGTWriteIndex(context->ibus, 4, temperature);
        }
        context->status.navIndexType = IBUS_CMD_GT_WRITE_INDEX;
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

static void BMBTMenuDashboard(BMBTContext_t *context)
{
    char title[BC127_METADATA_FIELD_SIZE] = {0};
    char artist[BC127_METADATA_FIELD_SIZE] = {0};
    char album[BC127_METADATA_FIELD_SIZE] = {0};
    strncpy(title, context->bt->title, BC127_METADATA_FIELD_SIZE - 1);
    strncpy(artist, context->bt->artist, BC127_METADATA_FIELD_SIZE - 1);
    strncpy(album, context->bt->album, BC127_METADATA_FIELD_SIZE - 1);
    if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
        if (strlen(title) == 0) {
            strncpy(title, LocaleGetText(LOCALE_STRING_NOT_PLAYING), BC127_METADATA_FIELD_SIZE - 1);
            strncpy(artist, " ", 2);
            strncpy(album, " ", 2);
        }
    } else {
        if (strlen(title) == 0) {
            strncpy(title, LocaleGetText(LOCALE_STRING_UNKNOWN_TITLE), BC127_METADATA_FIELD_SIZE - 1);
        }
        if (strlen(artist) == 0) {
            strncpy(artist, LocaleGetText(LOCALE_STRING_UNKNOWN_ARTIST), BC127_METADATA_FIELD_SIZE - 1);
        }
        if (strlen(album) == 0) {
            strncpy(album, LocaleGetText(LOCALE_STRING_UNKNOWN_ALBUM), BC127_METADATA_FIELD_SIZE - 1);
        }
    }
    BMBTMenuDashboardUpdate(context, title, artist, album);
    context->menu = BMBT_MENU_DASHBOARD;
}

static void BMBTMenuDeviceSelection(BMBTContext_t *context)
{
    uint8_t idx;
    uint8_t screenIdx = 2;
    if (context->bt->discoverable == BC127_STATE_ON) {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_ON), 0);
    } else {
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_OFF), 0);
    }
    BC127PairedDevice_t *dev = 0;
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
            if (strcmp(dev->macId, context->bt->activeDevice.macId) == 0) {
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
            feedCount = BMBT_MENU_IDX_BACK - (menuSettingsSize + 1);
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
    unsigned char currentVolume = ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL);
    char volText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    if (currentVolume > 0x30) {
        unsigned char gain = (currentVolume - 0x30) / 2;
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_NEG_DB), gain);
    } else if (currentVolume == 0) {
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_24_DB));
    } else if (currentVolume == 0x30) {
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_0_DB));
    } else {
        unsigned char gain = (0x30 - currentVolume) / 2;
        snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_POS_DB), gain);
    }
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_AUDIO_DAC_GAIN,
        volText,
        0
    );
    if (ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_ON) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT,
            LocaleGetText(LOCALE_STRING_DSP_DIGITAL),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT,
            LocaleGetText(LOCALE_STRING_DSP_ANALOG),
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
            1
        );
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_AUDIO));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_AUDIO;
}

static void BMBTMenuSettingsComfort(BMBTContext_t *context)
{
    unsigned char comfortLock = ConfigGetComfortLock();
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
    unsigned char comfortUnlock = ConfigGetComfortUnlock();
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
    unsigned char blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
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
            2
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_COMFORT_PARKING_LAMPS,
            LocaleGetText(LOCALE_STRING_PARK_LAMPS_OFF),
            2
        );
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 1);
    IBusCommandGTWriteIndexTitle(context->ibus, LocaleGetText(LOCALE_STRING_SETTINGS_COMFORT));
    IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    context->menu = BMBT_MENU_SETTINGS_COMFORT;
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
    if (ConfigGetSetting(CONFIG_SETTING_MIC_BIAS) == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_MIC_BIAS,
            LocaleGetText(LOCALE_STRING_MIC_BIAS_OFF),
            0
        );
    } else {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_MIC_BIAS,
            LocaleGetText(LOCALE_STRING_MIC_BIAS_ON),
            0
        );
    }
    unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
    if (micGain > 21) {
        micGain = 0;
    }
    char micGainText[BMBT_MENU_STRING_MAX_SIZE] = {0};
    snprintf(micGainText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_MIC_GAIN), (int8_t)BC127CVCGainTable[micGain]);
    BMBTGTWriteIndex(
        context,
        BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN,
        micGainText,
        3
    );
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
    unsigned char metadataMode = ConfigGetSetting(CONFIG_SETTING_METADATA_MODE);
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
    unsigned char tempMode = ConfigGetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS);
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
    unsigned char selectedLanguage = ConfigGetSetting(CONFIG_SETTING_LANGUAGE);
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
        2
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
        unsigned char currentVolume = ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL);
        currentVolume = currentVolume + 2;
        if (currentVolume > 96) {
            currentVolume = 0;
        }
        ConfigSetSetting(CONFIG_SETTING_DAC_AUDIO_VOL, currentVolume);
        char volText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        if (currentVolume > 0x30) {
            unsigned char gain = (currentVolume - 0x30) / 2;
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_NEG_DB), gain);
        } else if (currentVolume == 0) {
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_24_DB));
        } else if (currentVolume == 0x30) {
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_0_DB));
        } else {
            unsigned char gain = (0x30 - currentVolume) / 2;
            snprintf(volText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_VOLUME_POS_DB), gain);
        }
        BMBTGTWriteIndex(context, selectedIdx, volText, 0);
        PCM51XXSetVolume(currentVolume);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_AUDIO_DSP_INPUT) {
        if (ConfigGetSetting(CONFIG_SETTING_USE_SPDIF_INPUT) == CONFIG_SETTING_ON) {
            ConfigSetSetting(CONFIG_SETTING_USE_SPDIF_INPUT, CONFIG_SETTING_OFF);
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_DSP_ANALOG), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_USE_SPDIF_INPUT, CONFIG_SETTING_ON);
            IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_DSP_DIGITAL), 0);
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
        unsigned char value = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
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
        unsigned char value = ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS);
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
        unsigned char comfortLock = ConfigGetComfortLock();
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
        unsigned char comfortUnlock = ConfigGetComfortUnlock();
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
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
}

static void BMBTSettingsUpdateCalling(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_HFP) {
        unsigned char value = ConfigGetSetting(CONFIG_SETTING_HFP);
        if (value == 0x00) {
            ConfigSetSetting(CONFIG_SETTING_HFP, 0x01);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_ON), 0);
            BC127CommandSetProfiles(context->bt, 1, 1, 0, 1);
        } else {
            BC127CommandSetProfiles(context->bt, 1, 1, 0, 0);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_OFF), 0);
            ConfigSetSetting(CONFIG_SETTING_HFP, 0x00);
        }
        BC127CommandReset(context->bt);
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN) {
        unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
        micGain = micGain + 1;
        if (micGain > 21) {
            micGain = 0;
        }
        ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, micGain);
        unsigned char micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
        unsigned char micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
        BC127CommandSetMicGain(
            context->bt,
            micGain,
            micBias,
            micPreamp
        );
        char micGainText[BMBT_MENU_STRING_MAX_SIZE] = {0};
        snprintf(micGainText, BMBT_MENU_STRING_MAX_SIZE, LocaleGetText(LOCALE_STRING_MIC_GAIN), (int8_t)BC127CVCGainTable[micGain]);
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_CALLING_MIC_GAIN,
            micGainText,
            0
        );
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_MIC_BIAS) {
        unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
        unsigned char micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
        if (ConfigGetSetting(CONFIG_SETTING_MIC_BIAS) == CONFIG_SETTING_OFF) {
            BC127CommandSetMicGain(
                context->bt,
                micGain,
                1,
                micPreamp
            );
            ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_ON);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MIC_BIAS_ON), 0);
        } else {
            BC127CommandSetMicGain(
                context->bt,
                micGain,
                0,
                micPreamp
            );
            ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_OFF);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MIC_BIAS_OFF), 0);
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
        unsigned char value = ConfigGetSetting(CONFIG_SETTING_METADATA_MODE);
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
            context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING
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
        unsigned char tempMode = ConfigGetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS);
        if (tempMode == CONFIG_SETTING_OFF) {
            ConfigSetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS, CONFIG_SETTING_TEMP_COOLANT);
            unsigned char valueType = IBUS_SENSOR_VALUE_COOLANT_TEMP;
            EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_COOLANT), 0);
        } else if (tempMode == CONFIG_SETTING_TEMP_COOLANT) {
            ConfigSetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS, CONFIG_SETTING_TEMP_AMBIENT);
            unsigned char valueType = IBUS_SENSOR_VALUE_AMBIENT_TEMP;
            EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_AMBIENT), 0);
        } else if (
                tempMode == CONFIG_SETTING_TEMP_AMBIENT &&
                context->ibus->vehicleType != IBUS_VEHICLE_TYPE_E46_Z4
        ) {
            ConfigSetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS, CONFIG_SETTING_TEMP_OIL);
            unsigned char valueType = IBUS_SENSOR_VALUE_OIL_TEMP;
            EventTriggerCallback(IBUS_EVENT_SENSOR_VALUE_UPDATE, &valueType);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_OIL), 0);
        } else {
            ConfigSetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS, CONFIG_SETTING_OFF);          
            IBusCommandGTWriteZone(context->ibus, BMBT_HEADER_TEMPS, "       ");
            IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_TEMPS_OFF), 0);
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_LANGUAGE) {
        unsigned char selectedLanguage = ConfigGetSetting(CONFIG_SETTING_LANGUAGE);
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
 * BMBTBC127DeviceConnected()
 *     Description:
 *         Handle screen updates when a device connects
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBC127DeviceConnected(void *ctx, unsigned char *data)
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
 * BMBTBC127DeviceDisconnected()
 *     Description:
 *         Handle screen updates when a device disconnects
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBC127DeviceDisconnected(void *ctx, unsigned char *data)
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
 * BMBTBC127Metadata()
 *     Description:
 *         Handle metadata updates from the BC127
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBC127Metadata(void *ctx, unsigned char *data)
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
 * BMBTBC127PlaybackStatus()
 *     Description:
 *         Handle the BC127 playback state changes
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBC127PlaybackStatus(void *ctx, unsigned char *tmp)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.displayMode == BMBT_DISPLAY_ON) {
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
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
 * BMBTBC127Ready()
 *     Description:
 *         Handle the BC127 rebooting gracefully
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *tmp - The data from the event
 *     Returns:
 *         void
 */
void BMBTBC127Ready(void *ctx, unsigned char *tmp)
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
 *         unsigned char *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusBMBTButtonPress(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE) {
        if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_PlayPause ||
            pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_Num1
        ) {
            if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
                BC127CommandPause(context->bt);
            } else {
                BC127CommandPlay(context->bt);
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
            if (context->bt->callStatus == BC127_CALL_ACTIVE) {
                BC127CommandCallEnd(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_INCOMING) {
                BC127CommandCallAnswer(context->bt);
            } else if (context->bt->callStatus == BC127_CALL_OUTGOING) {
                BC127CommandCallEnd(context->bt);
            }
        } else if (context->bt->callStatus == BC127_CALL_INACTIVE &&
                   pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_TEL_Hold
        ) {
            BC127CommandToggleVR(context->bt);
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
 *         unsigned char *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusCDChangerStatus(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    unsigned char requestedCommand = pkt[IBUS_PKT_DB1];
    if (requestedCommand == IBUS_CDC_CMD_STOP_PLAYING) {
        // Stop Playing
        if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING) {
            BC127CommandPause(context->bt);
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
            BC127CommandPlay(context->bt);
        } else if (context->bt->playbackStatus == BC127_AVRCP_STATUS_PLAYING &&
                   context->status.playerMode == BMBT_MODE_INACTIVE
        ) {
            BC127CommandPause(context->bt);
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
            context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED
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
 *         unsigned char *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusGTChangeUIRequest(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[IBUS_PKT_DB1] == 0x02 && pkt[5] == 0x0C) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
            IBusCommandTELSetGTDisplayMenu(context->ibus);
        }
    }
}

void BMBTIBusMenuSelect(void *ctx, unsigned char *pkt)
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
                if (context->bt->discoverable == BC127_STATE_ON) {
                    BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_OFF), 0);
                    state = BC127_STATE_OFF;
                } else {
                    BMBTGTWriteIndex(context, BMBT_MENU_IDX_PAIRING_MODE, LocaleGetText(LOCALE_STRING_PAIRING_ON), 0);
                    state = BC127_STATE_ON;
                    if (context->bt->activeDevice.deviceId != 0) {
                        // To pair a new device, we must disconnect the active one
                        EventTriggerCallback(UIEvent_CloseConnection, 0x00);
                    }
                }
                IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
                BC127CommandBtState(context->bt, context->bt->connectable, state);
            } else if (selectedIdx == BMBT_MENU_IDX_CLEAR_PAIRING) {
                BC127CommandUnpair(context->bt);
                BC127ClearPairedDevices(context->bt);
                BMBTMenuDeviceSelection(context);
            } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
                // Back Button
                BMBTMenuMain(context);
            } else {
                uint8_t deviceId = selectedIdx - BMBT_MENU_IDX_FIRST_DEVICE;
                BC127PairedDevice_t *dev = &context->bt->pairedDevices[deviceId];
                if (strcmp(dev->macId, context->bt->activeDevice.macId) != 0 &&
                    dev != 0
                ) {
                    // Trigger device selection event
                    EventTriggerCallback(
                        UIEvent_InitiateConnection,
                        (unsigned char *)&deviceId
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
        } else if (context->menu == BMBT_MENU_SETTINGS_CALLING) {
            BMBTSettingsUpdateCalling(context, selectedIdx);
        } else if (context->menu == BMBT_MENU_SETTINGS_UI) {
            BMBTSettingsUpdateUI(context, selectedIdx);
        }
    }
}


/**
 * BMBTIBusSensorValueUpdate()
 *     Description:
 *         Respond to sensor value updates emitted on the I/K-Bus
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         unsigned char *type - The update type
 *     Returns:
 *         void
 */
void BMBTIBusSensorValueUpdate(void *ctx, unsigned char *type)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    unsigned char updateType = *type;
    int temp = 0;
    char tempUnit = 'C';
    char redraw = 0;
    char temperature[8] = {0};
    char config = ConfigGetSetting(CONFIG_SETTING_BMBT_TEMP_HEADERS);

    if (context->status.displayMode == BMBT_DISPLAY_ON) {

        if (ConfigGetSetting(CONFIG_SETTING_TEMPERATURE) == CONFIG_SETTING_TEMP_FAHRENHEIT) {
            tempUnit = 'F';
        }

        if (config == CONFIG_SETTING_TEMP_AMBIENT && 
            (updateType == IBUS_SENSOR_VALUE_AMBIENT2_TEMP || updateType == IBUS_SENSOR_VALUE_AMBIENT_TEMP || updateType == IBUS_SENSOR_VALUE_TEMP_UNIT) && 
            context->ibus->ambientTemperature2[0] != 0) {
                snprintf(temperature, 8, "%s\xB0%c", context->ibus->ambientTemperature2, tempUnit);
                redraw = 1;
        } else {
            if (config == CONFIG_SETTING_TEMP_COOLANT &&
                (updateType == IBUS_SENSOR_VALUE_COOLANT_TEMP || updateType == IBUS_SENSOR_VALUE_TEMP_UNIT)
            ) {
                temp = context->ibus->coolantTemperature;
                if (temp != 0) {
                    redraw = 1;
                }
            } else if (config == CONFIG_SETTING_TEMP_AMBIENT &&
                (updateType == IBUS_SENSOR_VALUE_AMBIENT_TEMP || updateType == IBUS_SENSOR_VALUE_AMBIENT2_TEMP || updateType == IBUS_SENSOR_VALUE_TEMP_UNIT)
            ) {
                temp = context->ibus->ambientTemperature;
                redraw = 1;
            } else if (config == CONFIG_SETTING_TEMP_OIL &&
                (updateType == IBUS_SENSOR_VALUE_OIL_TEMP || updateType == IBUS_SENSOR_VALUE_TEMP_UNIT)
            ) {
                temp = context->ibus->oilTemperature;
                if (temp != 0) {
                    redraw = 1;
                }
            }

        if (redraw == 1) {
            if ( tempUnit == 'F' ) {
                temp = temp * 1.8 + 32 + 0.5;
            }
            if (config == CONFIG_SETTING_TEMP_AMBIENT) {
                snprintf(temperature, 8, "%+d\xB0%c", temp, tempUnit);
                } else {
                    snprintf(temperature, 8, "%d\xB0%c", temp, tempUnit);                 
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
            BMBTMenuDashboard(context);
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
 *         unsigned char *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTRADDisplayMenu(void *ctx, unsigned char *pkt)
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
 *         unsigned char *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTRADUpdateMainArea(void *ctx, unsigned char *pkt)
{
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
                context->bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED
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
 * BMBTScreenModeUpdate()
 *     Description:
 *         This callback tracks the screen mode that is broadcast by the GT.
 *         We use this to know if we can write to the screen or not.
 *     Params:
 *         void *ctx - The context
 *         unsigned char *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTScreenModeUpdate(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.playerMode == BMBT_MODE_ACTIVE) {
        if (pkt[IBUS_PKT_DB1] == 0x01 || pkt[IBUS_PKT_DB1] == IBUS_GT_RADIO_SCREEN_OFF) {
            if (context->menu == BMBT_MENU_DASHBOARD) {
                context->menu = BMBT_MENU_DASHBOARD_FRESH;
            } else {
                context->menu = BMBT_MENU_NONE;
            }
            context->status.displayMode = BMBT_DISPLAY_OFF;
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_GT_MENU_CLEAR &&
            context->status.navState == BMBT_NAV_STATE_BOOT
        ) {
            IBusCommandRADDisableMenu(context->ibus);
            context->status.navState = BMBT_NAV_STATE_ON;
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_GT_MENU_CLEAR &&
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
 * BMBTScreenModeSet()
 *     Description:
 *         The GT sends this screen mode post-boot to tell the radio it can
 *         display to the UI. We set the menu to none so that on the next
 *         screen clear, we know to write the UI. Set the navstate state to
 *         "boot" so we know that we need to disable the radio updates on next
 *         screen clear
 *     Params:
 *         void *ctx - The context
 *         unsigned char *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTScreenModeSet(void *ctx, unsigned char *pkt)
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
 *         unsigned char *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTTVStatusUpdate(void *ctx, unsigned char *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    context->status.tvStatus = pkt[IBUS_PKT_DB1];
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
            if (context->mainDisplay.length > 9) {
                char text[10] = {0};
                strncpy(
                    text,
                    &context->mainDisplay.text[context->mainDisplay.index],
                    9
                );
                text[9] = '\0';
                BMBTGTWriteTitle(context, text);
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
                    BMBTGTWriteTitle(context, context->mainDisplay.text);
                }
                context->mainDisplay.index = 1;
            }
        }
    }
}
