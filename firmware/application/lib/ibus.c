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
        IBUS_UART_RX_PIN,
        IBUS_UART_TX_PIN,
        IBUS_UART_RX_PRIORITY,
        IBUS_UART_TX_PRIORITY,
        UART_BAUD_9600,
        UART_PARITY_EVEN
    );
    ibus.cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
    ibus.ignitionStatus = IBUS_IGNITION_OFF;
    ibus.lcmDimmerStatus1 = 0x80;
    ibus.lcmDimmerStatus2 = 0x80;
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
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON0 ||
        pkt[IBUS_PKT_CMD] == IBUS_CMD_BMBT_BUTTON1
    ) {
        EventTriggerCallback(IBusEvent_BMBTButton, pkt);
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
    if (pkt[IBUS_PKT_LEN] == 0x22 &&
        pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
        pkt[IBUS_PKT_CMD] == IBUS_CMD_DIAG_RESPONSE
    ) {
        // Decode the software and hardware versions
        uint8_t hardwareVersion = IBusGetNavHWVersion(pkt);
        uint8_t softwareVersion = IBusGetNavSWVersion(pkt);
        uint8_t navType = IBusGetNavType(pkt);
        if (navType != IBUS_GT_DETECT_ERROR) {
            LogRaw(
                "IBus: GT P/N: %c%c%c%c%c%c%c HW: %d SW: %d Build: %c%c/%c%c\r\n",
                pkt[4],
                pkt[5],
                pkt[6],
                pkt[7],
                pkt[8],
                pkt[9],
                pkt[10],
                hardwareVersion,
                softwareVersion,
                pkt[19],
                pkt[20],
                pkt[21],
                pkt[22]
            );
            EventTriggerCallback(IBusEvent_GTDiagResponse, pkt);
        }
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_MENU_SELECT) {
        EventTriggerCallback(IBusEvent_GTMenuSelect, pkt);
    } else if (pkt[IBUS_PKT_CMD] == IBUS_CMD_GT_SCREEN_MODE_SET) {
        EventTriggerCallback(IBusEvent_ScreenModeSet, pkt);
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
    if (pkt[IBUS_PKT_CMD] == IBUS_CMD_IGN_STATUS_RESP) {
        uint8_t ignitionStatus;
        if (pkt[4] == IBUS_IGNITION_OFF) {
            // Implied that the CDC should not be playing with the ignition off
            ibus->cdChangerFunction = IBUS_CDC_FUNC_NOT_PLAYING;
            ignitionStatus = IBUS_IGNITION_OFF;
        } else {
            ignitionStatus = IBUS_IGNITION_ON;
        }
        // The order of the items below should not be changed,
        // otherwise listeners will not know if the ignition status
        // has changed
        EventTriggerCallback(
            IBusEvent_IgnitionStatus,
            &ignitionStatus
        );
        ibus->ignitionStatus = ignitionStatus;
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
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GLO &&
        pkt[IBUS_PKT_CMD] == IBUS_LCM_LIGHT_STATUS
    ) {
        EventTriggerCallback(IBusEvent_LCMLightStatus, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_GLO &&
               pkt[IBUS_PKT_CMD] == IBUS_LCM_DIMMER_STATUS
    ) {
        EventTriggerCallback(IBusEvent_LCMDimmerStatus, pkt);
    } else if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_DIA &&
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIAG_RESPONSE &&
               pkt[IBUS_PKT_LEN] == 0x23
    ) {
        ibus->lcmDimmerStatus1 = pkt[19];
        ibus->lcmDimmerStatus2 = pkt[20];
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
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_RAD) {
        if(pkt[IBUS_PKT_CMD] == IBis_MID_Button_Press) {
            EventTriggerCallback(IBusEvent_MIDButtonPress, pkt);
        }
    }
}

static void IBusHandleRadioMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[IBUS_PKT_DST] == IBUS_DEVICE_CDC) {
        if (pkt[IBUS_PKT_CMD] == IBUS_COMMAND_CDC_POLL) {
            EventTriggerCallback(IBusEvent_CDPoll, pkt);
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
               pkt[IBUS_PKT_CMD] == IBUS_CMD_DIAG_RESPONSE
    ) {
        LogRaw(
            "IBus: RAD P/N: %d%d%d%d%d%d%d HW: %02d SW: %d%d Build: %d%d/%d%d\r\n",
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
                    unsigned char srcSystem = pkt[0];
                    if (srcSystem == IBUS_DEVICE_RAD) {
                        IBusHandleRadioMessage(ibus, pkt);
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
        ibus->rxLastStamp = TimerGetMillis();
    }

    // Flush the transmit buffer out to the bus
    uint8_t txTimeout = 0;
    uint8_t beginTxTimestamp = TimerGetMillis();
    while (ibus->txBufferWriteIdx != ibus->txBufferReadIdx &&
           txTimeout == IBUS_TX_TIMEOUT_OFF
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

uint8_t IBusGetDeviceManufacturer(const unsigned char mfgByte) {
    uint8_t deviceManufacturer = 0;
    switch (mfgByte) {
        case 0x00:
            //"Siemens"
            break;
        case 0x01:
            //"Reinshagen (Delphi)"
            break;
        case 0x02:
            //"Kostal"
            break;
        case 0x03:
            //"Hella"
            break;
        case 0x04:
            //"Siemens"
            break;
        case 0x07:
            //"Helbako"
            break;
        case 0x09:
            //"Loewe -> Lear"
            break;
        case 0x10:
            //"VDO"
            break;
        case 0x14:
            //"SWF"
            break;
        case 0x16:
            // MK3 "Philips / Siemens VDO" "0103708.21"
            break;
        case 0x17:
            //"Alpine"
            break;
        case 0x19:
            //"Kammerer"
            break;
        case 0x20:
            //"Becker"
            break;
        case 0x22:
            //"Alps"
            break;
        case 0x23:
            //"Continental"
            break;
        case 0x24:
            //"Temic"
            break;
        case 0x26:
            //"MotoMeter"
            break;
        case 0x34:
            //"VDO" 0108788.10" "0145067.10 For Mk4
            break;
        case 0x41:
            //"Megamos"
            break;
        case 0x46:
            //"Gemel"
            break;
        case 0x56:
            //"Siemens VDO Automotive"
            break;
        case 0x57:
            //"Visteon"
            break;
        case 0xD9:
            //"Webasto"
            break;
    }
    return deviceManufacturer;
}

/**
 * IBusGetRadioType()
 *     Description:
 *        Get the radio type based on the part number
 *     Params:
 *         uint32_t partNumber - The device part number
 *     Returns:
 *         void
 */
uint8_t IBusGetRadioType(uint32_t partNumber)
{
    uint8_t radioType = 0;
    switch (partNumber) {
        case 4160119:
        case 6915711:
        case 6924733:
        case 6924906:
        case 6927902:
        case 6961215:
        case 6961217:
        case 6932564:
        case 6933701:
        case 6921825:
        case 6935628:
        case 6932431:
        case 6939661:
        case 6943436:
        case 6976887:
        case 6921964:
        case 6927903:
        case 6941506:
        case 6915712:
        case 6919073:
        case 9124631:
            // Business Radio CD
            radioType = IBUS_RADIO_TYPE_BRCD;
            break;
        case 6900403:
        case 6915710:
        case 6928763:
        case 6935630:
        case 6943428:
        case 6923842:
        case 6943426:
            // Business Radio Tape Player
            radioType = IBUS_RADIO_TYPE_BRTP;
            break;
        case 6902718:
        case 6902719:
            // Navigation C43
            radioType = IBUS_RADIO_TYPE_C43;
            break;
        case 6904213:
        case 6904214:
        case 6919080:
        case 6919081:
        case 6922512:
        case 6922513:
        case 6933089:
        case 6933090:
        case 6927910:
        case 6927911:
        case 6943449:
        case 6943450:
        case 6964398:
        case 6964399:
        case 6972662:
        case 6972665:
        case 6976961:
        case 6976962:
        case 6988275:
        case 6988276:
            // Navigation BM53 (USDM)
            radioType = IBUS_RADIO_TYPE_BM53;
            break;
        case 6919079:
        case 6922511:
        case 6932812:
        case 6933092:
        case 6934650:
        case 6941691:
        case 6943452:
        case 6964401:
        case 6972667:
        case 6976963:
        case 6976964:
        case 8385457:
        case 9185185:
            // Navigation BM54 (EUDM)
            radioType = IBUS_RADIO_TYPE_BM54;
            break;
    }
    return radioType;
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
 * IBusGetRadioType()
 *     Description:
 *        Get the nav type based on the hardware and software versions
 *     Params:
 *         unsigned char *packet - The diagnostics packet
 *     Returns:
 *         uint8_t - The nav type
 */
uint8_t IBusGetNavType(unsigned char *packet)
{
    uint8_t hardwareVersion = IBusGetNavHWVersion(packet);
    uint8_t navType = 0;
    switch (hardwareVersion) {
        case 10:
            navType = IBUS_GT_MKIV;
            break;
        case 11:
            navType = IBUS_GT_MKIII;
            break;
        case 21:
            navType = IBUS_GT_MKII;
            break;
        case 50:
        case 53:
            navType = IBUS_GT_MKI;
            break;
        default:
            navType = IBUS_GT_DETECT_ERROR;
            break;
    }
    uint8_t softwareVersion = IBusGetNavSWVersion(packet);
    if (navType == IBUS_GT_MKIII && softwareVersion > 40) {
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
 * IBusCommandGetModuleStatus()
 *     Description:
 *        Request a "pong" from a given module to see if it is present
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char system - The system to target
 *     Returns:
 *         void
 */
void IBusCommandGetModuleStatus(IBus_t *ibus, unsigned char system)
{
    unsigned char msg[] = {0x01};
    IBusSendCommand(
        ibus,
        IBUS_DEVICE_RAD,
        system,
        msg,
        1
    );
}

void IBusCommandGTUpdate(IBus_t *ibus, unsigned char updateType)
{
    unsigned char msg[4] = {
        IBUS_CMD_GT_WRITE_MK2,
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
    unsigned char navVersion,
    unsigned char indexMode
) {
    unsigned char command;
    if (navVersion == IBUS_GT_MKI || navVersion == IBUS_GT_MKII) {
        command = IBUS_CMD_GT_WRITE_MK2;
        indexMode = IBUS_CMD_GT_WRITE_ZONE;
    } else {
        command = IBUS_CMD_GT_WRITE_MK4;
    }
    uint8_t length = strlen(message);
    if (length > 20) {
        length = 20;
    }
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = command;
    text[1] = indexMode;
    text[2] = 0x00;
    text[3] = 0x40 + (unsigned char) index;
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
    text[0] = 0x23;
    text[1] = 0xC4;
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
    char *message,
    unsigned char navVersion
) {
    IBusInternalCommandGTWriteIndex(
        ibus,
        index,
        message,
        navVersion,
        IBUS_CMD_GT_WRITE_INDEX
    );
}

void IBusCommandGTWriteIndexTMC(
    IBus_t *ibus,
    uint8_t index,
    char *message,
    unsigned char navVersion
) {
    IBusInternalCommandGTWriteIndex(
        ibus,
        index,
        message,
        navVersion,
        IBUS_CMD_GT_WRITE_INDEX_TMC
    );
}

void IBusCommandGTWriteIndexTitle(IBus_t *ibus, char *message) {
    uint8_t length = strlen(message);
    if (length > 20) {
        length = 20;
    }
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = 0x21;
    text[1] = 0x61;
    text[2] = 0x00;
    text[3] = 0x09;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteIndexStatic(IBus_t *ibus, uint8_t index, char *message)
{
    uint8_t length = strlen(message);
    if (length > 38) {
        length = 38;
    }
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = IBUS_CMD_GT_WRITE_MK4;
    text[1] = IBUS_CMD_GT_WRITE_STATIC;
    text[2] = 0x00;
    text[3] = 0x40 + (unsigned char) index;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
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
 * IBusCommandGTWriteTitleIndex()
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
    text[0] = IBUS_CMD_GT_WRITE_MK4;
    text[1] = IBUS_CMD_GT_WRITE_ZONE;
    text[2] = 0x01;
    text[3] = 0x40;
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
    text[0] = IBUS_CMD_GT_WRITE_MK2;
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
 * IBusCommandIKEGetIgnition()
 *     Description:
 *        Send the command to request the ignition status
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandIKEGetIgnition(IBus_t *ibus)
{
    unsigned char msg[] = {IBUS_CMD_IGN_STATUS_REQ};
    IBusSendCommand(ibus, IBUS_DEVICE_BMBT, IBUS_DEVICE_IKE, msg, 1);
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
 * IBusCommandLCMEnableBlinker()
 *     Description:
 *        Issue a diagnostic message to the LCM to enable the turn signals
 *            // E38/E39/E53/Range Rover Left / Right
 *            3F 0F D0 0C 00 00 40 00 00 00 00 00 00 FF FF 00 AC
 *            3F 0F D0 0C 00 00 80 00 00 00 00 00 00 FF FF 00 6C
 *            // E46/Z4 Left / Right
 *            3F 0F D0 0C 00 00 FF 50 00 00 00 80 00 80 80 00 C3
 *            3F 0F D0 0C 00 00 FF 80 00 00 00 80 00 80 80 00 13
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         unsigned char blinker - The byte containing the bits of which bulb
 *             to illuminate
 *     Returns:
 *         void
 */
void IBusCommandLCMEnableBlinker(IBus_t *ibus, unsigned char blinker) {
    unsigned char vehicleType = ConfigGetVehicleType();
    unsigned char lightStatus = 0x00;
    unsigned char lightStatus2 = 0x00;
    unsigned char ioStatus = 0x00;
    unsigned char ioStatus2 = 0xFF;
    unsigned char ioStatus3 = 0xFF;

    if (vehicleType == IBUS_VEHICLE_TYPE_E38_E39_E53) {
        lightStatus = blinker;
    } else if (vehicleType == IBUS_VEHICLE_TYPE_E46_Z4) {
        lightStatus = 0xFF;
        if (blinker == IBUS_LCM_BLINKER_DRV) {
            blinker = IBUS_LCM_BLINKER_DRV_E46;
        } else if (blinker == IBUS_LCM_BLINKER_PSG) {
            blinker = IBUS_LCM_BLINKER_PSG_E46;
        }
        lightStatus2 = blinker;
        ioStatus = 0x80;
        ioStatus2 = ibus->lcmDimmerStatus1;
        ioStatus3 = ibus->lcmDimmerStatus2;
    }
    // Only fire the command if the light status byte is set
    if (lightStatus != 0x00) {
        unsigned char msg[] = {
            0x0C,
            0x00,
            0x00,
            lightStatus,
            lightStatus2,
            0x00,
            0x00,
            0x00,
            ioStatus,
            0x00,
            ioStatus2,
            ioStatus3,
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
 * IBusCommandGMUnlock()
 *     Description:
 *        Issue a diagnostic message to the GM to unlock the car
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandGMUnlock(IBus_t *ibus) {
    unsigned char msg[] = {0x0C, 0x97, 0x01};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GM, msg, sizeof(msg));
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
