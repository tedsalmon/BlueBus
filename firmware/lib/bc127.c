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
    bt.activeDevice = 0;
    bt.uart = UARTInit(
        BC127_UART_MODULE,
        BC127_UART_RX_PIN,
        BC127_UART_TX_PIN,
        BC127_UART_RX_PRIORITY,
        BC127_UART_TX_PRIORITY,
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
    char command[18];
    snprintf(command, 18, "MUSIC %d BACKWARD", bt->activeDevice->avrcpLink);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandBtState()
 *     Description:
 *         Configure Discoverable and connectable states temporarily
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t connectable - 0 for off and 1 for on
 *         uint8_t discoverable - 0 for off and 1 for on
 *     Returns:
 *         void
 */
void BC127CommandBtState(BC127_t *bt, uint8_t connectable, uint8_t discoverable)
{
    bt->connectable = connectable;
    bt->discoverable = discoverable;
    char connectMode[4];
    char discoverMode[4];
    if (connectable == 1) {
        strncpy(connectMode, "ON", 4);
    } else if (connectable == 0) {
        strncpy(connectMode, "OFF", 4);
    }
    if (discoverable == 1) {
        strncpy(discoverMode, "ON", 4);
    } else if (discoverable == 0) {
        strncpy(discoverMode, "OFF", 4);
    }
    char command[17];
    snprintf(command, 17, "BT_STATE %s %s", connectMode, discoverMode);
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
    char command[17];
    snprintf(command, 17, "MUSIC %d FORWARD", bt->activeDevice->avrcpLink);
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
    snprintf(command, 19, "AVRCP_META_DATA %d", bt->activeDevice->avrcpLink);
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
    snprintf(command, 16, "MUSIC %d PAUSE", bt->activeDevice->avrcpLink);
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
    snprintf(command, 15, "MUSIC %d PLAY", bt->activeDevice->avrcpLink);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandProfileClose()
 *     Description:
 *         Close a profile for a device using its link ID
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t linkId - The link to terminate
 *     Returns:
 *         void
 */
void BC127CommandProfileClose(BC127_t *bt, uint8_t linkId)
{
    char command[10];
    snprintf(command, 10, "CLOSE %d", linkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandProfileOpen()
 *     Description:
 *         Open a profile for a given device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *macId - The MAC ID of the device for which to open the profile
 *         char *profile - The Profile type to open
 *     Returns:
 *         void
 */
void BC127CommandProfileOpen(BC127_t *bt, char *macId, char *profile)
{
    char command[24];
    snprintf(command, 24, "OPEN %s %s", macId, profile);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandReset()
 *     Description:
 *         Send the RESET command to reboot our device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandReset(BC127_t *bt)
{
    char command[6] = "RESET";
    BC127SendCommand(bt, command);
}

void BC127CommandSetAutoConnect(BC127_t *bt, uint8_t autoConnect)
{
    char command[15];
    snprintf(command, 15, "SET AUTOCONN=%d", autoConnect);
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

/**
 * BC127CommandSetBtState()
 *     Description:
 *         Set the discoverable / connectable settings for the device
 *         <connectable_mode> / <discoverable_mode>
 *             0 - (Default) Not connectable / discoverable at power-on
 *             1 - Always connectable  / discoverable
 *             2 - Connectable  / discoverable at power-on
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t connectMode - The discoverability mode to set
 *         uint8_t discoverMode - The connectability mode to set
 *     Returns:
 *         void
 */
void BC127CommandSetBtState(
    BC127_t *bt,
    char *connectMode,
    char *discoverMode
) {
    char command[24];
    snprintf(command, 24, "SET BT_STATE_CONFIG=%s %s", connectMode, discoverMode);
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

void BC127CommandSetMetadata(BC127_t *bt, uint8_t value)
{
    if (value == 0) {
        char command[24] = "SET MUSIC_META_DATA=OFF";
        BC127SendCommand(bt, command);
    } else if (value == 1) {
        char command[23] = "SET MUSIC_META_DATA=ON";
        BC127SendCommand(bt, command);
    }
    BC127CommandWrite(bt);
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
    BC127CommandWrite(bt);
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
 * BC127CommandWrite()
 *     Description:
 *         Send the WRITE command to save our settings
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandWrite(BC127_t *bt)
{
    char command[6] = "WRITE";
    BC127SendCommand(bt, command);
}

/**
 * BC127GetDeviceId()
 *     Description:
 *         Parse the device ID from the given link ID
 *     Params:
 *         char *str - The link ID to return a device ID for
 *     Returns:
 *         uint8_t - The device ID
 */
uint8_t BC127GetDeviceId(char *str)
{
    char deviceIdStr[1];
    strncpy(deviceIdStr, str, 1);
    return strToInt(deviceIdStr);
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
    uint8_t messageLength = CharQueueSeek(
        &bt->uart.rxQueue,
        BC127_MSG_END_CHAR
    );
    if (messageLength > 0) {
        char msg[messageLength];
        uint8_t i;
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
        UARTSetModuleState(&bt->uart, UART_STATE_IDLE);
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

        if (strcmp(msgBuf[0], "AVRCP_MEDIA") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice != 0 && bt->activeDevice->deviceId == deviceId) {
                if (strcmp(msgBuf[2], "TITLE:") == 0) {
                    strncpy(
                        bt->title,
                        &msg[BC127_METADATA_TITLE_OFFSET],
                        BC127_METADATA_FIELD_SIZE
                    );
                } else if (strcmp(msgBuf[2], "ARTIST:") == 0) {
                    strncpy(
                        bt->artist,
                        &msg[BC127_METADATA_ARTIST_OFFSET],
                        BC127_METADATA_FIELD_SIZE
                    );
                } else if (strcmp(msgBuf[2], "ALBUM:") == 0) {
                    strncpy(
                        bt->album,
                        &msg[BC127_METADATA_ALBUM_OFFSET],
                        BC127_METADATA_FIELD_SIZE
                    );
                    LogDebug(
                        "BT: title=%s,artist=%s,album=%s",
                        bt->title,
                        bt->artist,
                        bt->album
                    );
                    EventTriggerCallback(BC127Event_MetadataChange, 0);
                }
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PLAY") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice->deviceId != deviceId) {
                bt->activeDevice = &bt->connections[deviceId - 1];
            }
            bt->avrcpStatus = BC127_AVRCP_STATUS_PLAYING;
            LogDebug("BT: Playing");
            EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        } else if(strcmp(msgBuf[0], "AVRCP_PAUSE") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice->deviceId != deviceId) {
                bt->activeDevice = &bt->connections[deviceId - 1];
            }
            bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
            LogDebug("BT: Paused");
            EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        } else if(strcmp(msgBuf[0], "CALLER_NUMBER") == 0) {
            // HPF Call
        } else if(strcmp(msgBuf[0], "LINK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            uint8_t linkId = strToInt(msgBuf[1]);
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[4], deviceId);
            BC127ConnectionOpenProfile(conn, msgBuf[3], linkId);
            // Set the default device to the first connected one
            if (bt->activeDevice == 0) {
                bt->activeDevice = conn;
            }
            // If this is the selected AVRCP device, set the playback status
            if (strcmp(msgBuf[3], "AVRCP") == 0 &&
                bt->activeDevice->deviceId == deviceId
            ) {
                if (strcmp(msgBuf[5], "PLAYING") == 0) {
                    bt->avrcpStatus = BC127_AVRCP_STATUS_PLAYING;
                } else {
                    bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
                }
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
            LogDebug("BT: Status - %s connected on link %s", msgBuf[3], msgBuf[1]);
        } else if(strcmp(msgBuf[0], "CLOSE_OK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            // If the open connection is closing, update the state
            if (bt->activeDevice->deviceId == deviceId) {
                bt->activeDevice = 0;
            }
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[3], deviceId);
            BC127ConnectionCloseProfile(conn, msgBuf[2]);
            LogDebug("BT: Connection Closed for link %s", msgBuf[1]);
            // Fire off Callback
        } else if (strcmp(msgBuf[0], "OPEN_OK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            uint8_t linkId = strToInt(msgBuf[1]);
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[3], deviceId);
            BC127ConnectionOpenProfile(conn, msgBuf[2], linkId);
            // Set the default device to the first connected one
            if (bt->activeDevice == 0) {
                bt->activeDevice = conn;
            }
            LogDebug("BT: %s connected on ID %s", msgBuf[2], msgBuf[1]);
        } else if (strcmp(msgBuf[0], "NAME") == 0) {
            // It's okay to pass 0 for the device ID, since it should exist already
            BC127Connection_t *conn = BC127ConnectionGet(bt, msgBuf[1], 0);
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
            strncpy(conn->deviceName, deviceName, 33);
            LogDebug("BT: Got Device Name %s for Device %d", conn->deviceName, conn->deviceId);
            //EventTriggerCallback(BC127Event_DeviceConnected, conn->deviceId);
        } else if(strcmp(msgBuf[0], "Ready") == 0) {
            bt->activeDevice = 0;
            bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
            LogDebug("BT: Ready");
            EventTriggerCallback(BC127Event_DeviceReady, 0);
            EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        } else if (strcmp(msgBuf[0], "STATE") == 0) {
            if (strcmp(msgBuf[2], "CONNECTABLE[ON]") == 0) {
                bt->connectable = BC127_STATE_ON;
            } else if (strcmp(msgBuf[2], "CONNECTABLE[OFF]") == 0) {
                bt->connectable = BC127_STATE_OFF;
            }
            if (strcmp(msgBuf[3], "DISCOVERABLE[ON]") == 0) {
                bt->discoverable = BC127_STATE_ON;
            } else if (strcmp(msgBuf[3], "DISCOVERABLE[OFF]") == 0) {
                bt->discoverable = BC127_STATE_OFF;
            }
            LogDebug("BT: Got Status %s %s", msgBuf[2], msgBuf[3]);
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
    LogDebug("BT: Send Command '%s'", command);
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
 *         uint8_t deviceId - The device ID
 *     Returns:
 *         BC127Connection_t *
 */
BC127Connection_t BC127ConnectionInit(char *macId, uint8_t deviceId)
{
    BC127Connection_t conn;
    strncpy(conn.macId, macId, 13);
    conn.deviceId = deviceId;
    conn.a2dpLink = 0;
    conn.avrcpLink = 0;
    conn.hfpLink = 0;
    conn.state = BC127_CONN_STATE_CONNECTED;
    return conn;
}

/**
 * BC127ConnectionGet()
 *     Description:
 *          Get a connected device by its MAC ID. This method will create a
 *          connection based on the data given if one does not exist
 *     Params:
 *         BC127_t *bt
 *         char *macId
 *         uint8_t deviceId
 *     Returns:
 *         BC127Connection_t *
 */
BC127Connection_t *BC127ConnectionGet(BC127_t *bt, char *macId, uint8_t deviceId)
{
    BC127Connection_t *conn = 0;
    uint8_t idx;
    for (idx = 0; idx <= BC127_MAX_DEVICE_CONN; idx++) {
        BC127Connection_t *btConn = &bt->connections[idx];
        if (strcmp(macId, btConn->macId) == 0) {
            conn = btConn;
        }
    }
    // Create a connection for this device if it's available
    if (conn == 0 && deviceId != 0) {
        BC127Connection_t newConn = BC127ConnectionInit(macId, deviceId);
        memset(newConn.deviceName, 0, 33);
        strncpy(newConn.macId, macId, 13);
        bt->connections[deviceId - 1] = newConn;
        conn = &bt->connections[deviceId - 1];
        conn->deviceId = deviceId;
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
    if (strcmp(profile, "A2DP") == 0) {
        btConn->a2dpLink = linkId;
    } else if (strcmp(profile, "AVRCP") == 0) {
        btConn->avrcpLink = linkId;
    } else if (strcmp(profile, "HPF") == 0) {
        btConn->hfpLink = linkId;
    }
}
