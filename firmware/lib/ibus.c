/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#include <stdlib.h>
#include "../io_mappings.h"
#include "char_queue.h"
#include "debug.h"
#include "ibus.h"
#include "timer.h"
#include "uart.h"

/**
 * IBusInit()
 *     Description:
 *         Returns a fresh IBus_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         struct IBus_t *
 */
struct IBus_t IBusInit()
{
    struct IBus_t ibus;
    ibus.uart = UARTInit(
        IBUS_UART_MODULE,
        IBUS_UART_RX_PIN,
        IBUS_UART_TX_PIN,
        UART_BAUD_9600,
        UART_PARITY_EVEN
    );
    ibus.cdStatus = 0x01;
    ibus.rxBufferIdx = 0;
    ibus.rxLastStamp = TimerGetMillis();
    ibus.txBufferReadIdx = 0;
    ibus.txBufferWriteIdx = 0;
    ibus.txLastStamp = TimerGetMillis();
    ibus.displayTextIdx = 0;
    ibus.displayTextLastStamp = TimerGetMillis();
    ibus.playbackStatus = 0;
    return ibus;
}

/**
 * IBusProcess()
 *     Description:
 *         Process messages in the IBus RX queue
 *     Params:
 *         struct IBus_t *ibus
 *         struct BC127_t *bt
 *     Returns:
 *         void
 */
void IBusProcess(struct IBus_t *ibus)
{
    uint8_t rxSize = ibus->uart.rxQueue.size;
    if (rxSize > 0) {
        ibus->rxBuffer[ibus->rxBufferIdx] = CharQueueNext(&ibus->uart.rxQueue);
        ibus->rxBufferIdx++;
        ibus->rxLastStamp = TimerGetMillis();
        if (ibus->rxBufferIdx > 2) {
            uint8_t msgLength = (uint8_t) ibus->rxBuffer[1] + 2;
            if (msgLength == ibus->rxBufferIdx) {
                uint8_t idx, pktSize;
                pktSize = msgLength;
                unsigned char pkt[pktSize];
                for(idx = 0; idx < pktSize; idx++) {
                    pkt[idx] = ibus->rxBuffer[idx];
                    ibus->rxBuffer[idx] = 0x00;
                }
                ibus->rxBufferIdx = 0;
                LogDebug(
                    "Got IBus Message 0x%x -> 0x%x [0x%x] with length %d",
                    pkt[0],
                    pkt[2],
                    pkt[3],
                    (uint8_t) pkt[1]
                );
                // Radio Requests CD Changer Keep Alive
                if (pkt[0] == IBusDevice_RAD && pkt[2] == IBusDevice_CDC && pkt[3] == 0x01) {
                    LogDebug("Fire Off Keep Alive");
                    const unsigned char cdcPing[] = {0x02, 0x00};
                    IBusSendCommand(ibus, IBusDevice_CDC, IBusDevice_RAD, cdcPing, sizeof(cdcPing));
                }
                // Radio Requests CD Changer Status
                if (pkt[0] == IBusDevice_RAD && pkt[2] == IBusDevice_CDC && pkt[3] == IBusAction_CD_STATUS_REQ) {
                    LogDebug("Fire Off Status");
                    ibus->cdStatus = pkt[4];
                    unsigned char curStatus = 0x02;
                    if (pkt[4] == 0x03) {
                        curStatus = 0x09;
                    }
                    if (ibus->cdStatus == 0x0A) {
                        if (pkt[5] == 0x00) {
                            ibus->playbackStatus = 3;
                        } else {
                            ibus->playbackStatus = 4;
                        }
                    }
                    // Radio Number pressed
                    if (ibus->cdStatus == 0x06) {
                        if (pkt[5] == 0x01) {
                            ibus->playbackStatus = 1;
                        }
                        if (pkt[5] == 0x02) {
                            ibus->playbackStatus = 2;
                        }
                    }
                    const unsigned char cdcStatus[] = {IBusAction_CD_STATUS_REP, 0x00, curStatus, 0x00, 0x3F, 0x00, 0x01, 0x01};
                    IBusSendCommand(ibus, IBusDevice_CDC, IBusDevice_RAD, cdcStatus, sizeof(cdcStatus));
                    // If we're not being polled or asked to stop playing, change the title
                    if (pkt[4] != 0x01 && pkt[4] != 0x00) {
                        IBusDisplayText(ibus, "BlueBus");
                    } else {
                        IBusDisplayTextClear(ibus);
                    }
                }
            }
        }
    } else if (ibus->txBufferWriteIdx != ibus->txBufferReadIdx) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->txLastStamp) >= IBUS_TX_BUFFER_WAIT) {
            LogDebug(
                "Transmitting Message 0x%x -> 0x%x",
                ibus->txBuffer[ibus->txBufferReadIdx][0],
                ibus->txBuffer[ibus->txBufferReadIdx][2]
            );
            unsigned char len = ibus->txBuffer[ibus->txBufferReadIdx][1];
            uint8_t msgLen = (uint8_t) len + 2;
            uint8_t idx;
            for (idx = 0; idx < msgLen; idx++) {
                CharQueueAdd(
                    &ibus->uart.txQueue,
                    ibus->txBuffer[ibus->txBufferReadIdx][idx]
                );
                // Reset the buffer
                ibus->txBuffer[ibus->txBufferReadIdx][idx] = 0x00;
            }
            ibus->txLastStamp = now;
            SetUARTTXIE(ibus->uart.moduleIndex, 1);
            if (ibus->txBufferReadIdx + 1 == IBUS_TX_BUFFER_SIZE) {
                ibus->txBufferReadIdx = 0;
            } else {
                ibus->txBufferReadIdx++;
            }
        }
    }
    if (ibus->rxBufferIdx > 0) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->rxLastStamp) > IBUS_RX_BUFFER_TIMEOUT) {
            LogWarning("Message in the IBus RX Buffer Timed out");
            ibus->rxBufferIdx = 0;
            uint8_t idx = 0;
            // Reset the data in the buffer
            while (idx < IBUS_RX_BUFFER_SIZE) {
                ibus->rxBuffer[idx] = 0x00;
                idx++;
            }
        }
    }
    if (strlen(ibus->displayText) > 0 && ibus->cdStatus > 0x01) {
        uint32_t now = TimerGetMillis();
        if ((now - ibus->displayTextLastStamp) > 1250) {
            if (strlen(ibus->displayText) <= 11) {
                IBusDisplayText(ibus, ibus->displayText);
            } else {
                char text[12];
                text[11] = '\0';
                uint8_t idx;
                uint8_t endOfText = 0;
                uint8_t metadataLength = strlen(ibus->displayText);
                for (idx = 0; idx < 11; idx++) {
                    uint8_t offsetIdx = idx + ibus->displayTextIdx;
                    if (offsetIdx < metadataLength) {
                        text[idx] = c;
                    } else {
                        text[idx] = '\0';
                        endOfText = 1;
                    }
                }
                IBusDisplayText(ibus, text);
                if (endOfText == 1) {
                    ibus->displayTextIdx = 0;
                } else {
                    ibus->displayTextIdx = ibus->displayTextIdx + 11;
                }
            }
            ibus->displayTextLastStamp = now;
        }
    }
}

/**
 * IBusSendCommand()
 *     Description:
 *         Initialize the state of the I-Bus. Useful for doing things like
 *         declaring our
 *     Params:
 *         struct IBus_t *ibus,
 *         const unsigned char src,
 *         const unsigned char dst,
 *         const unsigned char *data
 *     Returns:
 *         void
 */
void IBusSendCommand(
    struct IBus_t *ibus,
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
 *         Initialize the state of the I-Bus. Useful for doing things like
 *         declaring our
 *     Params:
 *         struct IBus_t *ibus
 *     Returns:
 *         void
 */
void IBusStartup(struct IBus_t *ibus)
{
    const unsigned char cdcAlive[] = {0x02, 0x01};
    IBusSendCommand(ibus, IBusDevice_CDC, IBusDevice_LOC, cdcAlive, sizeof(cdcAlive));
    const unsigned char ikeAlive[] = {0x1B, 0x02};
    IBusSendCommand(ibus, IBusDevice_IKE, IBusDevice_RAD, ikeAlive, sizeof(ikeAlive));
    LogDebug("IBus Startup Complete");
}

void IBusDisplayText(struct IBus_t *ibus, char *message)
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
        IBusDevice_TEL,
        IBusDevice_IKE,
        displayText,
        sizeof(displayText)
    );
}

void IBusDisplayTextClear(struct IBus_t *ibus)
{
    const unsigned char displayText[] = {0x23, 0x42, 0x32};
    IBusSendCommand(
        ibus,
        IBusDevice_TEL,
        IBusDevice_IKE,
        displayText,
        sizeof(displayText)
    );
}
