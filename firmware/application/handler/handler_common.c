/*
 * File: handler_common.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Shared structs, defines and functions for the Handlers
 */
#include "handler_common.h"

/**
 * HandlerSetIBusTELStatus()
 *     Description:
 *         Send the TEL status to the vehicle
 *     Params:
 *         HandlerContext_t *context - The module context
 *         unsigned char sendFlag - Weather to update the status only if it is
 *             different, or force broadcasting it.
 *     Returns:
 *         uint8_t - Returns 1 if the status has changed, zero otherwise
 */
uint8_t HandlerSetIBusTELStatus(
    HandlerContext_t *context,
    unsigned char sendFlag
) {
    if (ConfigGetTelephonyFeaturesActive() == CONFIG_SETTING_ON) {
        unsigned char currentTelStatus = 0x00;
        if (context->bt->scoStatus == BT_CALL_SCO_OPEN) {
            currentTelStatus = IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE;
        } else {
            currentTelStatus = IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE;
        }
        if (context->telStatus != currentTelStatus ||
            sendFlag == HANDLER_TEL_STATUS_FORCE
        ) {
            context->telStatus = currentTelStatus;
            // Do not set the active call flag for these UIs to allow
            // the radio volume controls to remain active
            if (currentTelStatus == IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE &&
                (context->uiMode == CONFIG_UI_CD53 ||
                 context->uiMode == CONFIG_UI_BUSINESS_NAV)
            ) {
                return 1;
            }
            IBusCommandTELStatus(context->ibus, currentTelStatus);
            return 1;
        }
    }
    return 0;
}

/**
 * HandlerSetVolume()
 *     Description:
 *         Abstract function to raise and lower the A2DP volume
 *     Params:
 *         HandlerContext_t *context - The handler context
 *         uint8_t direction - Lower / Raise volume flag
 *     Returns:
 *         void
 */
void HandlerSetVolume(HandlerContext_t *context, uint8_t direction)
{
    uint8_t newVolume = 0;
    if (direction == HANDLER_VOLUME_DIRECTION_DOWN) {
        newVolume = context->bt->activeDevice.a2dpVolume / 2;
        context->volumeMode = HANDLER_VOLUME_MODE_LOWERED;
    } else {
        newVolume = context->bt->activeDevice.a2dpVolume * 2;
        context->volumeMode = HANDLER_VOLUME_MODE_NORMAL;
    }
    char hexVolString[3] = {0};
    snprintf(hexVolString, 3, "%X", newVolume / 8);
    BC127CommandVolume(
        context->bt,
        context->bt->activeDevice.a2dpId,
        hexVolString
    );
    context->bt->activeDevice.a2dpVolume = newVolume;
}
