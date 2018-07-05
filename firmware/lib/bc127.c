/*
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the Sierra Wireless BC127 Bluetooth UART API
 */
#define _ADDED_C_LIB 1
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
        UART_BAUD_9600,
        UART_PARITY_NONE
    );
    return bt;
}

/**
 * BC127CommandBackward()
 *     Description:
 *         Go to the next track on the currently selected A2DP device
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127CommandBackward(struct BC127_t *bt)
{
    char command[19];
    snprintf(command, 18, "MUSIC %d BACKWARD\r", bt->selectedDevice);
    LogDebug("BT: Trigger Backward Command");
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandForward()
 *     Description:
 *         Go to the next track on the currently selected A2DP device
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127CommandForward(struct BC127_t *bt)
{
    char command[18];
    snprintf(command, 17, "MUSIC %d FORWARD\r", bt->selectedDevice);
    LogDebug("BT: Trigger Forward Command");
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandPause()
 *     Description:
 *         Pause the currently selected A2DP device
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127CommandPause(struct BC127_t *bt)
{
    char command[16];
    snprintf(command, 15, "MUSIC %d PAUSE\r", bt->selectedDevice);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandPlay()
 *     Description:
 *         Play the currently selected A2DP device
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127CommandPlay(struct BC127_t *bt)
{
    char command[15];
    snprintf(command, 14, "MUSIC %d PLAY\r", bt->selectedDevice);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandStatus()
 *     Description:
 *         Play the currently selected A2DP device
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127CommandStatus(struct BC127_t *bt)
{
    char command[8] = "STATUS\r";
    BC127SendCommand(bt, command);
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
void BC127Process(struct BC127_t *bt, struct IBus_t *ibus)
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
                snprintf(ibus->displayText, 200, "%s - %s on %s", bt->title, bt->artist, bt->album);
                ibus->displayTextIdx = 0;
                LogDebug(
                    "BT: Now Playing: '%s' - '%s' on '%s'",
                    bt->title,
                    bt->artist,
                    bt->album
                );
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PLAY") == 0) {
            bt->avrcpStatus = BC127_AVRCP_STATUS_PLAYING;
            bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            LogDebug("BT: Playing");
            // Fire off Callback
        } else if(strcmp(msgBuf[0], "AVRCP_PAUSE") == 0) {
            bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
            bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            LogDebug("BT: Paused");
            // Fire off Callback
            strncpy(ibus->displayText, "Paused", 199);
            ibus->displayTextIdx = 0;
        } else if(strcmp(msgBuf[0], "CALLER_NUMBER") == 0) {
            uint8_t closedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            if (closedDevice == bt->selectedDevice) {
                bt->selectedDevice = 0;
                // Fire off Callback
                LogDebug("BT: Selected device %s closed connection", msgBuf[1]);
            } else {
                LogDebug("BT: Unselected device %s closed connection", msgBuf[1]);
            }
        } else if(strcmp(msgBuf[0], "LINK") == 0) {
            LogDebug("BT: Got Link for %s -> %s", msgBuf[3], msgBuf[1]);
            if(strcmp(msgBuf[3], "A2DP") == 0) {
                bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            }
        } else if(strcmp(msgBuf[0], "CLOSE_OK") == 0) {
            LogDebug("BT: Connection Closed for ID %s", msgBuf[1]);
            // Fire off Callback
        } else if(strcmp(msgBuf[0], "INFO") == 0) {
            if (strcmp(msgBuf[2], "CONNECTED") == 0 &&
                strcmp(msgBuf[3], "A2DP") == 0
            ) {
                bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
                LogDebug("BT: A2DP connected");
            }
        } else if (strcmp(msgBuf[0], "OPEN_OK") == 0) {
            LogDebug("BT: %s connected on ID %s", msgBuf[2], msgBuf[1]);
            // Fire off Callback
            // Get Device Name?
        }
    }
    if (ibus->playbackStatus != 0) {
        switch (ibus->playbackStatus) {
            case 1:
                BC127CommandPause(bt);
                break;
            case 2:
                BC127CommandPlay(bt);
                break;
            case 3:
                BC127CommandForward(bt);
                break;
            case 4:
                BC127CommandBackward(bt);
                break;
        }
        ibus->playbackStatus = 0;
    }
}

/**
 * BC127SendCommand()
 *     Description:
 *         Send data over UART
 *     Params:
 *         struct BC127_t *
 *         unsigned char *command
 *     Returns:
 *         void
 */
void BC127SendCommand(struct BC127_t *bt, char *command)
{
    unsigned char c;
    while ((c != BC127_MSG_END_CHAR)) {
        c = *command++;
        CharQueueAdd(&bt->uart.txQueue, c);
    }
    // Set the interrupt flag
    SetUARTTXIE(bt->uart.moduleIndex, 1);
}

/**
 * BC127Startup()
 *     Description:
 *         Perform initialization of BC127 module
 *     Params:
 *         struct CharQueue_t *
 *     Returns:
 *         void
 */
void BC127Startup(struct BC127_t *bt)
{
    BC127CommandStatus(bt);
    LogDebug("BC127 Startup Complete");
}
