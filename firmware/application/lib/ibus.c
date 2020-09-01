/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#include "ibus.h"

/**
 * IBusInit()
 *     Description:
 *         Returns a fresh IBus_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         IBus_t*
 */
IBus_t IBusInit()
{
    IBus_t ibus;
    ibus.uart = UARTInit(
        IBUS_UART_MODULE,
        IBUS_UART_RX_RPIN,
        IBUS_UART_TX_RPIN,
        IBUS_UART_RX_PRIORITY,
        IBUS_UART_TX_PRIORITY,
        UART_BAUD_9600,
        UART_PARITY_EVEN
    );
    ibus.cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
    ibus.ignitionStatus = IBUS_IGNITION_OFF;
    ibus.gtVersion = ConfigGetNavType();
    ibus.vehicleType = ConfigGetVehicleType();
    ibus.lmVariant = ConfigGetLMVariant();
    ibus.lmLoadFrontVoltage = 0x00; // Front load sensor voltage (LWR)
    ibus.lmDimmerVoltage = 0xFF;
    ibus.lmLoadRearVoltage = 0x00; // Rear load sensor voltage (LWR)
    ibus.lmPhotoVoltage = 0xFF; // Photosensor voltage (LSZ)
    ibus.oilTemperature = 0x00;
    ibus.coolantTemperature = 0x00;
    ibus.rxBufferIdx = 0;
    ibus.rxLastStamp = 0;
    ibus.txBufferReadIdx = 0;
    ibus.txBufferReadbackIdx = 0;
    ibus.txBufferWriteIdx = 0;
    ibus.txLastStamp = TimerGetMillis();
    return ibus;
}

/**
 * IBusHandleBMBTMessage()
 *     Description:
 *         Handle any messages received from the BMBT (Board Monitor)
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleBMBTMessage(unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON0 ||
        pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON1
    ) {
        EventTriggerCallback(IBusEvent_BMBTButton, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_VOL_CTRL) {
        EventTriggerCallback(IBusEvent_RADVolumeChange, pkt);
    }
}

/**
 * IBusHandleDSPMessage()
 *     Description:
 *         Handle any messages received from the DSP Amplifier
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleDSPMessage(unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    }
}

/**
 * IBusHandleEWSMessage()
 *     Description:
 *         Handle any messages received from the EWS
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleEWSMessage(unsigned char *pkt)
{
    // Do nothing for now -- for future use
}

/**
 * IBusHandleIKEMessage()
 *     Description:
 *         Handle any messages received from the IKE (Instrument Cluster)
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleGMMessage(unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GM_DOORS_FLAPS_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_DoorsFlapsStatusResponse, pkt);
    //} else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_IDENT_RESP) {
    //    unsigned char diagnosticIdx = pkt[9];
    //    unsigned char moduleVariant = 0x00;
    //
    //    if (diagnosticIdx < 0x20) {
    //        moduleVariant = IBUS_GM_ZKE4;
    //    }
    //    switch (diagnosticIdx) {
    //        case 0x20:
    //        case 0x21:
    //        case 0x22:
    //            moduleVariant = IBUS_GM_ZKE3_GM1;
    //            break;
    //        case 0x25:
    //            moduleVariant = IBUS_GM_ZKE3_GM5;
    //            break;
    //        case 0x40:
    //        case 0x50:
    //        case 0x41:
    //        case 0x51:
    //        case 0x42:
    //        case 0x52:
    //            moduleVariant = IBUS_GM_ZKE5;
    //            break;
    //        case 0x45:
    //        case 0x55:
    //        case 0x46:
    //        case 0x56:
    //            moduleVariant = IBUS_GM_ZKE5_S12;
    //            break;
    //        case 0x80:
    //        case 0x81:
    //            moduleVariant = IBUS_GM_ZKE3_GM4;
    //            break;
    //        case 0x85:
    //            moduleVariant = IBUS_GM_ZKE3_GM6;
    //            break;
    //        case 0xA0:
    //            moduleVariant = IBUS_GM_ZKEBC1;
    //            break;
    //        case 0xA3:
    //            moduleVariant = IBUS_GM_ZKEBC1RD;
    //            break;
    //    }
    //    // Emit event
    }
}

/**
 * IBusHandleGTMessage()
 *     Description:
 *         Handle any messages received from the GT (Graphics Terminal)
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleGTMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    } else if (pkt[IBUS_PKT_LEN] == 0x22 &&
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE
    ) {
        // Decode the software and hardware versions
        uint8_t hardwareVersion = IBusGetNavHWVersion(pkt);
        uint8_t softwareVersion = IBusGetNavSWVersion(pkt);
        uint8_t diagnosticIndex = IBusGetNavDiagnosticIndex(pkt);
        uint8_t gtVersion = IBusGetNavType(pkt);
        if (gtVersion != IBUS_GT_DETECT_ERROR) {
            LogRaw(
                "\r\nIBus: GT P/N: %c%c%c%c%c%c%c DI: %d HW: %d SW: %d Build: %c%c/%c%c\r\n",
                pkt[4],
                pkt[5],
                pkt[6],
                pkt[7],
                pkt[8],
                pkt[9],
                pkt[10],
                diagnosticIndex,
                hardwareVersion,
                softwareVersion,
                pkt[19],
                pkt[20],
                pkt[21],
                pkt[22]
            );
            ibus->gtVersion = gtVersion;
            EventTriggerCallback(IBusEvent_GTDIAIdentityResponse, &gtVersion);
        } else {
            LogError("IBus: Unable to decode navigation type");
        }
    } else if (pkt[IBUS_PKT_LEN] >= 0x0C &&
        pkt[IBUS_PKT_LEN] < 0x22 &&
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE
    ) {
        // Example Frame: 3B 0C 3F A0 42 4D 57 43 30 31 53 00 00 E1
        EventTriggerCallback(IBusEvent_GTDIAOSIdentityResponse, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_MENU_SELECT) {
        EventTriggerCallback(IBusEvent_GTMenuSelect, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_SCREEN_MODE_SET) {
        EventTriggerCallback(IBusEvent_ScreenModeSet, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_CHANGE_UI_REQ) {
        // Example Frame: 3B 05 FF 20 02 0C EF [Telephone Selected]
        EventTriggerCallback(IBusEvent_GTChangeUIRequest, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON1) {
        // The GT broadcasts an emulated version of the BMBT button press
        // command 0x48 that matches the "Phone" button on the BMBT
        EventTriggerCallback(IBusEvent_BMBTButton, pkt);
    }
}

/**
 * IBusHandleIKEMessage()
 *     Description:
 *         Handle any messages received from the IKE (Instrument Cluster)
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleIKEMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_IGN_STATUS_RESP) {
        uint8_t ignitionStatus = pkt[4];
        if (ignitionStatus == IBUS_IGNITION_OFF) {
            // Implied that the CDC should not be playing with the ignition off
            ibus->cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
        }
        // The order of the items below should not be changed,
        // otherwise listeners will not know if the ignition status
        // has changed
        EventTriggerCallback(
            IBusEvent_IKEIgnitionStatus,
            &ignitionStatus
        );
        ibus->ignitionStatus = ignitionStatus;
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_RESP_VEHICLE_TYPE) {
        ibus->vehicleType = IBusGetVehicleType(pkt);
        EventTriggerCallback(IBusEvent_IKEVehicleType, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_SPEED_RPM_UPDATE) {
        EventTriggerCallback(IBusEvent_IKESpeedRPMUpdate, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IKE_COOLANT_TEMP_UPDATE) {
        // Do not update the system if the value is the same
        if (ibus->coolantTemperature != pkt[5] && pkt[5] < 0x80) {
            ibus->coolantTemperature = pkt[5];
            EventTriggerCallback(IBusEvent_IKECoolantTempUpdate, pkt);
        }
    }
}

/**
 * IBusHandleLCMMessage()
 *     Description:
 *         Handle any messages received from the LCM (Lighting Control Module)
 *     Params:
 *         unsigned char *pkt - The frame received on the IBus
 *     Returns:
 *         None
 */
static void IBusHandleLCMMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GLO &&
        pkt[IBUS_PKT_CMD] == IBUS_LCM_LIGHT_STATUS
    ) {
        EventTriggerCallback(IBusEvent_LCMLightStatus, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GLO &&
               pkt[IBUS_PKT_CMD] == IBUS_LCM_DIMMER_STATUS
    ) {
        EventTriggerCallback(IBusEvent_LCMDimmerStatus, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x19
    ) {
        // LME38 has unique status. It's shorter, and different mapping.
        // Length is an (educated) guess based on number of bytes required to
        // populate the job results.
        ibus->lmDimmerVoltage = pkt[IBUS_LME38_IO_DIMMER_OFFSET];
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x23
    ) {
        // Status reply length and mapping is the same for LCM and LSZ variants.
        // The non-applicable parameters default to 0x00, i.e. LCM does not
        // have a photosensor, so value will be 0x00.

        // Front load sensor voltage (Xenon)
        ibus->lmLoadFrontVoltage = pkt[IBUS_LM_IO_LOAD_FRONT_OFFSET];
        // Dimmer (58G) voltage
        ibus->lmDimmerVoltage = pkt[IBUS_LM_IO_DIMMER_OFFSET];
        // Rear load sensor voltage (Xenon) / manual vertical aim control (non-Xenon)
        ibus->lmLoadRearVoltage = pkt[IBUS_LM_IO_LOAD_REAR_OFFSET];
        // Photosensor voltage (LSZ)
        ibus->lmPhotoVoltage = pkt[IBUS_LM_IO_PHOTO_OFFSET];
        if (ibus->vehicleType != IBUS_VEHICLE_TYPE_E46_Z4 &&
            pkt[23] != 0x00
        ) {
            // Oil Temp calculation
            float rawTemperature = (pkt[23] * 0.01275) + (pkt[24] * 0.000050);
            unsigned char oilTemperature = 1.0 * 67.2529 * log(rawTemperature) + 310.0;
            if (oilTemperature != ibus->oilTemperature) {
                ibus->oilTemperature = oilTemperature;
                unsigned char updateType = 0x01;
                EventTriggerCallback(
                    IBusEvent_ValueUpdate,
                    &updateType
                );
            }
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x03
    ) {
        EventTriggerCallback(IBusEvent_LCMDiagnosticsAcknowledge, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_LCM_RESP_REDUNDANT_DATA) {
        EventTriggerCallback(IBusEvent_LCMRedundantData, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE && // 0xa0
               pkt[IBUS_PKT_LEN] == 0x0F
    ) {
      // I was a bit nervous about relying upon message length, but only an
      // ident request (0x00) results in a reply of this length. Winning.
      // I would have done the same GT and had the ident logic in the handler,
      // but it's a bit unwieldy so I split it out.
      uint8_t lmVariant = IBusGetLMVariant(pkt);
      // I did not modify struct in above method as it was not very _SOLID_.
      // Yes, that's right, SOLID. Don't you roll your eyes at me!
      ibus->lmVariant = lmVariant;
      EventTriggerCallback(IBusEvent_LMIdentResponse, &lmVariant);
    }
}

static void IBusHandleMFLMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_MFL_BTN_EVENT ||
        (pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL && pkt[IBUS_PKT_CMD] == 0x01)
    ) {
        EventTriggerCallback(IBusEvent_MFLButton, pkt);
    }
    if (pkt[IBUS_PKT_CMD] == IBUS_MFL_BTN_VOL) {
        EventTriggerCallback(IBusEvent_MFLVolume, pkt);
    }
}

static void IBusHandleMIDMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_RAD) {
        if (pkt[IBUS_PKT_CMD] == IBus_MID_Button_Press) {
            EventTriggerCallback(IBusEvent_MIDButtonPress, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_LOC) {
        if (pkt[IBUS_PKT_CMD] == IBus_MID_CMD_MODE) {
            EventTriggerCallback(IBusEvent_MIDModeChange, pkt);
        }
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_VOL_CTRL) {
        EventTriggerCallback(IBusEvent_RADVolumeChange, pkt);
    }
}

static void IBusHandleRADMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_RESP) {
        EventTriggerCallback(IBusEvent_ModuleStatusResponse, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_CDC) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_REQ) {
            EventTriggerCallback(IBusEvent_ModuleStatusRequest, pkt);
        } else if(pkt[IBUS_PKT_CMD] == IBUS_COMMAND_CDC_GET_STATUS) {
            if (pkt[4] == IBUS_CDC_CMD_STOP_PLAYING) {
                ibus->cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
            } else if (pkt[4] == IBUS_CDC_CMD_PAUSE_PLAYING) {
                ibus->cdChangerFunction = IBUS_CDC_FUNC_PAUSE;
            } else if (pkt[4] == IBUS_CDC_CMD_START_PLAYING) {
                ibus->cdChangerFunction = IBUS_CDC_FUNC_PLAYING;
            }
            EventTriggerCallback(IBusEvent_CDStatusRequest, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_LEN] > 8 &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIA_DIAG_RESPONSE
    ) {
        LogRaw(
            "\r\nIBus: RAD P/N: %d%d%d%d%d%d%d HW: %02d SW: %d%d Build: %d%d/%d%d\r\n",
            pkt[4] & 0x0F,
            (pkt[5] & 0xF0) >> 4,
            pkt[5] & 0x0F,
            (pkt[6] & 0xF0) >> 4,
            pkt[6] & 0x0F,
            (pkt[7] & 0xF0) >> 4,
            pkt[7] & 0x0F,
            pkt[8],
            (pkt[15] & 0xF0) >> 4,
            pkt[15] & 0x0F,
            (pkt[12] & 0xF0) >> 4,
            pkt[12] & 0x0F,
            (pkt[13] & 0xF0) >> 4,
            pkt[13] & 0x0F
        );
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DSP) {
        if (pkt[IBUS_PKT_CMD] == IBUS_DSP_CMD_CONFIG_SET) {
            EventTriggerCallback(IBusEvent_DSPConfigSet, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GT) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_SCREEN_MODE_UPDATE) {
            EventTriggerCallback(IBusEvent_ScreenModeUpdate, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_UPDATE_MAIN_AREA) {
            EventTriggerCallback(IBusEvent_RADUpdateMainArea, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_DISPLAY_RADIO_MENU) {
            EventTriggerCallback(IBusEvent_RADDisplayMenu, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_LOC) {
        if (pkt[IBUS_PKT_CMD] == 0x3B) {
            EventTriggerCallback(IBusEvent_CDClearDisplay, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_UPDATE_MAIN_AREA) {
            EventTriggerCallback(IBusEvent_RADUpdateMainArea, pkt);
        }
        if (pkt[IBUS_PKT_CMD] == IBUS_DSP_CMD_CONFIG_SET) {
            EventTriggerCallback(IBusEvent_DSPConfigSet, pkt);
        }
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_MID) {
        if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_WRITE_MID_DISPLAY) {
            if (pkt[4] == 0xC0) {
                EventTriggerCallback(IBusEvent_RADMIDDisplayText, pkt);
            }
        } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_RAD_WRITE_MID_MENU) {
            EventTriggerCallback(IBusEvent_RADMIDDisplayMenu, pkt);
        }
    }
}

static void IBusHandleTELMessage(unsigned char *pkt)
{
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_MOD_STATUS_REQ) {
        EventTriggerCallback(IBusEvent_ModuleStatusRequest, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_VOL_CTRL) {
        EventTriggerCallback(IBusEvent_TELVolumeChange, pkt);
    }
}

static uint8_t IBusValidateChecksum(unsigned char *msg)
{
    uint8_t chk = 0;
    uint8_t msgSize = msg[1] + 2;
    uint8_t idx;
    for (idx = 0; idx < msgSize; idx++) {
        chk =  chk ^ msg[idx];
    }
    if (chk == 0) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * IBusProcess()
 *     Description:
 *         Process messages in the IBus RX queue
 *     Params:
 *         IBus_t *ibus
 *     Returns:
 *         void
 */
void IBusProcess(IBus_t *ibus)
{
    // Read messages from the IBus and if none are available, attempt to
    // transmit whatever is sitting in the transmit buffer
    if (ibus->uart.rxQueue.size > 0) {
        ibus->rxBuffer[ibus->rxBufferIdx++] = CharQueueNext(&ibus->uart.rxQueue);
        if (ibus->rxBufferIdx > 1) {
            uint8_t msgLength = (uint8_t) ibus->rxBuffer[1] + 2;
            // Make sure we do not read more than the maximum packet length
            if (msgLength > IBUS_MAX_MSG_LENGTH) {
                long long unsigned int ts = (long long unsigned int) TimerGetMillis();
                LogRawDebug(
                    LOG_SOURCE_IBUS,
                    "[%llu] ERROR: IBus: RX Invalid Length [%d - %02X]: ",
                    ts,
                    msgLength,
                    ibus->rxBuffer[1]
                );
                uint8_t idx;
                for (idx = 0; idx < ibus->rxBufferIdx; idx++) {
                    LogRawDebug(LOG_SOURCE_IBUS, "%02X ", ibus->rxBuffer[idx]);
                }
                LogRawDebug(LOG_SOURCE_IBUS, "\r\n");
                ibus->rxBufferIdx = 0;
                memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
                CharQueueReset(&ibus->uart.rxQueue);
            } else if (msgLength == ibus->rxBufferIdx) {
                uint8_t idx;
                unsigned char pkt[msgLength];
                long long unsigned int ts = (long long unsigned int) TimerGetMillis();
                LogRawDebug(LOG_SOURCE_IBUS, "[%llu] DEBUG: IBus: RX[%d]: ", ts, msgLength);
                for(idx = 0; idx < msgLength; idx++) {
                    pkt[idx] = ibus->rxBuffer[idx];
                    LogRawDebug(LOG_SOURCE_IBUS, "%02X ", pkt[idx]);
                }
                if (memcmp(ibus->txBuffer[ibus->txBufferReadbackIdx], pkt, msgLength) == 0) {
                    LogRawDebug(LOG_SOURCE_IBUS, "[SELF]");
                    memset(ibus->txBuffer[ibus->txBufferReadbackIdx], 0, msgLength);
                    if (ibus->txBufferReadbackIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                        ibus->txBufferReadbackIdx = 0;
                    } else {
                        ibus->txBufferReadbackIdx++;
                    }
                }
                LogRawDebug(LOG_SOURCE_IBUS, "\r\n");
                if (IBusValidateChecksum(pkt) == 1) {
                    unsigned char srcSystem = pkt[IBUS_PKT_SRC];
                    if (srcSystem == IBUS_DEVICE_RAD) {
                        IBusHandleRADMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_BMBT) {
                        IBusHandleBMBTMessage(pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_IKE) {
                        IBusHandleIKEMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_GT) {
                        IBusHandleGTMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_LCM) {
                        IBusHandleLCMMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_MID) {
                        IBusHandleMIDMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_MFL) {
                        IBusHandleMFLMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_DSP) {
                        IBusHandleDSPMessage(pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_GM) {
                        IBusHandleGMMessage(pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_EWS) {
                        IBusHandleEWSMessage(pkt);
                    }
                    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_TEL) {
                        IBusHandleTELMessage(pkt);
                    }
                } else {
                    LogError(
                        "IBus: %02X -> %02X Length: %d - Invalid Checksum",
                        pkt[0],
                        pkt[IBUS_PKT_DST],
                        msgLength,
                        (uint8_t) pkt[IBUS_PKT_LEN]
                    );
                }
                memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
                ibus->rxBufferIdx = 0;
            }
        }
        if (ibus->rxLastStamp == 0) {
            EventTriggerCallback(IBusEvent_FirstMessageReceived, 0);
        }
        ibus->rxLastStamp = TimerGetMillis();
    } else if (ibus->txBufferWriteIdx != ibus->txBufferReadIdx) {
        // Flush the transmit buffer out to the bus
        uint8_t txTimeout = IBUS_TX_TIMEOUT_OFF;
        uint8_t beginTxTimestamp = TimerGetMillis();
        while (ibus->txBufferWriteIdx != ibus->txBufferReadIdx &&
               txTimeout != IBUS_TX_TIMEOUT_ON
        ) {
            uint32_t now = TimerGetMillis();
            if ((now - ibus->txLastStamp) >= IBUS_TX_BUFFER_WAIT) {
                uint8_t msgLen = (uint8_t) ibus->txBuffer[ibus->txBufferReadIdx][1] + 2;
                uint8_t idx;
                /*
                 * Make sure that the STATUS pin on the TH3122 is low, indicating no
                 * bus activity before transmitting
                 */
                if (IBUS_UART_STATUS == 0) {
                    for (idx = 0; idx < msgLen; idx++) {
                        ibus->uart.registers->uxtxreg = ibus->txBuffer[ibus->txBufferReadIdx][idx];
                        // Wait for the data to leave the TX buffer
                        while ((ibus->uart.registers->uxsta & (1 << 9)) != 0);
                    }
                    txTimeout = IBUS_TX_TIMEOUT_DATA_SENT;
                    if (ibus->txBufferReadIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                        ibus->txBufferReadIdx = 0;
                    } else {
                        ibus->txBufferReadIdx++;
                    }
                    ibus->txLastStamp = TimerGetMillis();
                } else if (txTimeout != IBUS_TX_TIMEOUT_DATA_SENT) {
                    if ((now - beginTxTimestamp) > IBUS_TX_TIMEOUT_WAIT) {
                        txTimeout = IBUS_TX_TIMEOUT_ON;
                    }
                }
            }
        }
    }

    // Clear the RX Buffer if it's over the timeout or about to overflow
    if (ibus->rxBufferIdx > 0) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->rxLastStamp) > IBUS_RX_BUFFER_TIMEOUT ||
            (ibus->rxBufferIdx + 1) == IBUS_RX_BUFFER_SIZE
        ) {
            long long unsigned int ts = (long long unsigned int) TimerGetMillis();
            LogRawDebug(
                LOG_SOURCE_IBUS,
                "[%llu] ERROR: IBus: RX Buffer Timeout [%d]: ",
                ts,
                ibus->rxBufferIdx
            );
            uint8_t idx;
            for (idx = 0; idx < ibus->rxBufferIdx; idx++) {
                LogRawDebug(LOG_SOURCE_IBUS, "%02X ", ibus->rxBuffer[idx]);
            }
            LogRawDebug(LOG_SOURCE_IBUS, "\r\n");
            ibus->rxBufferIdx = 0;
            memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
        }
    }
    UARTReportErrors(&ibus->uart);
}

/**
 * IBusSendCommand()
 *     Description:
 *         Take a Destination, source and message and add it to the transmit
 *         char queue so we can send it later.
 *     Params:
 *         IBus_t *ibus,
 *         const unsigned char src,
 *         const unsigned char dst,
 *         const unsigned char *data
 *     Returns:
 *         void
 */
void IBusSendCommand(
    IBus_t *ibus,
    const unsigned char src,
    const unsigned char dst,
    const unsigned char *data,
    const size_t dataSize
) {
    uint8_t idx, msgSize;
    msgSize = dataSize + 4;
    unsigned char msg[msgSize];
    msg[0] = src;
    msg[1] = dataSize + 2;
    msg[2] = dst;
    idx = 3;
    uint8_t i;
    // Add the Data to the packet
    for (i = 0; i < dataSize; i++) {
        msg[idx++] = data[i];
    }
    // Calculate the CRC
    uint8_t crc = 0;
    uint8_t maxIdx = msgSize - 1;
    for (idx = 0; idx < maxIdx; idx++) {
        crc ^= msg[idx];
    }
    msg[msgSize - 1] = (unsigned char) crc;
    for (idx = 0; idx < msgSize; idx++) {
        ibus->txBuffer[ibus->txBufferWriteIdx][idx] = msg[idx];
    }
    /* Store the data into a buffer, so we can spread out their transmission */
    if (ibus->txBufferWriteIdx + 1 == IBUS_TX_BUFFER_SIZE) {
        ibus->txBufferWriteIdx = 0;
    } else {
        ibus->txBufferWriteIdx++;
    }
}

/***
 * IBusGetLMCodingIndex()
 *     Description:
 *        Get the light module coding index
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the light module coding index
 */
uint8_t IBusGetLMCodingIndex(unsigned char *packet)
{
    uint8_t codingIndex = {
        packet[IBUS_LM_CI_ID_OFFSET]
    };
    return codingIndex;
}

/***
 * IBusGetLMDiagnosticIndex()
 *     Description:
 *        Get the light module diagnostic index
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the light module diagnostic index
 */
uint8_t IBusGetLMDiagnosticIndex(unsigned char *packet)
{
    uint8_t diagnosticIndex = {
        packet[IBUS_LM_DI_ID_OFFSET]
    };
    return diagnosticIndex;
}

/***
 * IBusGetLMDimmerChecksum()
 *     Description:
 *        Get the dimmer checksum
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - the light module coding index
 */
uint8_t IBusGetLMDimmerChecksum(unsigned char *packet)
{
    uint8_t frameLength = packet[1] - 1;
    uint8_t index = 4;
    uint8_t checksum = 0x00;
    while (frameLength > 0) {
        checksum ^= packet[index];
        index++;
        frameLength--;
    }
    return checksum;
}


uint8_t IBusGetLMDimmerChecksum(unsigned char *);

/**
 * IBusGetLMVariant()
 *     Description:
 *        Get the light module variant, as per EDIABAS:
 *        Group file: D_00D0.GRP
*         Version:    1.5.1
 *     Params:
 *         unsigned char *packet - Diagnostics ident packet
 *     Returns:
 *         uint8_t - The light module variant
 */
uint8_t IBusGetLMVariant(unsigned char *packet)
{
    uint8_t diagnosticIndex = IBusGetLMDiagnosticIndex(packet);
    uint8_t codingIndex = IBusGetLMCodingIndex(packet);
    uint8_t lmVariant = 0;

    LogRaw("\r\nIBus: LM DI: %d CI: %d\r\n", diagnosticIndex, codingIndex);

    if (diagnosticIndex < 0x10) {
        lmVariant = IBUS_LM_LME38;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LME38");
    } else if (diagnosticIndex == 0x10) {
        lmVariant = IBUS_LM_LCM;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM");
    } else if (diagnosticIndex == 0x11) {
        lmVariant = IBUS_LM_LCM_A;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_A");
    } else if (diagnosticIndex == 0x12 && codingIndex == 0x16) {
        lmVariant = IBUS_LM_LCM_II;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_II");
    } else if ((diagnosticIndex == 0x12 || diagnosticIndex == 0x13) &&
              codingIndex == 0x17
    ) {
        lmVariant = IBUS_LM_LCM_III;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_III");
    } else if (diagnosticIndex == 0x14) {
        lmVariant = IBUS_LM_LCM_IV;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LCM_IV");
    } else if (diagnosticIndex >= 0x20 && diagnosticIndex <= 0x2f) {
        lmVariant = IBUS_LM_LSZ;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LSZ");
    } else if (diagnosticIndex == 0x30) {
        lmVariant = IBUS_LM_LSZ_2;
        LogInfo(LOG_SOURCE_IBUS, "Light Module: LSZ_2");
    }

    return lmVariant;
}

/**
 * IBusGetNavDiagnosticIndex()
 *     Description:
 *        Get the nav diagnostic index
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav hardware version
 */
uint8_t IBusGetNavDiagnosticIndex(unsigned char *packet)
{
    char diVersion[3] = {
        packet[IBUS_GT_DI_ID_OFFSET],
        packet[IBUS_GT_DI_ID_OFFSET + 1],
        '\0'
    };
    return UtilsStrToInt(diVersion);
}

/**
 * IBusGetNavHWVersion()
 *     Description:
 *        Get the nav type hardware version
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav hardware version
 */
uint8_t IBusGetNavHWVersion(unsigned char *packet)
{
    char hwVersion[3] = {
        packet[IBUS_GT_HW_ID_OFFSET],
        packet[IBUS_GT_HW_ID_OFFSET + 1],
        '\0'
    };
    return UtilsStrToInt(hwVersion);
}

/**
 * IBusGetNavSWVersion()
 *     Description:
 *        Get the nav type software version
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav software version
 */
uint8_t IBusGetNavSWVersion(unsigned char *packet)
{
    char swVersion[3] = {
        (char) packet[IBUS_GT_SW_ID_OFFSET],
        (char) packet[IBUS_GT_SW_ID_OFFSET + 1],
        '\0'
    };
    return UtilsStrToInt(swVersion);
}

/**
 * IBusGetNavType()
 *     Description:
 *        Get the nav type based on the hardware and software versions
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav type
 */
uint8_t IBusGetNavType(unsigned char *packet)
{
    uint8_t diagnosticIndex = IBusGetNavDiagnosticIndex(packet);
    uint8_t navType = 0;
    switch (diagnosticIndex) {
        case 1:
            navType = IBUS_GT_MKI;
            break;
        case 2:
        case 3:
            navType = IBUS_GT_MKII;
            break;
        case 4:
            navType = IBUS_GT_MKIII;
            break;
        case 5:
        case 6:
            navType = IBUS_GT_MKIV;
            break;
        default:
            navType = IBUS_GT_DETECT_ERROR;
            break;
    }
    uint8_t softwareVersion = IBusGetNavSWVersion(packet);
    if (navType == IBUS_GT_MKIII && softwareVersion >= 40) {
        navType = IBUS_GT_MKIII_NEW_UI;
    }
    if (navType == IBUS_GT_MKIV &&
        (softwareVersion == 0 || softwareVersion >= 40)
    ) {
        navType = IBUS_GT_MKIV_STATIC;
    }
    return navType;
}

/**
 * IBusGetVehicleType()
 *     Description:
 *        Get the vehicle type from the cluster type response
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav type
 */
uint8_t IBusGetVehicleType(unsigned char *packet)
{
    unsigned char vehicleType = (packet[4] >> 4) & 0xF;
    unsigned char detectedVehicleType = 0xFF;
    if (vehicleType == 0x0F || vehicleType == 0x0A) {
        detectedVehicleType = IBUS_VEHICLE_TYPE_E46_Z4;
    } else {
        // 0x00 and 0x02 are possibilities here
        detectedVehicleType = IBUS_VEHICLE_TYPE_E38_E39_E53;
    }
    return detectedVehicleType;
}

/**
 * IBusCommandCDCAnnounce()
 *     Description:
 *        Send the CDC Announcement Message so the radio knows we're here
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandCDCAnnounce(IBus_t *ibus)
{
    const unsigned char cdcAlive[] = {0x02, 0x01};
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_LOC, cdcAlive, sizeof(cdcAlive));
}

/**
 * IBusCommandCDCPollResponse()
 *     Description:
 *        Respond to the Radio's "Ping" with our "Pong"
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandCDCPollResponse(IBus_t *ibus)
{
    const unsigned char cdcPing[] = {0x02, 0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_RAD, cdcPing, sizeof(cdcPing));
}

/**
 * IBusCommandCDCStatus()
 *     Description:
 *        Respond to the Radio's status request
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char status - The current CDC status
 *         unsigned char function - The current CDC function
 *         unsigned char discCount - The number of discs to report loaded
 *     Returns:
 *         void
 */
void IBusCommandCDCStatus(
    IBus_t *ibus,
    unsigned char status,
    unsigned char function,
    unsigned char discCount
) {
    function = function + 0x80;
    const unsigned char cdcStatus[] = {
        IBUS_COMMAND_CDC_SET_STATUS,
        status,
        function,
        0x00, // Errors
        discCount,
        0x00,
        0x01,
        0x01,
        0x00,
        0x01,
        0x01,// Disc Number
        0x01 // Track Number
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_CDC,
        IBUS_DEVICE_RAD,
        cdcStatus,
        sizeof(cdcStatus)
    );
}

/**
 * IBusCommandDIAGetCodingData()
 *     Description:
 *        Request the given systems coding data
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetCodingData(
    IBus_t *ibus,
    unsigned char system,
    unsigned char addr,
    unsigned char offset
) {
    unsigned char msg[] = {0x08, 0x00, addr, offset};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 1);
}

/**
 * IBusCommandDIAGetIdentity()
 *     Description:
 *        Request the given systems identity info
 *        Raw: 3F LEN DST 00 CHK
 *        3F 00 3B 11 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetIdentity(IBus_t *ibus, unsigned char system)
{
    unsigned char msg[] = {0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 1);
}

/**
 * IBusCommandDIAGetIOStatus()
 *     Description:
 *        Request the IO Status of the given system
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetIOStatus(IBus_t *ibus, unsigned char system)
{
    unsigned char msg[] = {0x0B};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_DIA,
        system,
        msg,
        1
    );
}

/**
 * IBusCommandDIAGetOSIdentity()
 *     Description:
 *        Request the OS identity
 *        Raw: 3F LEN DST 11 CHK
 *        3F 00 3B 11 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIAGetOSIdentity(IBus_t *ibus, unsigned char system)
{
    unsigned char msg[] = {0x11};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, system, msg, 1);
}

/**
 * IBusCommandDIATerminateDiag()
 *     Description:
 *        Terminate any ongoing diagnostic request on the given system
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandDIATerminateDiag(IBus_t *ibus, unsigned char system)
{
    unsigned char msg[] = {0x9F};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_DIA,
        system,
        msg,
        1
    );
}

/**
 * IBusCommandDSPSetMode()
 *     Description:
 *        Set the DSP mode
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char mode - The mode to set the DSP to
 *     Returns:
 *         void
 */
void IBusCommandDSPSetMode(IBus_t *ibus, unsigned char mode)
{
    unsigned char msg[] = {IBUS_DSP_CMD_CONFIG_SET, mode};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_DSP,
        msg,
        2
    );
}

/**
 * IBusCommandGetModuleStatus()
 *     Description:
 *        Request a "pong" from a given module to see if it is present
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char source - The system to send the request from
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandGetModuleStatus(
    IBus_t *ibus,
    unsigned char source,
    unsigned char system
) {
    unsigned char msg[] = {0x01};
    IBusSendCommand(ibus, source, system, msg, 1);
}

/**
 * IBusCommandSetModuleStatus()
 *     Description:
 *        Request a "pong" from a given module to see if it is present
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char source - The system to send the request from
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandSetModuleStatus(
    IBus_t *ibus,
    unsigned char source,
    unsigned char system,
    unsigned char status
) {
    unsigned char msg[] = {IBUS_CMD_MOD_STATUS_RESP, status};
    IBusSendCommand(ibus, source, system, msg, 2);
}

/**
 * IBusCommandGMDoorCenterLockButton()
 *     Description:
 *        Issue a diagnostic message to the GM to act as if the center
 *        lock button was pressed
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorCenterLockButton(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_CENTRAL_LOCK, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_CENTRAL_LOCK, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorUnlockHigh()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock the high side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorUnlockHigh(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_UNLOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_UNLOCK_HIGH, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorUnlockLow()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock the low side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorUnlockLow(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_UNLOCK_LOW, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_UNLOCK_LOW, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorLockHigh()
 *     Description:
 *        Issue a diagnostic message to the GM to lock the high side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorLockHigh(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_LOCK_HIGH, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorLockLow()
 *     Description:
 *        Issue a diagnostic message to the GM to lock the low side doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorLockLow(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_LOCK_LOW, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorUnlockAll()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock all doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorUnlockAll(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_UNLOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        // Central unlock unlocks all doors on the ZKE3
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_CENTRAL_LOCK, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

/**
 * IBusCommandGMDoorLockAll()
 *     Description:
 *        Issue a diagnostic message to the GM to lock all doors
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMDoorLockAll(IBus_t *ibus)
{
    if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            IBUS_CMD_ZKE5_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    } else if (ibus->vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // Sub-Module
            IBUS_CMD_ZKE3_GM4_JOB_LOCK_ALL, // Job
            0x01 // On / Off
        };
        IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
    }
}

void IBusCommandGTUpdate(IBus_t *ibus, unsigned char updateType)
{
    unsigned char msg[4] = {
        IBUS_CMD_GT_WRITE_WITH_CURSOR,
        updateType,
        0x01,
        0x00
    };
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, msg, 4);
}

static void IBusInternalCommandGTWriteIndex(
    IBus_t *ibus,
    uint8_t index,
    char *message,
    unsigned char indexMode
) {
    uint8_t length = strlen(message);
    // @TODO: This is 14 for the older UI. Come up with a better solution
    if (length > 23) {
        length = 23;
    }
    const size_t pktLenght = length + 5;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_NO_CURSOR;
    text[1] = indexMode;
    text[2] = 0x00;
    text[3] = 0x40 + (unsigned char) index;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    text[pktLenght - 1] = 0x06;
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

static void IBusCommandGTWriteIndexStaticInternal(
    IBus_t *ibus,
    uint8_t index,
    char *message,
    uint8_t cursorPos
) {
    uint8_t length = strlen(message);
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_WITH_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_STATIC;
    text[2] = cursorPos;
    text[3] = index;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandGTWriteBusinessNavTitle()
 *     Description:
 *        Write the single line available to write to on the Business Nav system
 *        It supports a maximum of 11 characters
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandGTWriteBusinessNavTitle(IBus_t *ibus, char *message) {
    uint8_t length = strlen(message);
    if (length > 11) {
        length = 11;
    }
    const size_t pktLenght = length + 3;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_TITLE;
    text[1] = 0x40;
    text[2] = 0x30;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 3] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteIndex(
    IBus_t *ibus,
    uint8_t index,
    char *message
) {
    IBusInternalCommandGTWriteIndex(
        ibus,
        index,
        message,
        IBUS_CMD_GT_WRITE_INDEX
    );
}

void IBusCommandGTWriteIndexTMC(
    IBus_t *ibus,
    uint8_t index,
    char *message
) {
    IBusInternalCommandGTWriteIndex(
        ibus,
        index,
        message,
        IBUS_CMD_GT_WRITE_INDEX_TMC
    );
}

/**
 * IBusCommandGTWriteIndexTitle()
 *     Description:
 *        Write the TMC title "Strip"
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteIndexTitle(IBus_t *ibus, char *message) {
    uint8_t length = strlen(message);
    if (length > 20) {
        length = 20;
    }
    const size_t pktLenght = length + 6;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_NO_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_INDEX_TMC;
    text[2] = 0x00; // Cursor at 0
    text[3] = 0x09; // Write menu title index
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    text[pktLenght - 2] = 0x06;
    text[pktLenght - 1] = 0x06;
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteIndexStatic(IBus_t *ibus, uint8_t index, char *message)
{
    uint8_t length = strlen(message);
    if (length > 40) {
        length = 40;
    }
    uint8_t cursorPos = 0;
    uint8_t currentIdx = 0;
    while (currentIdx < length) {
        uint8_t textLength = length - currentIdx;
        if (textLength > 0x14) {
            textLength = 0x14;
        }
        char msg[textLength + 1];
        memset(msg, '\0', sizeof(msg));
        uint8_t i;
        for (i = 0; i < textLength; i++) {
            msg[i] = message[currentIdx];
            currentIdx++;
        }
        if (cursorPos == 0) {
            IBusCommandGTWriteIndexStaticInternal(ibus, index, msg, 1);
        } else {
            IBusCommandGTWriteIndexStaticInternal(ibus, index, msg, cursorPos);
        }
        // Make sure we do not write over the
        // last character of the previous string
        cursorPos = cursorPos + textLength + 1;
    }
}

/**
 * IBusCommandGTWriteTitleArea()
 *     Description:
 *        Write the title using the "old" UI "Area" update message
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteTitleArea(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 9) {
        length = 9;
    }
    // Length + Write Type + Write Area + Size
    const size_t pktLenght = length + 3;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_TITLE;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x30;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 3] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandGTWriteTitleIndex().l
 *     Description:
 *        Write the title using the "new" UI "Index" update message.
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The text
 *     Returns:
 *         void
 */
void IBusCommandGTWriteTitleIndex(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 9) {
        length = 9;
    }
    // Length + Write Type + Write Area + Write Index + Size
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_NO_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x01; // Unused in this layout
    text[3] = 0x40; // Write Area 0 Index
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteTitleC43(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 11) {
        length = 11;
    }
    // Length + Write Type + Write Area + Size + Watermark
    const size_t pktLenght = length + 8;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_TITLE;
    text[1] = 0x40;
    text[2] = 0x20;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 3] = message[idx];
    }
    text[idx + 3] = 0x04;
    idx++;
    text[idx + 3] = 0x20;
    idx++;
    text[idx + 3] = 0x20;
    idx++;
    text[idx + 3] = 0x20;
    idx++;
    // "Watermark" Any update we send, so we know that it was us
    text[idx + 3] = IBUS_RAD_MAIN_AREA_WATERMARK;
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteZone(IBus_t *ibus, uint8_t index, char *message)
{
    uint8_t length = strlen(message);
    if (length > 11) {
        length = 11;
    }
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_WITH_CURSOR;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x01;
    text[3] = (unsigned char) index;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

/**
 * IBusCommandIKEGetIgnitionStatus()
 *     Description:
 *        Send the command to request the ignition status
 *        Raw: 18 03 80 10 8B
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKEGetIgnitionStatus(IBus_t *ibus)
{
    unsigned char msg[] = {IBUS_CMD_IKE_IGN_STATUS_REQ};
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_IKE, msg, 1);
}

/**
 * IBusCommandIKEGetVehicleType()
 *     Description:
 *        Request the vehicle type from the IKE
 *        Raw: 68 03 80 14 FF
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKEGetVehicleType(IBus_t *ibus)
{
    unsigned char msg[] = {IBUS_CMD_IKE_REQ_VEHICLE_TYPE};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_IKE,
        msg,
        1
    );
}

/**
 * IBusCommandIKESetTime()
 *     Description:
 *        Set the current time
 *        Raw: 3B 06 80 40 01 HH MM CS
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKESetTime(IBus_t *ibus, uint8_t hour, uint8_t minute)
{
    unsigned char msg[] = {
        IBUS_CMD_IKE_SET_REQUEST,
        IBUS_CMD_IKE_SET_REQUEST_TIME,
        hour,
        minute
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_IKE,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandIKEText()
 *     Description:
 *        Send text to the Business Radio
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandIKEText(IBus_t *ibus, char *message)
{
    unsigned char displayText[strlen(message) + 3];
    displayText[0] = 0x23;
    displayText[1] = 0x42;
    displayText[2] = 0x32;
    uint8_t idx;
    for (idx = 0; idx < strlen(message); idx++) {
        displayText[idx + 3] = message[idx];
    }
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_TEL,
        IBUS_DEVICE_IKE,
        displayText,
        sizeof(displayText)
    );
}

/**
 * IBusCommandIKETextClear()
 *     Description:
 *        Send an empty string to the Business Radio to clear the display
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKETextClear(IBus_t *ibus)
{
    IBusCommandIKEText(ibus, 0);
}

/**
 * IBusCommandLMActivateBulbs()
 *     Description:
 *        Light module diagnostics: Activate bulbs
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char blinkerSide - left or right blinker
 *     Returns:
 *         void
 */
void IBusCommandLMActivateBulbs(IBus_t *ibus, unsigned char blinkerSide) {
    unsigned char blinker = IBUS_LSZ_BLINKER_OFF;

    if (ibus->lmVariant == IBUS_LM_LME38) {
        switch (blinkerSide) {
            case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LME38_BLINKER_LEFT;
                break;
            case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LME38_BLINKER_RIGHT;
                break;
        }
        // No LWR load sensor/HVAC pot voltage for LME38
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            blinker,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            ibus->lmDimmerVoltage,
            0x00,
            0x00,
            0x00,
            0x00,
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    } else if (ibus->lmVariant == IBUS_LM_LCM ||
               ibus->lmVariant == IBUS_LM_LCM_A
    ) {
        switch (blinkerSide) {
            case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LCM_BLINKER_LEFT;
                break;
            case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LCM_BLINKER_RIGHT;
                break;
        }
        // This is an issue! I'm not sure what differentiates S1 and S2.
        // It's meaningful enough a distinction that it is displayed in INPA.
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00, // S2_BLK_L, S2_BLK_R
            blinker, // S1_BLK_L, S1_BLK_R
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            ibus->lmDimmerVoltage,
            ibus->lmLoadRearVoltage,
            0x00
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    } else if (ibus->lmVariant == IBUS_LM_LCM_II ||
               ibus->lmVariant == IBUS_LM_LCM_III ||
               ibus->lmVariant == IBUS_LM_LCM_IV
    ) {
        switch (blinkerSide) {
            case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LCM_II_BLINKER_LEFT;
                break;
            case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LCM_II_BLINKER_RIGHT;
                break;
        }
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00,
            0x00,
            blinker,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            ibus->lmDimmerVoltage,
            ibus->lmLoadRearVoltage,
            0x00
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    }
    else if (ibus->lmVariant == IBUS_LM_LSZ ||
             ibus->lmVariant == IBUS_LM_LSZ_2
    ) {
        switch (blinkerSide) {
          case IBUS_LM_BLINKER_LEFT:
                blinker = IBUS_LSZ_BLINKER_LEFT;
                break;
          case IBUS_LM_BLINKER_RIGHT:
                blinker = IBUS_LSZ_BLINKER_RIGHT;
                break;
        }
        unsigned char msg[] = {
            IBUS_CMD_DIA_JOB_REQUEST,
            0x00,
            0x00,
            IBUS_LSZ_HEADLIGHT_OFF,
            blinker,
            0x00,
            0x00,
            0x00,
            ibus->lmLoadFrontVoltage,
            0x00,
            ibus->lmDimmerVoltage,
            ibus->lmLoadRearVoltage,
            ibus->lmPhotoVoltage,
            0x00,
            0x00,
            0x00
        };
        IBusSendCommand(
            ibus,
            IBUS_DEVICE_DIA,
            IBUS_DEVICE_LCM,
            msg,
            sizeof(msg)
        );
    }
}


/**
 * IBusCommandLMGetRedundantData()
 *     Description:
 *        Query the Light Module for the vehicle vehicle
 *        Raw: 80 03 D0 53 00
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandLMGetRedundantData(IBus_t *ibus)
{
    unsigned char msg[] = {IBUS_CMD_LCM_REQ_REDUNDANT_DATA};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_IKE,
        IBUS_DEVICE_LCM,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandMIDDisplayText()
 *     Description:
 *        Send text to the MID screen. Add a watermark so we do not fight
 *        ourselves writing to the screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDDisplayTitleText(IBus_t *ibus, char *message)
{
    unsigned char displayText[strlen(message) + 4];
    displayText[0] = IBUS_CMD_RAD_WRITE_MID_DISPLAY;
    displayText[1] = 0xC0;
    displayText[2] = 0x20;
    uint8_t idx;
    uint8_t textLength = strlen(message);
    if (textLength > IBus_MID_TITLE_MAX_CHARS) {
        textLength = IBus_MID_MAX_CHARS;
    }
    for (idx = 0; idx < textLength; idx++) {
        displayText[idx + 3] = message[idx];
    }
    displayText[idx + 3] = IBUS_RAD_MAIN_AREA_WATERMARK;
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_MID,
        displayText,
        sizeof(displayText)
    );
}

/**
 * IBusCommandMIDDisplayText()
 *     Description:
 *        Send text to the MID screen. Add a watermark so we do not fight
 *        ourselves writing to the screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDDisplayText(IBus_t *ibus, char *message)
{
    uint8_t textLength = strlen(message);
    if (textLength > IBus_MID_MAX_CHARS) {
        textLength = IBus_MID_MAX_CHARS;
    }
    unsigned char displayText[textLength + 4];
    displayText[0] = IBUS_CMD_RAD_WRITE_MID_DISPLAY;
    displayText[1] = 0x40;
    displayText[2] = 0x20;
    uint8_t idx;
    for (idx = 0; idx < textLength; idx++) {
        displayText[idx + 3] = message[idx];
    }
    displayText[idx + 3] = IBUS_RAD_MAIN_AREA_WATERMARK;
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_IKE,
        IBUS_DEVICE_MID,
        displayText,
        sizeof(displayText)
    );
}

/**
 * IBusCommandMIDMenuText()
 *     Description:
 *        Send text to the MID menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDMenuText(IBus_t *ibus, uint8_t idx, char *text)
{
    uint8_t textLength = strlen(text);
    if (textLength > IBus_MID_MENU_MAX_CHARS) {
        textLength = IBus_MID_MENU_MAX_CHARS;
    }
    unsigned char menuText[textLength + 4];
    menuText[0] = IBUS_CMD_RAD_WRITE_MID_MENU;
    menuText[1] = 0xC3;
    menuText[2] = 0x00;
    menuText[3] = 0x40 + idx;
    uint8_t textIdx;
    for (textIdx = 0; textIdx < textLength; textIdx++) {
        menuText[textIdx + 4] = text[textIdx];
    }
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_MID,
        menuText,
        sizeof(menuText)
    );
}

/**
 * IBusCommandRADC43ScreenModeSet()
 *     Description:
 *        Send the command that the C43 sends to update the screen mode
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char mode - The mode to broadcast
 *     Returns:
 *         void
 */
void IBusCommandRADC43ScreenModeSet(IBus_t *ibus, unsigned char mode)
{
    unsigned char msg[4] = {
        IBUS_CMD_RAD_C43_SCREEN_UPDATE,
        IBUS_CMD_RAD_C43_SET_MENU_MODE,
        0x00,
        mode
    };
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADClearMenu()
 *     Description:
 *        Clear the Radio Menu. The first bit here tells the GT to clear the
 *        screen. We're using 0x0B to attempt to keep certain radios from
 *        realizing that the screen has been cleared
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADClearMenu(IBus_t *ibus)
{
    unsigned char msg[] = {0x46, 0x0A};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADDisableMenu()
 *     Description:
 *        Disable the Radio Menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADDisableMenu(IBus_t *ibus)
{
    unsigned char msg[] = {0x45, 0x02};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_RAD,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADEnableMenu()
 *     Description:
 *        Enable the Radio Menu
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADEnableMenu(IBus_t *ibus)
{
    unsigned char msg[] = {0x45, 0x00};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_RAD,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandRADExitMenu()
 *     Description:
 *        Exit the radio menu and return to the BMBT home screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADExitMenu(IBus_t *ibus)
{
    unsigned char msg[] = {0x45, 0x91};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_GT,
        IBUS_DEVICE_RAD,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandSetVolune()
 *     Description:
 *        Exit the radio menu and return to the BMBT home screen
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char source - The source system of the command
 *         unsigned char dest - The destination system of the command
 *         unsigned char volume - The volume and direction to issue
 *     Returns:
 *         void
 */
void IBusCommandSetVolune(
    IBus_t *ibus,
    unsigned char source,
    unsigned char dest,
    unsigned char volume
) {
    unsigned char msg[] = {IBUS_CMD_VOLUME_SET, volume};
    IBusSendCommand(
        ibus,
        source,
        dest,
        msg,
        sizeof(msg)
    );
}

/**
 * IBusCommandTELSetGTDisplayMenu()
 *     Description:
 *        Enable the Telephone Menu on the GT
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandTELSetGTDisplayMenu(IBus_t *ibus)
{
    const unsigned char msg[] = {IBUS_TEL_CMD_MAIN_MENU, 0x42, 0x02, 0x20};
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_GT, msg, sizeof(msg));
}

/**
 * IBusCommandTELSetLEDIBus()
 *     Description:
 *        Set the LED indicator status
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char leds - The bitmask of LEDs to light up
 *     Returns:
 *         void
 */
void IBusCommandTELSetLED(IBus_t *ibus, unsigned char leds)
{
    const unsigned char msg[] = {IBUS_TEL_CMD_LED_STATUS, leds};
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
}

/**
 * IBusCommandTELStatus()
 *     Description:
 *        Send the TEL Announcement Message so the car knows we're here
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char status - The status to send to the front display
 *     Returns:
 *         void
 */
void IBusCommandTELStatus(IBus_t *ibus, unsigned char status)
{
    const unsigned char msg[] = {IBUS_TEL_CMD_STATUS, status};
    IBusSendCommand(ibus, IBUS_DEVICE_TEL, IBUS_DEVICE_ANZV, msg, sizeof(msg));
}


/* Temporary Commands for debugging */
void IBusCommandIgnitionStatus(IBus_t *ibus, unsigned char status)
{
    unsigned char statusMessage[2] = {0x11, status};
    IBusSendCommand(ibus, IBUS_DEVICE_IKE, IBUS_DEVICE_GLO, statusMessage, 2);
}

void IBusCommandLCMTurnLeft(IBus_t *ibus)
{
    unsigned char statusMessage[] = {0x5B, 0xC3, 0xEF, 0x26, 0x33};
    IBusSendCommand(ibus, IBUS_DEVICE_LCM, IBUS_DEVICE_GLO, statusMessage, 5);
}

void IBusCommandLCMTurnRight(IBus_t *ibus)
{
    unsigned char statusMessage[] = {0x5B, 0x23, 0xEF, 0x26, 0x33};
    IBusSendCommand(ibus, IBUS_DEVICE_LCM, IBUS_DEVICE_GLO, statusMessage, 5);
}
