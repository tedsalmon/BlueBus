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
    ibus.cdChangerStatus = 0x01;
    ibus.ignitionStatus = 0x01;
    ibus.rxBufferIdx = 0;
    ibus.rxLastStamp = 0;
    ibus.txBufferReadIdx = 0;
    ibus.txBufferWriteIdx = 0;
    ibus.txLastStamp = TimerGetMillis();
    return ibus;
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
        ibus->rxBuffer[ibus->rxBufferIdx] = CharQueueNext(&ibus->uart.rxQueue);
        ibus->rxBufferIdx++;
        if (ibus->rxBufferIdx > 1) {
            uint8_t msgLength = (uint8_t) ibus->rxBuffer[1] + 2;
            // Make sure we do not read more than the maximum packet length
            if (msgLength > IBUS_MAX_MSG_LENGTH) {
                LogError("IBus: Invalid Message Length of %d found", msgLength);
                ibus->rxBufferIdx = 0;
                memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
                CharQueueReset(&ibus->uart.rxQueue);
            } else if (msgLength == ibus->rxBufferIdx) {
                uint8_t idx, pktSize;
                pktSize = msgLength;
                unsigned char pkt[pktSize];
                for(idx = 0; idx < pktSize; idx++) {
                    pkt[idx] = ibus->rxBuffer[idx];
                    ibus->rxBuffer[idx] = 0x00;
                }
                if (IBusValidateChecksum(pkt) == 1) {
                    LogDebug(
                        "IBus: %02X -> %02X Action: %02X Length: %d",
                        pkt[0],
                        pkt[2],
                        pkt[3],
                        (uint8_t) pkt[1]
                    );
                    unsigned char srcSystem = pkt[0];
                    if (srcSystem == IBusDevice_RAD) {
                        IBusHandleRadioMessage(ibus, pkt);
                    }
                    if (srcSystem == IBusDevice_IKE) {
                        IBusHandleIKEMessage(ibus, pkt);
                    }
                } else {
                    LogError(
                        "IBus: %02X -> %02X Length: %d has invalid checksum",
                        pkt[0],
                        pkt[2],
                        pkt[3],
                        (uint8_t) pkt[1]
                    );
                }
                ibus->rxBufferIdx = 0;
            }
            ibus->rxLastStamp = TimerGetMillis();
        }
    // If we have a byte to read, and we're not still receiving data, transmit
    } else if (ibus->txBufferWriteIdx != ibus->txBufferReadIdx &&
        ibus->rxBufferIdx == 0
    ) {
        uint32_t now = TimerGetMillis();
        // Wait at least IBUS_TX_BUFFER_WAIT before attempting to send
        // a new message, since we want to maintain a lower priority on the bus
        if ((now - ibus->txLastStamp) >= IBUS_TX_BUFFER_WAIT) {
            LogDebug("IBus: Transmit Message");
            // Make sure we have not gotten or are currently getting a byte
            if (ibus->uart.rxQueue.size == 0 && ibus->uart.status == UART_STATUS_IDLE) {
                uint8_t shouldClear = 1;
                uint8_t msgLen = (uint8_t) ibus->txBuffer[ibus->txBufferReadIdx][1] + 2;
                uint8_t idx;
                for (idx = 0; idx < msgLen; idx++) {
                    // If there isn't a byte waiting
                    if (ibus->uart.status == UART_STATUS_IDLE) {
                        ibus->uart.registers->uxtxreg = ibus->txBuffer[ibus->txBufferReadIdx][idx];
                        // Wait for the data to leave the tx buffer
                        while ((ibus->uart.registers->uxsta & (1 << 9)) != 0);
                    } else {
                        // Stop transmitting and keep the data in the buffer
                        idx = msgLen;
                        shouldClear = 0;
                        LogWarning("IBus: Detected RX while transmitting - Abort");
                    }
                }
                LogDebug("IBus: Finish Tx");
                if (shouldClear == 1) {
                     memset(ibus->txBuffer[ibus->txBufferReadIdx], 0, msgLen);
                    if (ibus->txBufferReadIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                        ibus->txBufferReadIdx = 0;
                    } else {
                        ibus->txBufferReadIdx++;
                    }
                }
            }
            ibus->txLastStamp = now;
        }
    }
    // Clear the RX Buffer if it's over the timeout or about to overflow
    if (ibus->rxBufferIdx > 0) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->rxLastStamp) > IBUS_RX_BUFFER_TIMEOUT ||
            (ibus->rxBufferIdx + 1) == IBUS_RX_BUFFER_SIZE
        ) {
            LogError(
                "IBus: %d bytes in the RX buffer timed out",
                ibus->rxBufferIdx + 1
            );
            ibus->rxBufferIdx = 0;
            memset(ibus->rxBuffer, 0, IBUS_RX_BUFFER_SIZE);
        }
    }
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

void IBusCommandDisplayText(IBus_t *ibus, char *message)
{
    LogDebug("IBus: Display Text");
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
        IBusDevice_TEL,
        IBusDevice_IKE,
        displayText,
        sizeof(displayText)
    );
}

void IBusCommandDisplayTextClear(IBus_t *ibus)
{
    LogDebug("IBus: Clear Display Text");
    IBusCommandDisplayText(ibus, 0);
}

void IBusCommandSendCdChangeAnnounce(IBus_t *ibus)
{
    LogDebug("IBus: Announce CD Changer");
    const unsigned char cdcAlive[] = {0x02, 0x01};
    IBusSendCommand(ibus, IBusDevice_CDC, IBusDevice_LOC, cdcAlive, sizeof(cdcAlive));
}

void IBusCommandSendCdChangerKeepAlive(IBus_t *ibus)
{
    LogDebug("IBus: Send CD Changer Keep-Alive");
    const unsigned char cdcPing[] = {0x02, 0x00};
    IBusSendCommand(ibus, IBusDevice_CDC, IBusDevice_RAD, cdcPing, sizeof(cdcPing));
}

void IBusCommandSendCdChangerStatus(
    IBus_t *ibus,
    unsigned char *curStatus,
    unsigned char *curAction
) {
    LogDebug("IBus: Send CD Changer Status");
    const unsigned char cdcStatus[] = {
        IBusAction_CD_STATUS_REP,
        *curAction,
        *curStatus,
        0x00,
        0x3F,
        0x00,
        0x01, // CD
        0x01 // Track
    };
    IBusSendCommand(ibus, IBusDevice_CDC, IBusDevice_RAD, cdcStatus, sizeof(cdcStatus));
}

void IBusHandleIKEMessage(IBus_t *ibus, unsigned char *pkt)
{

}

void IBusHandleRadioMessage(IBus_t *ibus, unsigned char *pkt)
{
    if (pkt[2] == IBusDevice_CDC) {
        if (pkt[3] == IBusAction_CD_KEEPALIVE) {
            EventTriggerCallback(IBusEvent_CDKeepAlive, pkt);
        } else if(pkt[3] == IBusAction_CD_STATUS_REQ) {
            if (pkt[4] == 0x01 || pkt[4] == 0x03) {
                ibus->cdChangerStatus = pkt[4];
            }
            EventTriggerCallback(IBusEvent_CDStatusRequest, pkt);
        }
    }
}

uint8_t IBusValidateChecksum(unsigned char *msg)
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
