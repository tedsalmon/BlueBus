/*
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the Sierra Wireless BC127 Bluetooth UART API
 */
#include "bc127.h"

/**
 * BC127Init()
 *     Description:
 *         Returns a fresh BC127_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         BC127_t *
 */
BC127_t BC127Init()
{
    BC127_t bt;
    bt.avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
    bt.selectedDevice = 0;
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
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandBackward(BC127_t *bt)
{
    char command[19];
    snprintf(command, 19, "MUSIC %d BACKWARD", bt->selectedDevice);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandForward()
 *     Description:
 *         Go to the next track on the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandForward(BC127_t *bt)
{
    char command[18];
    snprintf(command, 18, "MUSIC %d FORWARD", bt->selectedDevice);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandGetDeviceName()
 *     Description:
 *         Go to the next track on the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *macId - The MAC ID of the device to get the name for
 *     Returns:
 *         void
 */
void BC127CommandGetDeviceName(BC127_t *bt, char *macId)
{
    char command[18];
    snprintf(command, 18, "NAME %s", macId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandGetMetadata()
 *     Description:
 *         Request the metadata for the track playing
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandGetMetadata(BC127_t *bt)
{
    char command[19];
    snprintf(command, 19, "AVRCP_META_DATA %d", bt->selectedDevice);
    LogDebug("Request Metadata: %s", command);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandPause()
 *     Description:
 *         Pause the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandPause(BC127_t *bt)
{
    char command[16];
    snprintf(command, 16, "MUSIC %d PAUSE", bt->selectedDevice);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandPlay()
 *     Description:
 *         Play the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandPlay(BC127_t *bt)
{
    char command[15];
    snprintf(command, 15, "MUSIC %d PLAY", bt->selectedDevice);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandSetModuleName()
 *     Description:
 *         Set the name and short name for the BT device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *name - A friendly name to set the module to
 *     Returns:
 *         void
 */
void BC127CommandSetModuleName(BC127_t *bt, char *name)
{
    char nameSetCommand[42];
    snprintf(nameSetCommand, 42, "SET NAME=%s", name);
    BC127SendCommand(bt, nameSetCommand);
    // Set the "short" name
    char nameShortSetCommand[23];
    char shortName[8];
    strncpy(shortName, name, BC127_SHORT_NAME_MAX_LEN);
    snprintf(nameShortSetCommand, 23, "SET NAME_SHORT=%s", shortName);
    BC127SendCommand(bt, nameShortSetCommand);
}

/**
 * BC127CommandSetPin()
 *     Description:
 *         Set the pairing pin for the device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *pin - The pin number to set for the device
 *     Returns:
 *         void
 */
void BC127CommandSetPin(BC127_t *bt, char *pin)
{
    char command[13];
    snprintf(command, 13, "SET PIN=%s", pin);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandStatus()
 *     Description:
 *         Play the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandStatus(BC127_t *bt)
{
    char command[8] = "STATUS";
    BC127SendCommand(bt, command);
}

/**
 * BC127Connectable()
 *     Description:
 *         Enable / Disable connectable state
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t connectable - 0 for off and 1 for on
 *     Returns:
 *         void
 */
void BC127Connectable(BC127_t *bt, uint8_t connectable)
{
    if (connectable == 1) {
        char command[15] = "CONNECTABLE ON";
        BC127SendCommand(bt, command);
    } else if (connectable == 0) {
        char command[16] = "CONNECTABLE OFF";
        BC127SendCommand(bt, command);
    }
}

/**
 * BC127Discoverable()
 *     Description:
 *         Enable / Disable Pairing mode
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t discoverable - 0 for off and 1 for on
 *     Returns:
 *         void
 */
void BC127Discoverable(BC127_t *bt, uint8_t discoverable)
{
    if (discoverable == 1) {
        char command[16] = "DISCOVERABLE ON";
        BC127SendCommand(bt, command);
    } else if (discoverable == 0) {
        char command[17] = "DISCOVERABLE OFF";
        BC127SendCommand(bt, command);
    }
}

/**
 * BC127Process()
 *     Description:
 *         Read the RX queue and process the messages into meaningful data
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127Process(BC127_t *bt)
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
                strncpy(bt->title, msg, BC127_METADATA_TITLE_FIELD_SIZE - 1);
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
                LogDebug(
                    "BT: title=%s,artist=%s,album=%s",
                    bt->title,
                    bt->artist,
                    bt->album
                );
                EventTriggerCallback(BC127Event_MetadataChange, 0);
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PLAY") == 0) {
            bt->avrcpStatus = BC127_AVRCP_STATUS_PLAYING;
            bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            LogDebug("BT: Playing");
            EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        } else if(strcmp(msgBuf[0], "AVRCP_PAUSE") == 0) {
            bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
            bt->selectedDevice = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            LogDebug("BT: Paused");
            EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        } else if(strcmp(msgBuf[0], "CALLER_NUMBER") == 0) {
            // HPF Call
        } else if(strcmp(msgBuf[0], "LINK") == 0) {
            uint8_t linkId = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[4]);
            BC127ConnectionOpenProfile(conn, msgBuf[3], linkId);
            // Set the default device to the first connected one
            if (conn->avrcpLink != 0 && bt->selectedDevice == 0) {
                bt->selectedDevice = conn->avrcpLink;
            }
            // If this is the selected AVRCP device, set the playback status
            if (strcmp(msgBuf[3], "AVRCP") == 0 &&
                bt->selectedDevice == conn->avrcpLink
            ) {
                if (strcmp(msgBuf[5], "PLAYING") == 0) {
                    bt->avrcpStatus = BC127_AVRCP_STATUS_PLAYING;
                } else {
                    bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
                }
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
            LogDebug("BT: Status - %s connected on ID %s", msgBuf[3], msgBuf[1]);
        } else if(strcmp(msgBuf[0], "CLOSE_OK") == 0) {
            uint8_t linkId = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            // If the open connection is closing, update the state
            if (linkId == bt->selectedDevice) {
                bt->selectedDevice = 0;
            }
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[3]);
            BC127ConnectionCloseProfile(conn, msgBuf[2]);
            LogDebug("BT: Connection Closed for ID %s", msgBuf[1]);
            // Fire off Callback
        } else if (strcmp(msgBuf[0], "OPEN_OK") == 0) {
            uint8_t linkId = (uint8_t)strtol(msgBuf[1], &ptr, 10);
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[3]);
            BC127ConnectionOpenProfile(conn, msgBuf[2], linkId);
            // Set the default device to the first connected one
            if (conn->avrcpLink != 0 && bt->selectedDevice == 0) {
                bt->selectedDevice = conn->avrcpLink;
            }
            LogDebug("BT: %s connected on ID %s", msgBuf[2], msgBuf[1]);
            // Fire off Callback
            EventTriggerCallback(BC127Event_DeviceConnected, (unsigned char *) msgBuf[3]);
        } else if (strcmp(msgBuf[0], "NAME") == 0) {
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[1]);
            char deviceName[33];
            // Clear out the stack
            memset(deviceName, 0, 33);
            uint8_t idx;
            uint8_t nameLen = strlen(msg) - 19;
            for (idx = 0; idx < nameLen; idx++) {
                char c = msg[idx + 19];
                if (c != 0x22) {
                    deviceName[idx] = msg[idx + 19];
                }
            }
            LogDebug("BT: Got Device Name %s", deviceName);
            strncpy(conn->deviceName, deviceName, 33);
        }
    }
}

/**
 * BC127SendCommand()
 *     Description:
 *         Send data over UART
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *command - A command to send, with null termination included
 *     Returns:
 *         void
 */
void BC127SendCommand(BC127_t *bt, char *command)
{
    uint8_t idx = 0;
    for (idx = 0; idx < strlen(command); idx++) {
        CharQueueAdd(&bt->uart.txQueue, command[idx]);
    }
    CharQueueAdd(&bt->uart.txQueue, BC127_MSG_END_CHAR);
    // Set the interrupt flag
    SetUARTTXIE(bt->uart.moduleIndex, 1);
}

/**
 * BC127Startup()
 *     Description:
 *         Trigger the callbacks listening for the BC127 Startup
 *     Params:
 *         None
 *     Returns:
 *         void
 */
void BC127Startup()
{
    EventTriggerCallback(BC127Event_Startup, 0);
}

/** Begin BC127 Connection Implementation **/

/**
 * BC127ConnectionInit()
 *     Description:
 *         Returns a fresh BC127_t object to the caller
 *     Params:
 *         char macId - MAC Address
 *     Returns:
 *         BC127Connection_t *
 */
BC127Connection_t BC127ConnectionInit(char *macId)
{
    BC127Connection_t conn;
    strncpy(conn.macId, macId, 13);
    conn.a2dpLink = 0;
    conn.avrcpLink = 0;
    conn.hfpLink = 0;
    conn.state = BC127_CONN_STATE_NEW;
    return conn;
}

/**
 * BC127ConnectionInit()
 *     Description:
 *         Searches for the MAC ID in the list of connected devices. If one is
 *         not found, then it will create a new one, and return it.
 *     Params:
 *         BC127_t *bt
 *         char macId - MAC Address to look for
 *     Returns:
 *         BC127Connection_t *
 */
BC127Connection_t *BC127ConnectionGet(BC127_t *bt, char *macId)
{
    BC127Connection_t *conn = 0;
    uint8_t idx;
    for (idx = 0; idx < bt->connectionsCount; idx++) {
        BC127Connection_t btConn = bt->connections[idx];
        if (strcmp(macId, btConn.macId) == 0) {
            conn = &btConn;
        }
    }
    // A connection state does not exist for this device
    if (conn == 0) {
        BC127Connection_t newConn = BC127ConnectionInit(macId);
        memset(newConn.deviceName, 0, 33);
        strncpy(newConn.macId, macId, 13);
        bt->connections[bt->connectionsCount] = newConn;
        conn = &bt->connections[bt->connectionsCount];
        bt->connectionsCount++;
        BC127CommandGetDeviceName(bt, macId);
    }
    return conn;
}

/**
 * BC127ConnectionCloseProfile()
 *     Description:
 *         Closes a profile for the given connection. If all profiles are
 *         closed, it will set the connection to disconnected.
 *     Params:
 *         BC127Connection_t *btConn - The connection to update
 *         char *profile - The profile to close
 *     Returns:
 *         None
 */
void BC127ConnectionCloseProfile(BC127Connection_t *btConn, char *profile)
{
    if (strcmp(profile, "A2DP") == 0) {
        btConn->a2dpLink = 0;
    } else if (strcmp(profile, "AVRCP") == 0) {
        btConn->avrcpLink = 0;
    } else if (strcmp(profile, "HPF") == 0) {
        btConn->hfpLink = 0;
    }
    // Set the state to disconnected if all profiles are disconnected
    if (btConn->a2dpLink == 0 && btConn->avrcpLink == 0 && btConn->hfpLink == 0) {
        btConn->state = BC127_CONN_STATE_DISCONNECTED;
    }
}

/**
 * BC127ConnectionOpenProfile()
 *     Description:
 *         Opens a profile for the given connection
 *     Params:
 *         BC127Connection_t *btConn - The connection to update
 *         char *profile - The profile to open
 *         uint8_t linkId - The link ID for the profile
 *     Returns:
 *         None
 */
void BC127ConnectionOpenProfile(
    BC127Connection_t *btConn,
    char *profile,
    uint8_t linkId
) {
    btConn->state = BC127_CONN_STATE_CONNECTED;
    if (strcmp(profile, "A2DP") == 0) {
        btConn->a2dpLink = linkId;
    } else if (strcmp(profile, "AVRCP") == 0) {
        btConn->avrcpLink = linkId;
    } else if (strcmp(profile, "HPF") == 0) {
        btConn->hfpLink = linkId;
    }
}
