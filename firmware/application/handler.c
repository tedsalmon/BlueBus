/*
 * File: handler.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#include "handler.h"
#include "handler/handler_common.h"
#include "lib/bt/bt_common.h"
#include "lib/config.h"
static HandlerContext_t Context;

/**
 * HandlerInit()
 *     Description:
 *         Initialize our context and register all event listeners and
 *         scheduled tasks.
 *     Params:
 *         BT_t *bt - The Bluetooth module Object
 *         IBus_t *ibus - The IBus Object
 *     Returns:
 *         void
 */
void HandlerInit(BT_t *bt, IBus_t *ibus)
{
    Context.bt = bt;
    Context.ibus = ibus;
    uint32_t now = TimerGetMillis();
    Context.btDeviceConnRetries = 0;
    Context.btStartupIsRun = 0;
    Context.btSelectedDevice = 0;
    Context.volumeMode = HANDLER_VOLUME_MODE_NORMAL;
    Context.gtStatus = HANDLER_GT_STATUS_UNCHECKED;
    Context.monitorStatus = HANDLER_MONITOR_STATUS_UNSET;
    Context.uiMode = ConfigGetUIMode();
    Context.seekMode = HANDLER_CDC_SEEK_MODE_NONE;
    Context.lmDimmerChecksum = 0x00;
    Context.mflButtonStatus = HANDLER_MFL_STATUS_OFF;
    Context.telStatus = IBUS_TEL_STATUS_NONE;
    Context.telOnStatus = HANDLER_TEL_OFF;
    Context.btBootState = HANDLER_BT_BOOT_OK;
    memset(&Context.gmState, 0, sizeof(HandlerBodyModuleStatus_t));
    memset(&Context.lmState, 0, sizeof(HandlerLightControlStatus_t));
    Context.powerStatus = HANDLER_POWER_ON;
    Context.scanIntervals = 0;
    Context.lmLastIOStatus = 0;
    Context.cdChangerLastPoll = now;
    Context.cdChangerLastStatus = now;
    Context.pdcLastStatus = 0;
    Context.pdcActive = 0;
    Context.lmLastStatusSet = 0;
    Context.radLastMessage = TimerGetMillis();
    EventRegisterCallback(
        UI_EVENT_CLOSE_CONNECTION,
        &HandlerUICloseConnection,
        &Context
    );
    EventRegisterCallback(
        UI_EVENT_INITIATE_CONNECTION,
        &HandlerUIInitiateConnection,
        &Context
    );
    TimerRegisterScheduledTask(
        &HandlerTimerPoweroff,
        &Context,
        HANDLER_INT_POWEROFF
    );
    HandlerBTInit(&Context);
    HandlerIBusInit(&Context);
    if (Context.uiMode == CONFIG_UI_CD53 ||
        Context.uiMode == CONFIG_UI_MIR ||
        Context.uiMode == CONFIG_UI_IRIS
    ) {
        CD53Init(bt, ibus);
    } else if (Context.uiMode == CONFIG_UI_BMBT) {
        BMBTInit(bt, ibus);
    } else if (Context.uiMode == CONFIG_UI_MID) {
        MIDInit(bt, ibus);
    } else if (Context.uiMode == CONFIG_UI_MID_BMBT) {
        MIDInit(bt, ibus);
        BMBTInit(bt, ibus);
    }
}

/**
 * HandlerUICloseConnection()
 *     Description:
 *         Close the active connection and dissociate ourselves from it
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerUICloseConnection(void *ctx, unsigned char *data)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    context->btSelectedDevice = HANDLER_BT_SELECTED_DEVICE_NONE;
    // Reset the metadata so we don't display the wrong data
    BTClearMetadata(context->bt);
    // Clear the actively paired device
    BTClearActiveDevice(context->bt);
    // Close All connections
    BTCommandDisconnect(context->bt);
}

/**
 * HandlerUIInitiateConnection()
 *     Description:
 *         Handle the connection when a new device is selected in the UI
 *         Rather than doing any connecting here, we instead set the correct
 *         states so that the disconnection or scan event will
 *     Params:
 *         void *ctx - The context provided at registration
 *         unsigned char *tmp - Any event data
 *     Returns:
 *         void
 */
void HandlerUIInitiateConnection(void *ctx, unsigned char *deviceId)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    context->btStatus = HANDLER_BT_STATUS_CONNECTING;
    context->btSelectedDevice = (uint8_t) *deviceId;
    // Reset the metadata so we don't display the wrong data
    BTClearMetadata(context->bt);
    // Clear the actively paired device
    BTClearActiveDevice(context->bt);
    // Close All connections
    BTCommandDisconnect(context->bt);
}

/**
 * HandlerTimerPoweroff()
 *     Description:
 *         Track the time since the last I-Bus message and see if we need to
 *         power off.
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void HandlerTimerPoweroff(void *ctx)
{
    HandlerContext_t *context = (HandlerContext_t *) ctx;
    if (ConfigGetSetting(CONFIG_SETTING_AUTO_POWEROFF) == CONFIG_SETTING_ON) {
        uint32_t lastRx = TimerGetMillis() - context->ibus->rxLastStamp;
        if (lastRx >= HANDLER_POWER_TIMEOUT_MILLIS) {
            if (context->powerStatus == HANDLER_POWER_ON) {
                // Destroy the UART module for IBus
                UARTDestroy(IBUS_UART_MODULE);
                TimerDelayMicroseconds(500);
                context->powerStatus = HANDLER_POWER_OFF;
                // Disable the TH3122
                IBUS_EN = 0;
            } else {
                // Re-enable the TH3122 EN line so we can try pulling it,
                // and the regulator low again
                IBUS_EN = 1;
                context->powerStatus = HANDLER_POWER_ON;
            }
        }
    }
}
