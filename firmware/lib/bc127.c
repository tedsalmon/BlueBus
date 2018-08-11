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
    bt.activeDevice = BC127ConnectionInit();
    bt.pairedDevicesCount = 0;
    // Make sure that we initialize the char arrays to all zeros
    BC127ClearMetadata(&bt);
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
 * BC127ClearMetadata()
 *     Description:
 *        (Re)Initialize the metadata fields to blank
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127ClearMetadata(BC127_t *bt)
{
    memset(bt->activeDevice.title, 0, BC127_METADATA_FIELD_SIZE);
    memset(bt->activeDevice.artist, 0, BC127_METADATA_FIELD_SIZE);
    memset(bt->activeDevice.album, 0, BC127_METADATA_FIELD_SIZE);
}

/**
 * BC127ClearPairedDevices()
 *     Description:
 *        Clear the paired devices list
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127ClearPairedDevices(BC127_t *bt)
{
    uint8_t idx;
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
        BC127PairedDevice_t *btConn = &bt->pairedDevices[idx];
        if (btConn != 0) {
            memset(btConn, 0, sizeof(bt->pairedDevices[idx]));
        }
    }
    bt->pairedDevicesCount = 0;
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
    snprintf(command, 18, "MUSIC %d BACKWARD", bt->activeDevice.avrcpLinkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandClose()
 *     Description:
 *         Close a link ID, device ID or all (255)
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t id - The object to close
 *     Returns:
 *         void
 */
void BC127CommandClose(BC127_t *bt, uint8_t id)
{
    if (id == BC127_CLOSE_ALL) {
        char command[10] = "CLOSE ALL";
        BC127SendCommand(bt, command);
    } else {
        char command[9];
        snprintf(command, 9, "CLOSE %d", id);
        BC127SendCommand(bt, command);
    }
}

/**
 * BC127CommandConnectable()
 *     Description:
 *         Enable / Disable connectable state
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t *connectable - 0 for off and 1 for on
 *     Returns:
 *         void
 */
void BC127CommandConnectable(BC127_t *bt, uint8_t connectable)
{
    if (connectable != bt->connectable) {
        bt->connectable = connectable;
        if (connectable == 1) {
            char command[15] = "CONNECTABLE ON";
            BC127SendCommand(bt, command);
        } else if (connectable == 0) {
            char command[16] = "CONNECTABLE OFF";
            BC127SendCommand(bt, command);
        }
    }
}

/**
 * BC127CommandDiscoverable()
 *     Description:
 *         Enable / Disable Pairing mode
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t *discoverable - 0 for off and 1 for on
 *     Returns:
 *         void
 */
void BC127CommandDiscoverable(BC127_t *bt, uint8_t discoverable)
{
    if (discoverable != bt->discoverable) {
        bt->discoverable = discoverable;
        if (discoverable == 1) {
            char command[16] = "DISCOVERABLE ON";
            BC127SendCommand(bt, command);
        } else if (discoverable == 0) {
            char command[17] = "DISCOVERABLE OFF";
            BC127SendCommand(bt, command);
        }
    }
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
    snprintf(command, 17, "MUSIC %d FORWARD", bt->activeDevice.avrcpLinkId);
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
    snprintf(command, 19, "AVRCP_META_DATA %d", bt->activeDevice.avrcpLinkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandList()
 *     Description:
 *         Request the paired devices list
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandList(BC127_t *bt)
{
    char command[5];
    snprintf(command, 5, "LIST");
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
    snprintf(command, 16, "MUSIC %d PAUSE", bt->activeDevice.avrcpLinkId);
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
    snprintf(command, 15, "MUSIC %d PLAY", bt->activeDevice.avrcpLinkId);
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

/**
 * BC127CommandSetAutoConnect()
 *     Description:
 *         Set the time that the device should remain connectable
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t autoConnect - The time to stay connectable or 0 for forever
 *     Returns:
 *         void
 */
void BC127CommandSetAutoConnect(BC127_t *bt, uint8_t autoConnect)
{
    char command[15];
    snprintf(command, 15, "SET AUTOCONN=%d", autoConnect);
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

/**
 * BC127CommandSetConnTo()
 *     Description:
 *         Set the time that the device should remain connectable
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t value - The time in seconds to stay connectable or 0 for forever
 *     Returns:
 *         void
 */
void BC127CommandSetConnTo(BC127_t *bt, uint8_t value)
{
    char connToCommand[15];
    snprintf(connToCommand, 15, "SET CONN_TO=%d", value);
    BC127SendCommand(bt, connToCommand);
}

/**
 * BC127CommandSetDiscoverable()
 *     Description:
 *         Set the discoverable settings for the device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t mode - The Discoverable mode to set
 *         uint8_t timeout - The time to remain discoverable
 *     Returns:
 *         void
 */
void BC127CommandSetDiscoverable(BC127_t *bt, uint8_t mode, uint8_t timeout)
{
    char command[23];
    snprintf(command, 23, "SET DISCOVERABLE=%d %d", mode, timeout);
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
 * BC127CommandSetProfiles()
 *     Description:
 *         Configuration of the BT profiles. Each value indicates the maximum
 *         number of connections (up to 3).
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandSetProfiles(
    BC127_t *bt,
    uint8_t a2dp,
    uint8_t avrcp,
    uint8_t ble,
    uint8_t hpf
) {
    char command[37];
    snprintf(
        command,
        37,
        "SET PROFILES=%d 0 %d 0 %d %d 1 1 0 0 1 0",
        hpf,
        a2dp,
        avrcp,
        ble
    );
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
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
    char deviceIdStr[2] = {str[0], '\0'};
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
            // Always copy size of buffer minus one to make sure we're always
            // null terminated
            if (strcmp(msgBuf[1], "TITLE:") == 0) {
                // Clear Metadata since we're receiving new data
                BC127ClearMetadata(bt);
                strncpy(
                    bt->activeDevice.title,
                    &msg[BC127_METADATA_TITLE_OFFSET],
                    BC127_METADATA_FIELD_SIZE - 1
                );
            } else if (strcmp(msgBuf[1], "ARTIST:") == 0) {
                strncpy(
                    bt->activeDevice.artist,
                    &msg[BC127_METADATA_ARTIST_OFFSET],
                    BC127_METADATA_FIELD_SIZE - 1
                );
            } else if (strcmp(msgBuf[1], "ALBUM:") == 0) {
                strncpy(
                    bt->activeDevice.album,
                    &msg[BC127_METADATA_ALBUM_OFFSET],
                    BC127_METADATA_FIELD_SIZE - 1
                );
            } else if (strcmp(msgBuf[1], "PLAYING_TIME(MS):") == 0) {
                LogDebug(
                    "BT: title=%s,artist=%s,album=%s",
                    bt->activeDevice.title,
                    bt->activeDevice.artist,
                    bt->activeDevice.album
                );
                EventTriggerCallback(BC127Event_MetadataChange, 0);
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PLAY") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice.deviceId == deviceId) {
                // Clear the Metadata, since this notification comes prior
                // to the new metadata
                BC127ClearMetadata(bt);
                bt->activeDevice.playbackStatus = BC127_AVRCP_STATUS_PLAYING;
                LogDebug("BT: Playing");
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PAUSE") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice.deviceId == deviceId) {
                bt->activeDevice.playbackStatus = BC127_AVRCP_STATUS_PAUSED;
                LogDebug("BT: Paused");
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
        } else if(strcmp(msgBuf[0], "LINK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            uint8_t isNew = 0;
            // No active device is configured
            if (bt->activeDevice.deviceId == 0) {
                bt->activeDevice.deviceId = deviceId;
                strncpy(bt->activeDevice.macId, msgBuf[4], 12);
                char *deviceName = BC127PairedDeviceGetName(bt, msgBuf[4]);
                if (deviceName != 0) {
                    strncpy(bt->activeDevice.deviceName, deviceName, 32);
                } else {
                    BC127CommandGetDeviceName(bt, msgBuf[4]);
                }
                isNew = 1;
            }
            if (bt->activeDevice.deviceId == deviceId) {
                uint8_t linkId = strToInt(msgBuf[1]);
                BC127ConnectionOpenProfile(&bt->activeDevice, msgBuf[3], linkId);
                // Set the playback status
                if (strcmp(msgBuf[3], "AVRCP") == 0) {
                    if (strcmp(msgBuf[5], "PLAYING") == 0) {
                       bt->activeDevice.playbackStatus = BC127_AVRCP_STATUS_PLAYING;
                    } else {
                        bt->activeDevice.playbackStatus = BC127_AVRCP_STATUS_PAUSED;
                    }
                    EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
                }
                LogDebug("BT: Link: %s connected on %s", msgBuf[3], msgBuf[1]);
                EventTriggerCallback(
                    BC127Event_DeviceLinkConnected,
                    (unsigned char *) msgBuf[1]
                );
            }
            if (isNew == 1) {
                EventTriggerCallback(BC127Event_DeviceConnected, 0);
            }
        } else if(strcmp(msgBuf[0], "LIST") == 0) {
            // Request the device name. Note that the name will only be returned
            // if the device is in range
            LogDebug("BT: Paired Device %s", msgBuf[1]);
            BC127CommandGetDeviceName(bt, msgBuf[1]);
        } else if(strcmp(msgBuf[0], "CLOSE_OK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            // If the open connection is closing, update the state
            if (bt->activeDevice.deviceId == deviceId) {
                BC127ConnectionCloseProfile(&bt->activeDevice, msgBuf[2]);
                LogDebug("BT: Closed link %s", msgBuf[1]);
            }
        } else if (strcmp(msgBuf[0], "OPEN_OK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            uint8_t linkId = strToInt(msgBuf[1]);
            if (bt->activeDevice.deviceId != deviceId) {
                bt->activeDevice.deviceId = deviceId;
                strncpy(bt->activeDevice.macId, msgBuf[3], 12);
                char *deviceName = BC127PairedDeviceGetName(bt, msgBuf[3]);
                if (deviceName != 0) {
                    strncpy(bt->activeDevice.deviceName, deviceName, 32);
                } else {
                    BC127CommandGetDeviceName(bt, msgBuf[3]);
                }
                EventTriggerCallback(BC127Event_DeviceConnected, 0);
            }
            BC127ConnectionOpenProfile(&bt->activeDevice, msgBuf[2], linkId);
            LogDebug("BT: Open %s for ID %s", msgBuf[2], msgBuf[1]);
            EventTriggerCallback(
                BC127Event_DeviceLinkConnected,
                (unsigned char *) msgBuf[1]
            );
        } else if (strcmp(msgBuf[0], "OPEN_ERROR") == 0) {
            if (bt->activeDevice.deviceId != 0) {
                BC127CommandProfileOpen(bt, bt->activeDevice.macId, msgBuf[1]);
            }
        } else if (strcmp(msgBuf[0], "NAME") == 0) {
            char deviceName[33];
            uint8_t idx;
            uint8_t strIdx = 0;
            for (idx = 0; idx < strlen(msg) - 19; idx++) {
                char c = msg[idx + 19];
                // 0x22 (") is the character that delimits the device name
                if (c != 0x22) {
                    deviceName[strIdx] = c;
                    strIdx++;
                }
            }
            deviceName[strIdx] = '\0';
            if (strcmp(msgBuf[1], bt->activeDevice.macId) == 0) {
                memset(bt->activeDevice.deviceName, 0, 33);
                strncpy(bt->activeDevice.deviceName, deviceName, 32);
                EventTriggerCallback(BC127Event_DeviceConnected, 0);
            }
            BC127PairedDeviceInit(bt, msgBuf[1], deviceName);
            LogDebug("BT: New Pairing Profile %s -> %s", msgBuf[1], deviceName);
        } else if(strcmp(msgBuf[0], "Ready") == 0) {
            bt->activeDevice = BC127ConnectionInit();
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
 *         Void
 */
void BC127Startup()
{
    EventTriggerCallback(BC127Event_Startup, 0);
}

/** Begin BC127 Paired Device Implementation **/

/**
 * BC127PairedDeviceInit()
 *     Description:
 *         Initialize a pairing profile if one does not exist
 *     Params:
 *         BC127_t *bt
 *         char *macId
 *         char *deviceName
 *     Returns:
 *         Void
 */
void BC127PairedDeviceInit(BC127_t *bt, char *macId, char *deviceName)
{
    uint8_t deviceExists = 0;
    uint8_t idx;
    for (idx = 0; idx <= bt->pairedDevicesCount; idx++) {
        BC127PairedDevice_t *btDevice = &bt->pairedDevices[idx];
        if (strcmp(macId, btDevice->macId) == 0) {
            deviceExists = 1;
        }
    }
    // Create a connection for this device since one does not exist
    if (deviceExists == 0) {
        BC127PairedDevice_t pairedDevice;
        strncpy(pairedDevice.macId, macId, 13);
        memset(pairedDevice.deviceName, 0, 33);
        strncpy(pairedDevice.deviceName, deviceName, 32);
        bt->pairedDevices[bt->pairedDevicesCount++] = pairedDevice;
    }
}

/**
 * BC127PairedDeviceGetName()
 *     Description:
 *         Get the name of a device from its MAC ID
 *     Params:
 *         BC127_t *bt
 *         char *macId
 *     Returns:
 *         char * - A pointer to the device name
 */
char *BC127PairedDeviceGetName(BC127_t *bt, char *macId)
{
    char *deviceName = 0;
    uint8_t idx;
    for (idx = 0; idx <= bt->pairedDevicesCount; idx++) {
        BC127PairedDevice_t *btDevice = &bt->pairedDevices[idx];
        if (strcmp(macId, btDevice->macId) == 0) {
            deviceName = btDevice->deviceName;
        }
    }
    return deviceName;
}

/** Begin BC127 Connection Implementation **/

/**
 * BC127ConnectionInit()
 *     Description:
 *         Returns a fresh BC127Connection_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         BC127Connection_t
 */
BC127Connection_t BC127ConnectionInit()
{
    BC127Connection_t conn;
    memset(conn.macId, 0, 13);
    memset(conn.deviceName, 0, 33);
    conn.playbackStatus = BC127_AVRCP_STATUS_PAUSED;
    conn.deviceId = 0;
    conn.a2dpLinkId = 0;
    conn.avrcpLinkId = 0;
    conn.hfpLinkId = 0;
    return conn;
}

/**
 * BC127ConnectionCloseProfile()
 *     Description:
 *         Closes a profile for the given connection. If all profiles are
 *         closed, it will set the connection to disconnected.
 *     Params:
 *         BC127Connection_t *conn - The connection to update
 *         char *profile - The profile to close
 *     Returns:
 *         None
 */
void BC127ConnectionCloseProfile(BC127Connection_t *conn, char *profile)
{
    if (strcmp(profile, "A2DP") == 0) {
        conn->a2dpLinkId = 0;
    } else if (strcmp(profile, "AVRCP") == 0) {
        conn->avrcpLinkId = 0;
    } else if (strcmp(profile, "HPF") == 0) {
        conn->hfpLinkId = 0;
    }
    // Clear the connection once all the links are closed
    if (conn->a2dpLinkId == 0 && conn->avrcpLinkId == 0 && conn->hfpLinkId == 0) {
        memset(conn->macId, 0, 13);
        memset(conn->deviceName, 0, 33);
        conn->playbackStatus = BC127_AVRCP_STATUS_PAUSED;
        conn->deviceId = 0;
        // Notify the world that the device disconnected
        EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        EventTriggerCallback(BC127Event_DeviceDisconnected, 0);
    }
}

/**
 * BC127ConnectionOpenProfile()
 *     Description:
 *         Opens a profile for the given connection
 *     Params:
 *         BC127Connection_t *conn - The connection to update
 *         char *profile - The profile to open
 *         uint8_t linkId - The link ID for the profile
 *     Returns:
 *         None
 */
void BC127ConnectionOpenProfile(BC127Connection_t *conn, char *profile, uint8_t linkId) {
    if (strcmp(profile, "A2DP") == 0) {
        conn->a2dpLinkId = linkId;
    } else if (strcmp(profile, "AVRCP") == 0) {
        conn->avrcpLinkId = linkId;
    } else if (strcmp(profile, "HPF") == 0) {
        conn->hfpLinkId = linkId;
    }
}
