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
    Context.mainDisplay = UtilsDisplayValueInit(
        LocaleGetText(LOCALE_STRING_BLUETOOTH),
        BMBT_DISPLAY_OFF
    );
    Context.navZoom = -1;
    Context.navZoomTime = 0;

    if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x02) {
        Context.bt->carPlay = 1;
        IBusCommandCarplayDisplay(Context.ibus, 1);
    } else {
        Context.bt->carPlay = 0;
        uint8_t pkt[] = { 0x48, 0x08 };
        IBusSendCommand(Context.ibus, IBUS_DEVICE_BMBT, IBUS_DEVICE_LOC, pkt, sizeof(pkt));
        IBusCommandCarplayDisplay(Context.ibus, 0);
    }

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
        IBUS_EVENT_GT_MENU_BUFFER_UPDATE,
        &BMBTIBusGTMenuBufferUpdate,
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
        IBUS_EVENT_MONITOR_CONTROL,
        &BMBTMonitorControl,
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
    EventRegisterCallback(
        IBUS_EVENT_MONITOR_STATUS,
        &BMBTIBusMonitorStatus,
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
        IBUS_EVENT_GT_MENU_BUFFER_UPDATE,
        &BMBTIBusGTMenuBufferUpdate
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
        IBUS_EVENT_MONITOR_STATUS,
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
 * BMBTGTBufferFlush()
 *     Description:
 *         Flush or set the flag to flush the buffer to the menu.
 *         For the new UI, this must be done immediately while on the old UI
 *         we should wait for the GT to confirm the buffer writes beforehand
 *         to ensure we don't end up with empty menu indicies
 *     Params:
 *         BMBTContext_t *context - The BMBT context
 *     Returns:
 *         void
 */
static void BMBTGTBufferFlush(BMBTContext_t *context)
{
    if (context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI &&
        context->ibus->moduleStatus.NAV == 0
    ) {
        context->status.menuBufferStatus = BMBT_MENU_BUFFER_FLUSH;
    } else {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
    }
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
        context->status.radType == IBUS_RADIO_TYPE_C43 ||
        context->ibus->moduleStatus.NAV == 0
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
 *         uint8_t lastIdx - Number of additional rows to clear
 *     Returns:
 *         void
 */
static void BMBTGTWriteIndex(
    BMBTContext_t *context,
    uint8_t index,
    char *text,
    uint8_t clearIdxs
) {

    uint8_t stringLength = strlen(text);
    uint8_t newTextLength = stringLength + clearIdxs + 1;
    if (context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI) {
        if (stringLength > IBUS_DATA_GT_MKIII_MAX_IDX_LEN) {
            newTextLength = newTextLength - (stringLength - IBUS_DATA_GT_MKIII_MAX_IDX_LEN);
            stringLength = IBUS_DATA_GT_MKIII_MAX_IDX_LEN;
        }
        if (index + clearIdxs < 7) {
            index = index + 0x40;
        }
        newTextLength = IBUS_DATA_GT_MKIII_MAX_IDX_LEN + clearIdxs + 1;
    } else {
        index = index + 0x40;
    }
    context->status.navIndexType = IBUS_CMD_GT_WRITE_INDEX_TMC;
    char newText[newTextLength + 1];
    memset(&newText, 0x20, newTextLength);
    strncpy(newText, text, stringLength);
    stringLength = newTextLength - (clearIdxs + 1);
    while (stringLength < newTextLength) {
        newText[stringLength] = 0x06;
        stringLength++;
    }
    newText[newTextLength] = '\0';
    IBusCommandGTWriteIndexTMC(context->ibus, index, newText);
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
    if (context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI ||
        context->ibus->moduleStatus.NAV == 0
    ) {
        IBusCommandGTWriteTitleArea(context->ibus, text);
    } else {
        IBusCommandGTWriteTitleIndex(context->ibus, text);
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    }
}


/**
 * BMBTGTWriteTitleIndex()
 *     Description:
 *         Wrapper to automatically account for the nav type when
 *         writing the menu title index
 *     Params:
 *         BMBTContext_t *context - The context
 *         char *text - The text to write
 *     Returns:
 *         void
 */
static void BMBTGTWriteTitleIndex(BMBTContext_t *context, char *text)
{
    if (context->ibus->gtVersion < IBUS_GT_MKIII_NEW_UI &&
        context->ibus->moduleStatus.NAV == 0
    ) {
        if (strlen(text) > IBUS_DATA_GT_MKIII_MAX_TITLE_LEN) {
             char shortText[IBUS_DATA_GT_MKIII_MAX_TITLE_LEN + 1] = {0};
             UtilsStrncpy(shortText, text, IBUS_DATA_GT_MKIII_MAX_TITLE_LEN + 1);
             IBusCommandGTWriteIndexTitle(context->ibus, shortText);
        } else {
            IBusCommandGTWriteIndexTitle(context->ibus, text);
        }
        IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
    } else {
        IBusCommandGTWriteIndexTitleNGUI(context->ibus, text);
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
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_MAIN_MENU));
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_DASHBOARD, LocaleGetText(LOCALE_STRING_DASHBOARD), 0);
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_DEVICE_SELECTION, LocaleGetText(LOCALE_STRING_DEVICES), 0);
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_SETTINGS, LocaleGetText(LOCALE_STRING_SETTINGS), 5);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_MAIN;
}

static void BMBTMenuDashboardUpdateOBCValues(BMBTContext_t *context)
{
    if (ConfigGetSetting(CONFIG_SETTING_BMBT_DASHBOARD_OBC) == CONFIG_SETTING_OFF) {
        if (context->ibus->gtVersion == IBUS_GT_MKIV_STATIC) {
           IBusCommandGTWriteIndexStatic(context->ibus, 0x45, "\x06");
        }
        return;
    }
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
        context->status.navIndexType = IBUS_CMD_GT_WRITE_STATIC;
        BMBTGTBufferFlush(context);
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
        BMBTGTBufferFlush(context);
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
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_DEVICES));
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
        BMBTGTWriteIndex(context, BMBT_MENU_IDX_CLEAR_PAIRING, LocaleGetText(LOCALE_STRING_CLEAR_PAIRINGS), 5);
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
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_DEVICE_SELECTION;
}

static void BMBTMenuSettings(BMBTContext_t *context)
{
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_SETTINGS));
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
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_SETTINGS;
}

static void BMBTMenuSettingsAbout(BMBTContext_t *context)
{
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_SETTINGS_ABOUT));
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
        4
    );
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_SETTINGS_ABOUT;
}

static void BMBTMenuSettingsAudio(BMBTContext_t *context)
{
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_SETTINGS_AUDIO));
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
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_SETTINGS_AUDIO;
}

static void BMBTMenuSettingsComfort(BMBTContext_t *context)
{
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_SETTINGS_COMFORT));
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
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_SETTINGS_COMFORT_AUTOZOOM, autoZoomText, 1);
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_SETTINGS_COMFORT;
}

static void BMBTMenuSettingsCalling(BMBTContext_t *context)
{
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_SETTINGS_CALLING));
    if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_OFF) {
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
        volOffsetSkip = 4;
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
                3
            );
        } else if (telMode == CONFIG_SETTING_TEL_MODE_NO_MUTE) {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_NO_MUTE),
                3
            );
        } else {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_CALLING_MODE,
                LocaleGetText(LOCALE_STRING_MODE_DEFAULT),
                3
            );
        }
    }
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
    context->menu = BMBT_MENU_SETTINGS_CALLING;
}

static void BMBTMenuSettingsUI(BMBTContext_t *context)
{
    BMBTGTWriteTitleIndex(context, LocaleGetText(LOCALE_STRING_SETTINGS_UI));
    if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU,
            LocaleGetText(LOCALE_STRING_MENU_MAIN),
            0
        );
    } else if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x02 ) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU,
            "Menu: CarPlay",
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
            BMBT_MENU_IDX_SETTINGS_IU_DASH_OBC,
            LocaleGetText(LOCALE_STRING_DASH_OBC_ON),
            0
        );
    } else if (dashboardOBC == CONFIG_SETTING_OFF) {
        BMBTGTWriteIndex(
            context,
            BMBT_MENU_IDX_SETTINGS_IU_DASH_OBC,
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
    char localeName[5] = {0};
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
        case CONFIG_SETTING_LANGUAGE_POLISH:
            strncpy(localeName, "PL", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_FRENCH:
            strncpy(localeName, "FR", 2);
            break;
        case CONFIG_SETTING_LANGUAGE_AUTO:
        default:
            strncpy(localeName, "Auto", 4);
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
    BMBTGTWriteIndex(context, BMBT_MENU_IDX_BACK, LocaleGetText(LOCALE_STRING_BACK), 0);
    BMBTGTBufferFlush(context);
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
        BMBTGTBufferFlush(context);
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
    } else if (selectedIdx == BMBT_MENU_IDX_BACK) {
        BMBTMenuSettings(context);
    }
    if (selectedIdx != BMBT_MENU_IDX_BACK) {
        BMBTGTBufferFlush(context);
    }
}

static void BMBTSettingsUpdateCalling(BMBTContext_t *context, uint8_t selectedIdx)
{
    if (selectedIdx == BMBT_MENU_IDX_SETTINGS_CALLING_HFP) {
        uint8_t value = ConfigGetSetting(CONFIG_SETTING_HFP);
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (value == 0x00) {
                ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_ON);
                BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_ON), 0);
                BC127CommandProfileOpen(context->bt, "HFP");
            } else {
                ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_OFF);
                BC127CommandClose(context->bt, context->bt->activeDevice.hfpId);
                BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_HANDSFREE_OFF), 0);
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
        BMBTGTBufferFlush(context);
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
            BMBTGTBufferFlush(context);
            BMBTGTWriteTitle(context, LocaleGetText(LOCALE_STRING_BLUETOOTH));
        }
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_UI_DEFAULT_MENU) {
        if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x00) {
            ConfigSetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU, 0x01);
            BMBTGTWriteIndex(context, selectedIdx, LocaleGetText(LOCALE_STRING_MENU_DASHBOARD), 0);
        } else if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) == 0x01){
            ConfigSetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU, 0x02);
            BMBTGTWriteIndex(context, selectedIdx, "Menu: CarPlay", 0);
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
            context->ibus->vehicleType != IBUS_VEHICLE_TYPE_E46 &&
            context->ibus->vehicleType != IBUS_VEHICLE_TYPE_E8X
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
    } else if (selectedIdx == BMBT_MENU_IDX_SETTINGS_IU_DASH_OBC) {
        if (ConfigGetSetting(CONFIG_SETTING_BMBT_DASHBOARD_OBC) == CONFIG_SETTING_ON) {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_IU_DASH_OBC,
                LocaleGetText(LOCALE_STRING_DASH_OBC_OFF),
                0
            );
            ConfigSetSetting(
                CONFIG_SETTING_BMBT_DASHBOARD_OBC,
                CONFIG_SETTING_OFF
            );
        } else {
            BMBTGTWriteIndex(
                context,
                BMBT_MENU_IDX_SETTINGS_IU_DASH_OBC,
                LocaleGetText(LOCALE_STRING_DASH_OBC_ON),
                0
            );
            ConfigSetSetting(
                CONFIG_SETTING_BMBT_DASHBOARD_OBC,
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
        if (selectedLanguage == CONFIG_SETTING_LANGUAGE_AUTO) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_ENGLISH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_ENGLISH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_ESTONIAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_ESTONIAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_GERMAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_GERMAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_ITALIAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_ITALIAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_RUSSIAN;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_RUSSIAN) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_SPANISH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_SPANISH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_POLISH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_POLISH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_FRENCH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_FRENCH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_DUTCH;
        } else if (selectedLanguage == CONFIG_SETTING_LANGUAGE_DUTCH) {
            selectedLanguage = CONFIG_SETTING_LANGUAGE_AUTO;
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
        BMBTGTBufferFlush(context);
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
        if (strlen(context->bt->activeDevice.deviceName) > 0) {
            BMBTHeaderWriteDeviceName(context, context->bt->activeDevice.deviceName);
            IBusCommandGTUpdate(context->ibus, IBUS_CMD_GT_WRITE_ZONE);
        }
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
            if (pkt[IBUS_PKT_DB1] == IBUS_DEVICE_BMBT_Button_Num1 || context->bt->carPlay != 1) {
                if (context->bt->playbackStatus == BT_AVRCP_STATUS_PLAYING) {
                    BTCommandPause(context->bt);
                } else {
                    BTCommandPlay(context->bt);
                }
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
                    if (context->ibus->moduleStatus.NAV == 1) {
                        IBusCommandRADDisableMenu(context->ibus);
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
                context->status.displayMode = BMBT_DISPLAY_TONE_SEL_INFO;
            } else if (pkt[5] == IBUS_DEVICE_BMBT_Button_SEL) {
                context->status.displayMode = BMBT_DISPLAY_TONE_SEL_INFO;
            }
        }
    }
    // Handle calls at any time
/*
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
*/
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
        if (context->status.tvStatus == BMBT_TV_STATUS_OFF &&
            context->ibus->moduleStatus.NAV == 1
        ) {
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
        // Only set Audio + OBC mode if we have a Nav computer equipped
        // This message locks up certain Video Modules
        if (context->ibus->moduleStatus.NAV == 1) {
            IBusCommandRADDisableMenu(context->ibus);
        }
        context->timerHeaderIntervals = BMBT_MENU_HEADER_TIMER_OFF;
        context->timerMenuIntervals = BMBT_MENU_HEADER_TIMER_OFF;
        context->status.playerMode = BMBT_MODE_ACTIVE;
        context->status.displayMode = BMBT_DISPLAY_ON;
        BMBTTriggerWriteHeader(context);
        BMBTTriggerWriteMenu(context);
    } else if (requestedCommand == IBUS_CDC_CMD_RANDOM_MODE) {
        // Enable & Disable the UI based on the "Random" playback mode setting
        // This adds support for GTs that run without radio, like the R51/R52/R53
        if (pkt[IBUS_PKT_DB2] == 0x01) {
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
        } else {
            IBusCommandRADClearMenu(context->ibus);
            context->menu = BMBT_MENU_NONE;
            context->status.playerMode = BMBT_MODE_INACTIVE;
            context->status.displayMode = BMBT_DISPLAY_OFF;
            BMBTSetMainDisplayText(context, LocaleGetText(LOCALE_STRING_BLUETOOTH), 0, 0);
        }
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

/**
 * BMBTIBusMonitorStatus()
 *     Description:
 *         Look for updates from the GT in order to ensure that we do not
 *         respond to input while the VM (GT) is in "Reverse Camera" mode
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusMonitorStatus(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[IBUS_PKT_LEN] != 0x04) {
        return;
    }
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_JNAV &&
        (pkt[IBUS_PKT_DB1] & 0xF) != 0
    ) {
        // Sent after pin 17 is left floating again
        LogDebug(LOG_SOURCE_UI,"MonStatus Leaving CarPlay");
        context->status.displayMode = BMBT_DISPLAY_ON;
        context->bt->carPlay = 0;
        IBusCommandRADEnableMenu(context->ibus);
    } else if ((pkt[IBUS_PKT_DB1] & 0xF) != 0) {
        // Entering Reverse Camera mode
        LogDebug(LOG_SOURCE_UI,"MonStatus Entering CarPlay");
        context->status.displayMode = BMBT_DISPLAY_REVERSE_CAM_INIT;
        context->bt->carPlay = 1;
        IBusCommandRADDisableMenu(context->ibus);
    }
}

/**
 * BMBTIBusGTMenuBufferUpdate()
 *     Description:
 *         Flush the menu buffer if we get a buffer update from the GT
 *     Params:
 *         void *context - A void pointer to the BMBTContext_t struct
 *         uint8_t *pkt - A pointer to the data packet
 *     Returns:
 *         void
 */
void BMBTIBusGTMenuBufferUpdate(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (context->status.menuBufferStatus == BMBT_MENU_BUFFER_FLUSH) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
        context->status.menuBufferStatus = BMBT_MENU_BUFFER_OK;
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
                BMBTGTBufferFlush(context);
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
    // If we cannot write to the display, we are not the active
    // player, or this is a "Zone" (header) update, then ignore the message.
    if (context->status.playerMode != BMBT_MODE_ACTIVE ||
        context->status.displayMode != BMBT_DISPLAY_ON ||
        pkt[IBUS_PKT_DB1] != IBUS_CMD_GT_WRITE_ZONE
    ) {
        return;
    }
    // If the update does not match our current menu state, override it
    if (pkt[IBUS_PKT_DB1] != context->status.navIndexType) {
        IBusCommandGTUpdate(context->ibus, context->status.navIndexType);
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

    // We may need to add additional `updateType` values in the future
    if (config == CONFIG_SETTING_OFF ||
        context->status.displayMode != BMBT_DISPLAY_ON ||
        updateType == IBUS_SENSOR_VALUE_GEAR_POS
    ) {
        return;
    }
    if (context->ibus->ambientTemperatureCalculated[0] == 0) {
       IBusCommandIKEOBCControl(
            context->ibus,
            IBUS_IKE_OBC_PROPERTY_TEMPERATURE,
            IBUS_IKE_OBC_PROPERTY_REQUEST_TEXT
        );
    }

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
        BMBTGTBufferFlush(context);
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
    context->status.displayMode = BMBT_DISPLAY_TONE_SEL_INFO;
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
    if (pkt[IBUS_PKT_DB2] == 0x30) {
        // The BM24 uses this layout to write out multiple headers
        // at the same time. Ensure the text is ours
        if (pkt[IBUS_PKT_DB1] != IBUS_C43_TITLE_MODE &&
            pkt[6] != 0x20 &&
            pkt[7] != 0x20 &&
            pkt[8] != 0x07
        ) {
            textIsOurs = 1;
        } else {
            context->status.radType = IBUS_RADIO_TYPE_BM24;
        }
    }
    if (context->status.playerMode == BMBT_MODE_ACTIVE &&
        textIsOurs == 0
    ) {
        uint8_t pktLen = (uint8_t) pkt[1] + 2;
        uint8_t textLen = 0;
        if (pktLen > 7) {
            textLen = pktLen - 7;
        }
        char text[textLen + 1];
        memset(&text, 0, textLen + 1);
        int8_t idx = 0;
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
        while (idx > 0 && (text[idx] == 0x20 || text[idx] == 0x00)) {
            text[idx] = '\0';
            idx--;
        }
        text[textLen] = '\0';
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
            if (context->ibus->moduleStatus.NAV == 1) {
                IBusCommandRADDisableMenu(context->ibus);
            }
            context->status.navState = BMBT_NAV_STATE_ON;
        }
        if (pkt[IBUS_PKT_DB1] == IBUS_RAD_HIDE_BODY &&
            (context->status.displayMode == BMBT_DISPLAY_ON ||
             context->status.displayMode == BMBT_DISPLAY_TONE_SEL_INFO)
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
 * BMBTMonitorControl()
 *     Description:
 *         Listen for the GT/VM -> BMBT Television status messages
 *         to return from CarPlay / Reverse mode
 *     Params:
 *         void *ctx - The context
 *         uint8_t *pkt - The IBus Message received
 *     Returns:
 *         void
 */
void BMBTMonitorControl(void *ctx, uint8_t *pkt)
{
    BMBTContext_t *context = (BMBTContext_t *) ctx;
    if (pkt[IBUS_PKT_DB1] == 0x12 && pkt[IBUS_PKT_DB2] == 0x11) {
        if (context->status.displayMode == BMBT_DISPLAY_REVERSE_CAM) {
            LogDebug(LOG_SOURCE_UI,"MonControl Leaving CarPlay");
            context->status.displayMode = BMBT_DISPLAY_ON;
            context->bt->carPlay = 0;
            if (context->menu != BMBT_MENU_DASHBOARD_FRESH) {
                context->menu = BMBT_MENU_NONE;
            }
            if (context->ibus->moduleStatus.NAV == 1) {
                IBusCommandRADDisableMenu(context->ibus);
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
            IBusCommandRADEnableMenu(context->ibus);
        } else if (context->status.displayMode == BMBT_DISPLAY_REVERSE_CAM_INIT) {
            LogDebug(LOG_SOURCE_UI,"MonControl Entering CarPlay");
            context->status.displayMode = BMBT_DISPLAY_REVERSE_CAM;
            context->bt->carPlay = 1;
            IBusCommandRADDisableMenu(context->ibus);
        }
    }
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

    uint8_t distUnit = IBusGetConfigDistance(pkt);
    if (distUnit != ConfigGetDistUnit()) {
        ConfigSetDistUnit(distUnit);
    }

// Update also the language if we support it
// https://github.com/piersholt/wilhelm-docs/blob/master/ike/15.md

    uint8_t langIbus = IBusGetConfigLanguage(pkt);;
    uint8_t lang = 255;

    switch (langIbus) {
        case 0: // DE
            lang = CONFIG_SETTING_LANGUAGE_GERMAN;
            break;
        case 3: // IT
            lang = CONFIG_SETTING_LANGUAGE_ITALIAN;
            break;
        case 4: // ES
            lang = CONFIG_SETTING_LANGUAGE_SPANISH;
            break;
        case 6: // FR
            lang = CONFIG_SETTING_LANGUAGE_FRENCH;
            break;
        case 1: // GB
        case 2: // US
        case 5: // JP
        case 7: // CA
        case 8: // GOLF
        default:
            lang = CONFIG_SETTING_LANGUAGE_ENGLISH;
            break;
    }

    uint8_t bbLang = ConfigGetSetting(CONFIG_SETTING_LANGUAGE);

    if (((bbLang == CONFIG_SETTING_LANGUAGE_AUTO) || (bbLang == 255) || (bbLang >= 0x80)) && (lang != (bbLang & 0x0F))) {
// overwrite only when not flagged as user-forced
        ConfigSetSetting(CONFIG_SETTING_LANGUAGE, (lang | 0x80));
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
                    case BMBT_MENU_SETTINGS_ABOUT:
                        BMBTMenuSettingsAbout(context);
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
                        if (ConfigGetSetting(CONFIG_SETTING_BMBT_DEFAULT_MENU) != 0x00) {
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
