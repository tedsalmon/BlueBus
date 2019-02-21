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
    // Assume we're playing and the key inserted, so device resets are graceful
    ibus.cdChangerStatus = IBUS_CDC_NOT_PLAYING;
    ibus.ignitionStatus = IBUS_IGNITION_OFF;
    ibus.rxBufferIdx = 0;
    ibus.rxLastStamp = 0;
    ibus.txBufferReadIdx = 0;
    ibus.txBufferReadbackIdx = 0;
    ibus.txBufferWriteIdx = 0;
    ibus.txLastStamp = TimerGetMillis();
    ibus.gtCanDisplayStatic = 0;
    return ibus;
}

static void IBusHandleBMBTMessage(unsigned char *pkt)
{
    if (pkt[3] == IBusAction_BMBT_BUTTON) {
        EventTriggerCallback(IBusEvent_BMBTButton, pkt);
    }
}

static void IBusHandleGTMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[2] == IBUS_DEVICE_DIA && pkt[3] == IBusAction_DIAG_DATA) {
        // Decode the software and hardware versions
        char hwVersion[3] = {
            pkt[IBUS_GT_HW_ID_OFFSET],
            pkt[IBUS_GT_HW_ID_OFFSET + 1],
            '\0'
        };
        uint8_t hardwareVersion = strToInt(hwVersion);
        uint8_t gtHardwareVersion = 0;
        switch (hardwareVersion) {
            case 10:
                gtHardwareVersion = IBUS_GT_MKIV;
                break;
            case 11:
                gtHardwareVersion = IBUS_GT_MKIII;
            case 21:
                gtHardwareVersion = IBUS_GT_MKII;
            // No idea what an MKI reports -- Anything else must be it?
            default:
                gtHardwareVersion = IBUS_GT_MKI;
        }
        char swVersion[3] = {
            (char) pkt[IBUS_GT_SW_ID_OFFSET],
            (char) pkt[IBUS_GT_SW_ID_OFFSET + 1],
            '\0'
        };
        uint8_t softwareVersion = strToInt(swVersion);
        if (softwareVersion == 0 || softwareVersion >= 40) {
            ibus->gtCanDisplayStatic = 1;
        }
        LogDebug(LOG_SOURCE_IBUS, "IBus: Got GT HW %d SW %d", hardwareVersion, softwareVersion);
        EventTriggerCallback(IBusEvent_GTDiagResponse, pkt);
    } else if (pkt[3] == IBusAction_GT_MENU_SELECT) {
        EventTriggerCallback(IBusEvent_GTMenuSelect, pkt);
    } else if (pkt[3] == IBusAction_GT_SCREEN_MODE_SET) {
        EventTriggerCallback(IBusEvent_ScreenModeSet, pkt);
    }
}

static void IBusHandleIKEMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[3] == IBusAction_IGN_STATUS_REQ) {
        uint8_t ignitionStatus;
        if (pkt[4] == IBUS_IGNITION_OFF) {
            // Implied that the CDC should not be playing with the ignition off
            ibus->cdChangerStatus = 0;
            ignitionStatus = IBUS_IGNITION_OFF;
        } else {
            ignitionStatus = IBUS_IGNITION_ON;
        }
        if (ibus->ignitionStatus != ignitionStatus) {
            ibus->ignitionStatus = ignitionStatus;
            EventTriggerCallback(IBusEvent_IgnitionStatus, pkt);
        }
    }
}

static void IBusHandleMFLMessage(IBus_t *ibus, unsigned char *pkt)
{
    /*
    50 04 68 3B 21 26 <next> release
    50 04 68 3B 28 2F <previous> release
    50 04 C8 3B 80 27 <R/T>
    50 04 C8 3B 90 37 <voice> hold
    50 04 C8 3B A0 07 <voice> release
    */
    //if (pkt[4] == IBUS_MFL_BTN_EVENT)
}

static void IBusHandleRadioMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[2] == IBUS_DEVICE_CDC) {
        if (pkt[3] == IBUS_COMMAND_CDC_ALIVE) {
            EventTriggerCallback(IBusEvent_CDKeepAlive, pkt);
        } else if(pkt[3] == IBUS_COMMAND_CDC_GET_STATUS) {
            if (pkt[4] == IBUS_CDC_STOP_PLAYING) {
                ibus->cdChangerStatus = IBUS_CDC_NOT_PLAYING;
            } else if (pkt[4] == 0x02 || pkt[4] == 0x03) {
                ibus->cdChangerStatus = IBUS_CDC_PLAYING;
            }
            EventTriggerCallback(IBusEvent_CDStatusRequest, pkt);
        }
    } else if (pkt[2] == IBUS_DEVICE_DIA && pkt[3] == IBusAction_DIAG_DATA) {

    } else if (pkt[2] == IBUS_DEVICE_GT) {
        if (pkt[3] == IBusAction_RAD_SCREEN_MODE_UPDATE) {
            EventTriggerCallback(IBusEvent_ScreenModeUpdate, pkt);
        }
        if (pkt[3] == IBusAction_RAD_UPDATE_MAIN_AREA) {
            EventTriggerCallback(IBusEvent_RADUpdateMainArea, pkt);
        }
    } else if (pkt[2] == IBUS_DEVICE_LOC) {
        if (pkt[3] == 0x3B) {
            EventTriggerCallback(IBusEvent_CDClearDisplay, pkt);
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
                } else {
                    LogError(
                        "IBus: %02X -> %02X Length: %d - Invalid Checksum",
                        pkt[0],
                        pkt[2],
                        msgLength,
                        (uint8_t) pkt[1]
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
 * IBusCommandCDCKeepAlive()
 *     Description:
 *        Respond to the Radio's "Ping" with our "Pong"
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandCDCKeepAlive(IBus_t *ibus)
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
 *         unsigned char action - The current CDC action
 *         unsigned char status - The current CDC status
 *         unsigned char discCount - The number of Discs to report loaded
 *     Returns:
 *         void
 */
void IBusCommandCDCStatus(
    IBus_t *ibus,
    unsigned char action,
    unsigned char status,
    unsigned char discCount
) {
    status = status + 0x80;
    const unsigned char cdcStatus[] = {
        IBUS_COMMAND_CDC_SET_STATUS,
        action,
        status,
        0x00,
        discCount,
        0x00,
        0x01,
        0x01,
        0x00,
        0x01,
        0x01, // Disc Number
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

void IBusCommandGTGetDiagnostics(IBus_t *ibus)
{
    unsigned char msg[] = {0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_GT, msg, 1);
}

void IBusCommandGTUpdate(IBus_t *ibus, unsigned char updateType)
{
    unsigned char msg[4] = {
        IBusAction_GT_WRITE_MK2,
        updateType,
        0x01,
        0x00
    };
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, msg, 4);
}

void IBusCommandGTWriteIndexMk2(IBus_t *ibus, uint8_t index, char *message) {
    IBusCommandGTWriteIndex(
        ibus,
        index,
        message,
        IBusAction_GT_WRITE_MK2,
        IBusAction_GT_WRITE_ZONE
    );
}

void IBusCommandGTWriteIndexMk4(IBus_t *ibus, uint8_t index, char *message) {
    IBusCommandGTWriteIndex(
        ibus,
        index,
        message,
        IBusAction_GT_WRITE_MK4,
        IBusAction_GT_WRITE_INDEX
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

void IBusCommandGTWriteIndex(
    IBus_t *ibus,
    uint8_t index,
    char *message,
    unsigned char command,
    unsigned char mode
) {
    uint8_t length = strlen(message);
    if (length > 20) {
        length = 20;
    }
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = command;
    text[1] = mode;
    text[2] = 0x00;
    text[3] = 0x40 + (unsigned char) index;
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
    text[0] = IBusAction_GT_WRITE_MK4;
    text[1] = IBusAction_GT_WRITE_STATIC;
    text[2] = 0x00;
    text[3] = 0x40 + (unsigned char) index;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 4] = message[idx];
    }
    IBusSendCommand(ibus, IBUS_DEVICE_RAD, IBUS_DEVICE_GT, text, pktLenght);
}

void IBusCommandGTWriteTitle(IBus_t *ibus, char *message)
{
    uint8_t length = strlen(message);
    if (length > 11) {
        length = 11;
    }
    // Length + Write Type + Write Area + Size + Watermark
    const size_t pktLenght = length + 4;
    unsigned char text[pktLenght];
    text[0] = IBusAction_GT_WRITE_TITLE;
    text[1] = IBusAction_GT_WRITE_ZONE;
    text[2] = 0x10;
    uint8_t idx;
    for (idx = 0; idx < length; idx++) {
        text[idx + 3] = message[idx];
    }
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
    text[0] = IBusAction_GT_WRITE_MK2;
    text[1] = IBusAction_GT_WRITE_ZONE;
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
    LogDebug(LOG_SOURCE_IBUS, "IBus: Get Ignition Status");
    unsigned char msg[] = {0x10};
    IBusSendCommand(ibus, IBUS_DEVICE_BMBT, IBUS_DEVICE_IKE, msg, 1);
}

/**
 * IBusCommandMIDText()
 *     Description:
 *        Send text to the radio's MID
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *         char *message - The to display on the MID
 *     Returns:
 *         void
 */
void IBusCommandMIDText(IBus_t *ibus, char *message)
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
 * IBusCommandMIDTextClear()
 *     Description:
 *        Send an empty string to the MID to clear the display
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandMIDTextClear(IBus_t *ibus)
{
    IBusCommandMIDText(ibus, 0);
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
 * IBusCommandRADGetDiagnostics()
 *     Description:
 *        Request Diagnostic info from the radio
 *     Params:
 *         IBus_t *ibus - The pointer to the IBus_t object
 *     Returns:
 *         void
 */
void IBusCommandRADGetDiagnostics(IBus_t *ibus)
{
    unsigned char msg[] = {0x00};
    IBusSendCommand(ibus, IBUS_DEVICE_DIA, IBUS_DEVICE_RAD, msg, 1);
}

/* Temporary Commands for debugging */
void IBusCommandIgnitionStatus(IBus_t *ibus, unsigned char status)
{
    unsigned char statusMessage[2] = {0x11, status};
    IBusSendCommand(ibus, IBUS_DEVICE_IKE, IBUS_DEVICE_GLO, statusMessage, 2);
}
