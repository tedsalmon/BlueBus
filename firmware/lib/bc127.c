/*
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the Sierra Wireless BC127 Bluetooth UART API
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../io_mappings.h"
#include "bc127.h"
#include "debug.h"
#include "uart.h"
#include "utils.h"

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
    uint16_t messageLength = CharQueueSeek(
        &bt->uart.rxQueue,
        BC127_MSG_END_CHAR
    );
    if (messageLength > 0) {
        char msg[messageLength];
        uint16_t i;
        uint8_t delimCount = 1;
        for (i = 0; i < messageLength; i++) {
            char c = CharQueueNext(&bt->uart.rxQueue);
            if (c == BC127_MSG_DELIMETER) {
                delimCount++;
            }
            if (c != BC127_MSG_END_CHAR) {
                msg[i] = c;
            } else {
                // The protocol states that 0x0D delimits messages,
                // so we change it to a null terminator instead
                msg[i] = '\0';
            }
        }
        // Copy the message, since strtok adds a null terminator after the first
        // occurence of the delimiter, causes issues with any functions used going forward
        char tmpMsg[messageLength];
        strcpy(tmpMsg, msg);
        char *msgBuf[delimCount];
        char *p = strtok(tmpMsg, " ");
        i = 0;
        while (p != NULL) {
            msgBuf[i++] = p;
            p = strtok(NULL, " ");
        }
        char *ptr;
        if (strcmp(msgBuf[0], "AVRCP_MEDIA") == 0) {
            // The strncpy call adds a null terminator, so send size_t - 1
            if (strcmp(msgBuf[1], "TITLE:") == 0) {
                removeSubstring(msg, "AVRCP_MEDIA TITLE: ");
                strncpy(bt->title, msg, BC127_METADATA_FIELD_SIZE - 1);
                // We have new metadata, so clear the old
                memset(&bt->artist, 0, BC127_METADATA_FIELD_SIZE);
                memset(&bt->album, 0, BC127_METADATA_FIELD_SIZE);
            } else if (strcmp(msgBuf[1], "ARTIST:") == 0) {
                removeSubstring(msg, "AVRCP_MEDIA ARTIST: ");
                strncpy(bt->artist, msg, BC127_METADATA_FIELD_SIZE - 1);
            } else if (strcmp(msgBuf[1], "ALBUM:") == 0) {
                removeSubstring(msg, "AVRCP_MEDIA ALBUM: ");
                strncpy(bt->album, msg, BC127_METADATA_FIELD_SIZE - 1);
            } else if (strcmp(msgBuf[1], "PLAYING_TIME(MS):") == 0) {
                char nowPlaying[256];
                sprintf(
                    nowPlaying,
                    "BT: Now Playing: '%s' - '%s' on '%s'",
                    bt->title,
                    bt->artist,
                    bt->album
                );
                LogDebug(nowPlaying);
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PLAY") == 0) {
            bt->avrcpStatus = BC127_AVRCP_STATUS_PLAYING;
            bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            LogDebug("BT: Playing");
        } else if(strcmp(msgBuf[0], "AVRCP_PAUSE") == 0) {
            bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
            bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            LogDebug("BT: Paused");
        } else if(strcmp(msgBuf[0], "INFO") == 0) {
            if (strcmp(msgBuf[2], "CONNECTED") == 0 &&
                strcmp(msgBuf[3], "A2DP") == 0
            ) {
                bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
                LogDebug("BT: A2DP connected");
            }
        }
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
