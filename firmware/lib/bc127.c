/*
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the Sierra Wireless BC127 Bluetooth UART API
 */
#include <stdio.h>
#include "../io_mappings.h"
#include "bc127.h"
#include "debug.h"
#include "uart.h"

/**
 * BC127Init()
 *     Description:
 *         Returns a fresh BC127_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         struct BC127_t *
 */
struct BC127_t BC127Init()
{
    BC127_t bt;
    bt.avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
    bt.newAvrcpMeta = 0;
    bt.uart = UARTInit(
        BC127_UART_MODULE,
        BC127_UART_RX_PIN,
        BC127_UART_TX_PIN,
        UART_BAUD_9600
    );
    return bt;
}

/**
 * BC127Process()
 *     Description:
 *         Read the RX queue and process the messages into meaningful data
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127Process(struct BC127_t *bt)
{
    int size = bt->uart.rxQueue.size;
    if (size > 0) {
        char msg[size + 5];
        int i = 0;
        for (i = 0; i < size; i++) {
            char c = CharQueueNext(&bt->uart.rxQueue);
            msg[i] = c;
            if (c == 0x0D) {
                i++;
                msg[i] = 0x0A;
            }
        }
        LogDebug(msg);
    }
}

/**
 * BC127SendCommand()
 *     Description:
 *         Send data over UART
 *     Params:
 *         struct CharQueue_t *
 *         unsigned char *command
 *     Returns:
 *         void
 */
void BC127SendCommand(struct BC127_t *bt, unsigned char *command)
{
    UARTSendData(&bt->uart, command);
}
