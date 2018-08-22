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
    ibus.cdChangerStatus = IBUS_CDC_PLAYING;
    ibus.ignitionStatus = 0x01;
    ibus.rxBufferIdx = 0;
    ibus.rxLastStamp = 0;
    ibus.txBufferReadIdx = 0;
    ibus.txBufferWriteIdx = 0;
    ibus.txLastStamp = TimerGetMillis();
    ibus.gtHardwareVersion = IBUS_GT_MKI;
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
        switch (hardwareVersion) {
            case 10:
                ibus->gtHardwareVersion = IBUS_GT_MKIV;
                break;
            case 11:
                ibus->gtHardwareVersion = IBUS_GT_MKIII;
            case 21:
                ibus->gtHardwareVersion = IBUS_GT_MKII;
            // No idea what an MKI reports -- Anything else must be it?
            default:
                ibus->gtHardwareVersion = IBUS_GT_MKI;
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
        LogDebug("IBus: Got GT HW %d SW %d", hardwareVersion, softwareVersion);
        EventTriggerCallback(IBusEvent_GTDiagResponse, pkt);
    } else if (pkt[3] == IBusAction_GT_MENU_SELECT) {
        EventTriggerCallback(IBusEvent_GTMenuSelect, pkt);
    }
}

static void IBusHandleIKEMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[3] == IBusAction_IGN_STATUS_REQ) {
        if (pkt[4] == 0x00 && ibus->ignitionStatus == IBUS_IGNITION_ON) {
            // Implied that the CDC should not be playing with the ignition off
            ibus->cdChangerStatus = 0;
            ibus->ignitionStatus = IBUS_IGNITION_OFF;
            EventTriggerCallback(IBusEvent_IgnitionStatus, pkt);
        } else if (ibus->ignitionStatus == IBUS_IGNITION_OFF) {
            ibus->ignitionStatus = IBUS_IGNITION_ON;
            EventTriggerCallback(IBusEvent_IgnitionStatus, pkt);
        }
    }
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
                LogRaw(
                    "[%llu] ERROR: IBus: RX Invalid Length [%d - %02X]: ",
                    ts,
                    msgLength,
                    ibus->rxBuffer[1]
                );
                uint8_t idx;
                for (idx = 0; idx < ibus->rxBufferIdx; idx++) {
                    LogRaw("%02X ", ibus->rxBuffer[idx]);
                }
                LogRaw("\r\n");
                ibus->rxBufferIdx = 0;
                memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
                CharQueueReset(&ibus->uart.rxQueue);
            } else if (msgLength == ibus->rxBufferIdx) {
                uint8_t idx;
                unsigned char pkt[msgLength];
                for(idx = 0; idx < msgLength; idx++) {
                    pkt[idx] = ibus->rxBuffer[idx];
                }
                if (IBusValidateChecksum(pkt) == 1) {
                    LogDebug(
                        "IBus: RX: %02X -> %02X Action: %02X Length: %d",
                        pkt[0],
                        pkt[2],
                        pkt[3],
                        (uint8_t) pkt[1]
                    );
                    unsigned char srcSystem = pkt[0];
                    if (srcSystem == IBUS_DEVICE_BMBT) {
                        IBusHandleBMBTMessage(pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_IKE) {
                        IBusHandleIKEMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_GT) {
                        IBusHandleGTMessage(ibus, pkt);
                    }
                    if (srcSystem == IBUS_DEVICE_RAD) {
                        IBusHandleRadioMessage(ibus, pkt);
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
    } else if (ibus->txBufferWriteIdx != ibus->txBufferReadIdx &&
        (TimerGetMillis() - ibus->txLastStamp) >= IBUS_TX_BUFFER_WAIT
    ) {
        uint8_t msgLen = (uint8_t) ibus->txBuffer[ibus->txBufferReadIdx][1] + 2;
        uint8_t idx;
        /*
         * Make sure that the STATUS pin on the TH3122 is low, indicating no
         * bus activity, before transmitting
         */
        if (IBUS_UART_STATUS == 0) {
            for (idx = 0; idx < msgLen; idx++) {
                ibus->uart.registers->uxtxreg = ibus->txBuffer[ibus->txBufferReadIdx][idx];
                // Wait for the data to leave the TX buffer
                while ((ibus->uart.registers->uxsta & (1 << 9)) != 0);
            }
            LogDebug(
                "IBus: TX: %02X -> %02X",
                ibus->txBuffer[ibus->txBufferReadIdx][0],
                ibus->txBuffer[ibus->txBufferReadIdx][2]
            );
            // Clear the slot and advance the index
            memset(ibus->txBuffer[ibus->txBufferReadIdx], 0, msgLen);
            if (ibus->txBufferReadIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                ibus->txBufferReadIdx = 0;
            } else {
                ibus->txBufferReadIdx++;
            }
        }
        ibus->txLastStamp = TimerGetMillis();
    }

    // Clear the RX Buffer if it's over the timeout or about to overflow
    if (ibus->rxBufferIdx > 0) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->rxLastStamp) > IBUS_RX_BUFFER_TIMEOUT ||
            (ibus->rxBufferIdx + 1) == IBUS_RX_BUFFER_SIZE
        ) {
            long long unsigned int ts = (long long unsigned int) TimerGetMillis();
            LogRaw(
                "[%llu] ERROR: IBus: RX Buffer Timeout [%d]: ",
                ts,
                ibus->rxBufferIdx
            );
            uint8_t idx;
            for (idx = 0; idx < ibus->rxBufferIdx; idx++) {
                LogRaw("%02X ", ibus->rxBuffer[idx]);
            }
            LogRaw("\r\n");
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

/**
 * IBusStartup()
 *     Description:
 *        Trigger any callbacks listening for our Startup event
 *     Params:
 *         None
 *     Returns:
 *         void
 */
void IBusStartup()
{
    EventTriggerCallback(IBusEvent_Startup, 0);
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
    LogDebug("IBus: Announce CD Changer");
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
    LogDebug("IBus: Send CD Changer Keep-Alive");
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
 *     Returns:
 *         void
 */
void IBusCommandCDCStatus(IBus_t *ibus, unsigned char action, unsigned char status) {
    LogDebug("IBus: Send CD Changer Status");
    status = status + 0x80;
    const unsigned char cdcStatus[] = {
        IBUS_COMMAND_CDC_SET_STATUS,
        action,
        status,
        0x00,
        0x01,
        0x00,
        0x01,
        0x01,
        0x00,
        0x01,
        0x01,
        0x01
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
    LogDebug("IBus: Get Ignition Status");
    unsigned char msg[] = {0x10};
    IBusSendCommand(ibus, IBUS_DEVICE_CDC, IBUS_DEVICE_IKE, msg, 1);
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
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT,
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
        IBUS_DEVICE_RAD,
        IBUS_DEVICE_GT,
        msg,
        sizeof(msg)
    );
}
