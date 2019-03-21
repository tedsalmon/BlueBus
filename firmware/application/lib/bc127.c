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
    bt.connectable = BC127_STATE_ON;
    bt.discoverable = BC127_STATE_ON;
    bt.callStatus = BC127_CALL_INACTIVE;
    bt.metadataStatus = BC127_METADATA_STATUS_NEW;
    bt.pairedDevicesCount = 0;
    memset(bt.pairingErrors, 0, sizeof(bt.pairingErrors));
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
    memset(bt->title, 0, BC127_METADATA_FIELD_SIZE);
    memset(bt->artist, 0, BC127_METADATA_FIELD_SIZE);
    memset(bt->album, 0, BC127_METADATA_FIELD_SIZE);
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
    memset(bt->pairingErrors, 0, sizeof(bt->pairingErrors));
    bt->pairedDevicesCount = 0;
}

/**
 * BC127ClearInactivePairedDevices()
 *     Description:
 *        Clear the paired devices list
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127ClearInactivePairedDevices(BC127_t *bt)
{
    uint8_t idx;
    uint8_t pairedDevices = bt->pairedDevicesCount;
    bt->pairedDevicesCount = 0;
    for (idx = 0; idx < pairedDevices; idx++) {
        BC127PairedDevice_t *btConn = &bt->pairedDevices[idx];
        if (btConn != 0) {
            if (strcmp(btConn->macId, bt->activeDevice.macId) == 0) {
                // Move the connected devices to the first index, unless it's
                // already there
                if (idx != 0) {
                    BC127PairedDeviceInit(bt, btConn->macId, btConn->deviceName);
                }
            } else {
                memset(btConn, 0, sizeof(bt->pairedDevices[idx]));
            }
        }
    }
    memset(bt->pairingErrors, 0, sizeof(bt->pairingErrors));
}

/**
 * BC127ClearPairingErrors()
 *     Description:
 *        Clear the pairing error list
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127ClearPairingErrors(BC127_t *bt)
{
    memset(bt->pairingErrors, 0, sizeof(bt->pairingErrors));
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
 * BC127CommandBackwardSeekPress()
 *     Description:
 *         Seek backwards on the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandBackwardSeekPress(BC127_t *bt)
{
    char command[19];
    snprintf(command, 19, "MUSIC %d REW_PRESS", bt->activeDevice.avrcpLinkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandBackwardSeekRelease()
 *     Description:
 *         Stop seeking backwards on the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandBackwardSeekRelease(BC127_t *bt)
{
    char command[21];
    snprintf(command, 21, "MUSIC %d REW_RELEASE", bt->activeDevice.avrcpLinkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandCallAnswer()
 *     Description:
 *         Answer the incoming call
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandCallAnswer(BC127_t *bt)
{
    char command[15];
    snprintf(command, 15, "CALL %d ANSWER", bt->activeDevice.hfpLinkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandCallAnswer()
 *     Description:
 *         End the active call
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandCallEnd(BC127_t *bt)
{
    char command[12];
    snprintf(command, 12, "CALL %d END", bt->activeDevice.hfpLinkId);
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandCallReject()
 *     Description:
 *         Reject the incoming call
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandCallReject(BC127_t *bt)
{
    char command[15];
    snprintf(command, 15, "CALL %d REJECT", bt->activeDevice.hfpLinkId);
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
    if (bt->activeDevice.avrcpLinkId != 0) {
        char command[17];
        snprintf(command, 17, "MUSIC %d FORWARD", bt->activeDevice.avrcpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to FORWARD - AVRCP link unopened");
    }
}

/*
 * BC127CommandForwardSeekPress()
 *     Description:
 *         Seek forward on the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandForwardSeekPress(BC127_t *bt)
{
    if (bt->activeDevice.avrcpLinkId != 0) {
        char command[18];
        snprintf(command, 18, "MUSIC %d FF_PRESS", bt->activeDevice.avrcpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to SEEK FORWARD - AVRCP link unopened");
    }
}

/*
 * BC127CommandForwardSeekRelease()
 *     Description:
 *         Stop seeking forward on the currently selected A2DP device
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandForwardSeekRelease(BC127_t *bt)
{
    if (bt->activeDevice.avrcpLinkId != 0) {
        char command[20];
        snprintf(command, 20, "MUSIC %d FF_RELEASE", bt->activeDevice.avrcpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to SEEK FORWARD - AVRCP link unopened");
    }
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
    if (bt->activeDevice.avrcpLinkId != 0) {
        char command[19];
        snprintf(command, 19, "AVRCP_META_DATA %d", bt->activeDevice.avrcpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to get Metadata - AVRCP link unopened");
    }
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
    if (bt->activeDevice.avrcpLinkId != 0) {
        char command[16];
        snprintf(command, 16, "MUSIC %d PAUSE", bt->activeDevice.avrcpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to PAUSE - AVRCP link unopened");
    }
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
    if (bt->activeDevice.avrcpLinkId != 0) {
        char command[15];
        snprintf(command, 15, "MUSIC %d PLAY", bt->activeDevice.avrcpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to PLAY - AVRCP link unopened");
    }
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
 *         Open a profile for a given device. Set the given MAC ID as the active
 *         device so we can reference it in case we get OPEN_ERROR's
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
    strncpy(bt->activeDevice.macId, macId, 12);
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
 * BC127CommandSetAudio()
 *     Description:
 *         Set the audio parameters
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t input - Input Analog / Digital (0 = Analog)
 *         uint8_t output - Output Analog / Digital (0 = Analog)
 *     Returns:
 *         void
 */
void BC127CommandSetAudio(BC127_t *bt, uint8_t input, uint8_t output) {
    char command[14];
    snprintf(command, 14, "SET AUDIO=%d %d", input, output);
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

/**
 * BC127CommandSetAudioAnalog()
 *     Description:
 *         Set the analog audio parameters
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *inputGain - The microphone/line in gain. Default: 15
 *         char *outputGain - The output gain. Default: 15
 *         char *micBias - The Mic Bias setting. Default On with audio (1)
 *         char *enablePreamp - Enable a 20dB gain on the input. Default: Off
 *     Returns:
 *         void
 */
void BC127CommandSetAudioAnalog(
    BC127_t *bt,
    char *inputGain,
    char *outputGain,
    char *micBias,
    char *enablePreamp
) {
    char command[29];
    snprintf(
        command,
        29,
        "SET AUDIO_ANALOG=%s %s %s %s",
        inputGain,
        outputGain,
        micBias,
        enablePreamp
    );
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

/**
 * BC127CommandSetAudioDigital()
 *     Description:
 *         Set the digital audio parameters
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         char *format - The format to pass over (0, 1, 2)
 *         char *rate - The rate in khz
 *         char *p1 - 12S - BCLK, PCM - Master Clk, SPDIF - N/A
 *         char *p2 - Mode format
 *     Returns:
 *         void
 */
void BC127CommandSetAudioDigital(
    BC127_t *bt,
    char *format,
    char *rate,
    char *p1,
    char *p2
) {
    char command[40];
    snprintf(command, 40, "SET AUDIO_DIGITAL=%s %s %s %s OFF", format, rate, p1, p2);
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

/**
 * BC127CommandSetAutoConnect()
 *     Description:
 *         Enable / Disable auto connection at power-on
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *         uint8_t autoConnect - Set the parameter to on or off
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
    uint8_t connectMode,
    uint8_t discoverMode
) {
    char command[24];
    snprintf(command, 24, "SET BT_STATE_CONFIG=%d %d", connectMode, discoverMode);
    BC127SendCommand(bt, command);
    BC127CommandWrite(bt);
}

 /**
  * BC127CommandSetCodec()
  *     Description:
  *         Set the codec confiuration value
  *     Params:
  *         BC127_t *bt - A pointer to the module object
  *         uint8_t bitmask - The codec bitmask
  *        char *talkback - The A2DP Talk back mode (on / off)
  *     Returns:
  *         void
  */
void BC127CommandSetCodec(BC127_t *bt, uint8_t bitmask, char *talkback) {
    char command[17];
    snprintf(command, 17, "SET CODEC=%d %s", bitmask, talkback);
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
    uint8_t hfp
) {
    char command[37];
    snprintf(
        command,
        37,
        "SET PROFILES=%d 0 %d 0 %d %d 1 1 0 0 1 0",
        hfp,
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
 *         Get the BC127 connectivity status
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandStatus(BC127_t *bt)
{
    char command[7] = "STATUS";
    BC127SendCommand(bt, command);
}

/**
 * BC127CommandToggleVR()
 *     Description:
 *         Toggle the voice recognition agent
 *     Params:
 *         BC127_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BC127CommandToggleVR(BC127_t *bt)
{
    if (bt->activeDevice.hfpLinkId != 0) {
        char command[13];
        snprintf(command, 16, "TOGGLE_VR %d", bt->activeDevice.hfpLinkId);
        BC127SendCommand(bt, command);
    } else {
        LogWarning("BT: Unable to TOGGLE_VR - HFP link unopened");
    }
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
void BC127CommandUnpair(BC127_t *bt)
{
    char command[7] = "UNPAIR";
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
    uint8_t messageLength = CharQueueSeek(&bt->uart.rxQueue, BC127_MSG_END_CHAR);
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
        LogDebug(LOG_SOURCE_BT, "BT: %s", msg);

        if (strcmp(msgBuf[0], "AVRCP_MEDIA") == 0) {
            // Always copy size of buffer minus one to make sure we're always
            // null terminated
            if (strcmp(msgBuf[2], "TITLE:") == 0) {
                // Clear Metadata since we're receiving new data
                BC127ClearMetadata(bt);
                bt->metadataStatus = BC127_METADATA_STATUS_NEW;
                strncpy(
                    bt->title,
                    &msg[BC127_METADATA_TITLE_OFFSET],
                    BC127_METADATA_FIELD_SIZE - 1
                );
            } else if (strcmp(msgBuf[2], "ARTIST:") == 0) {
                strncpy(
                    bt->artist,
                    &msg[BC127_METADATA_ARTIST_OFFSET],
                    BC127_METADATA_FIELD_SIZE - 1
                );
            } else {
                if (strcmp(msgBuf[2], "ALBUM:") == 0) {
                    strncpy(
                        bt->album,
                        &msg[BC127_METADATA_ALBUM_OFFSET],
                        BC127_METADATA_FIELD_SIZE - 1
                    );
                }
                if (bt->metadataStatus == BC127_METADATA_STATUS_NEW) {
                    LogDebug(LOG_SOURCE_BT, 
                        "BT: title=%s,artist=%s,album=%s",
                        bt->title,
                        bt->artist,
                        bt->album
                    );
                    EventTriggerCallback(BC127Event_MetadataChange, 0);
                    // Setting this flag in either event prevents us from
                    // potentially spamming the BC127 with metadata requests
                    bt->metadataStatus = BC127_METADATA_STATUS_CUR;
                }
            }
            bt->metadataTimestamp = TimerGetMillis();
        } else if(strcmp(msgBuf[0], "AVRCP_PLAY") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice.deviceId == deviceId) {
                // Clear the Metadata, since this notification comes prior
                // to the new metadata
                BC127ClearMetadata(bt);
                bt->playbackStatus = BC127_AVRCP_STATUS_PLAYING;
                LogDebug(LOG_SOURCE_BT, "BT: Playing");
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
        } else if(strcmp(msgBuf[0], "AVRCP_PAUSE") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            if (bt->activeDevice.deviceId == deviceId) {
                bt->playbackStatus = BC127_AVRCP_STATUS_PAUSED;
                LogDebug(LOG_SOURCE_BT, "BT: Paused");
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
        } else if(strcmp(msgBuf[0], "A2DP_STREAM_START") == 0) {
            if (bt->playbackStatus == BC127_AVRCP_STATUS_PAUSED) {
                bt->playbackStatus = BC127_AVRCP_STATUS_PLAYING;
                LogDebug(LOG_SOURCE_BT, "BT: Playing [A2DP Stream Start]");
                EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
            }
        } else if(strcmp(msgBuf[0], "CALL_ACTIVE") == 0) {
            bt->callStatus = BC127_CALL_ACTIVE;
            EventTriggerCallback(
                BC127Event_CallStatus,
                (unsigned char *) BC127_CALL_ACTIVE
            );
        } else if(strcmp(msgBuf[0], "CALL_END") == 0) {
            bt->callStatus = BC127_CALL_INACTIVE;
            EventTriggerCallback(
                BC127Event_CallStatus,
                (unsigned char *) BC127_CALL_INACTIVE
            );
        } else if(strcmp(msgBuf[0], "CALL_INCOMING") == 0) {
            bt->callStatus = BC127_CALL_INCOMING;
            EventTriggerCallback(
                BC127Event_CallStatus,
                (unsigned char *) BC127_CALL_INCOMING
            );
        } else if (strcmp(msgBuf[0], "CALL_OUTGOING") == 0) {
            bt->callStatus = BC127_CALL_OUTGOING;
            EventTriggerCallback(
                BC127Event_CallStatus,
                (unsigned char *) BC127_CALL_OUTGOING
            );
        } else if(strcmp(msgBuf[0], "LINK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            uint8_t isNew = 0;
            // No active device is configured
            if (bt->activeDevice.deviceId == 0) {
                LogDebug(LOG_SOURCE_BT, "BT: New Active Device");
                bt->activeDevice.deviceId = deviceId;
                strncpy(bt->activeDevice.macId, msgBuf[4], 12);
                char *deviceName = BC127PairedDeviceGetName(bt, msgBuf[4]);
                if (deviceName != 0) {
                    strncpy(bt->activeDevice.deviceName, deviceName, 32);
                } else {
                    BC127CommandGetDeviceName(bt, msgBuf[4]);
                }
                isNew = 1;
            } else {
                LogDebug(LOG_SOURCE_BT, "BT: Found device but already have active");
            }
            if (bt->activeDevice.deviceId == deviceId) {
                uint8_t linkId = strToInt(msgBuf[1]);
                BC127ConnectionOpenProfile(&bt->activeDevice, msgBuf[3], linkId);
                // Set the playback status
                if (strcmp(msgBuf[3], "AVRCP") == 0) {
                    if (strcmp(msgBuf[5], "PLAYING") == 0) {
                       bt->playbackStatus = BC127_AVRCP_STATUS_PLAYING;
                    } else {
                        bt->playbackStatus = BC127_AVRCP_STATUS_PAUSED;
                    }
                    EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
                }
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
            LogDebug(LOG_SOURCE_BT, "BT: Paired Device %s", msgBuf[1]);
            BC127CommandGetDeviceName(bt, msgBuf[1]);
        } else if(strcmp(msgBuf[0], "CLOSE_OK") == 0) {
            uint8_t deviceId = BC127GetDeviceId(msgBuf[1]);
            // If the open connection is closing, update the state
            if (bt->activeDevice.deviceId == deviceId) {
                uint8_t status = BC127ConnectionCloseProfile(
                    &bt->activeDevice,
                    msgBuf[2]
                );
                if (status == BC127_CONN_STATE_DISCONNECTED) {
                    bt->playbackStatus = BC127_AVRCP_STATUS_PAUSED;
                    // Notify the world that the device disconnected
                    memset(&bt->activeDevice, 0, sizeof(bt->activeDevice));
                    bt->activeDevice = BC127ConnectionInit();
                    EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
                    EventTriggerCallback(BC127Event_DeviceDisconnected, 0);
                }
                LogDebug(LOG_SOURCE_BT, "BT: Closed link %s", msgBuf[1]);
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
            // Clear the pairing error
            if (strcmp(msgBuf[2], "A2DP") == 0) {
                bt->pairingErrors[BC127_LINK_A2DP] = 0;
            }
            if (strcmp(msgBuf[2], "AVRCP") == 0) {
                bt->pairingErrors[BC127_LINK_AVRCP] = 0;
            }
            if (strcmp(msgBuf[2], "HFP") == 0) {
                bt->pairingErrors[BC127_LINK_HFP] = 0;
            }
            BC127ConnectionOpenProfile(&bt->activeDevice, msgBuf[2], linkId);
            LogDebug(LOG_SOURCE_BT, "BT: Open %s for ID %s", msgBuf[2], msgBuf[1]);
            EventTriggerCallback(
                BC127Event_DeviceLinkConnected,
                (unsigned char *) msgBuf[1]
            );
        } else if (strcmp(msgBuf[0], "OPEN_ERROR") == 0) {
            if (strcmp(msgBuf[1], "A2DP") == 0) {
                bt->pairingErrors[BC127_LINK_A2DP] = 1;
            }
            if (strcmp(msgBuf[1], "AVRCP") == 0) {
                bt->pairingErrors[BC127_LINK_AVRCP] = 1;
            }
            if (strcmp(msgBuf[1], "HFP") == 0) {
                bt->pairingErrors[BC127_LINK_HFP] = 1;
            }
        } else if (strcmp(msgBuf[0], "NAME") == 0) {
            char deviceName[33];
            uint8_t idx;
            uint8_t strIdx = 0;
            for (idx = 0; idx < strlen(msg) - 19; idx++) {
                char c = msg[idx + 19];
                // 0x22 (") is the character that wraps the device name
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
            EventTriggerCallback(BC127Event_DeviceFound, (unsigned char *) msgBuf[1]);
            LogDebug(LOG_SOURCE_BT, "BT: New Pairing Profile %s -> %s", msgBuf[1], deviceName);
        } else if(strcmp(msgBuf[0], "Build:") == 0) {
            // The device sometimes resets without sending the "Ready" message
            // so we instead watch for the build string
            memset(&bt->activeDevice, 0, sizeof(bt->activeDevice));
            bt->activeDevice = BC127ConnectionInit();
            bt->callStatus = BC127_CALL_INACTIVE;
            bt->metadataStatus = BC127_METADATA_STATUS_NEW;
            LogDebug(LOG_SOURCE_BT, "BT: Boot Complete");
            EventTriggerCallback(BC127Event_Boot, 0);
            EventTriggerCallback(BC127Event_PlaybackStatusChange, 0);
        } else if (strcmp(msgBuf[0], "STATE") == 0) {
            // Make sure the state is not "OFF", like when module first boots
            if (strcmp(msgBuf[1], "OFF") != 0) {
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
                LogDebug(LOG_SOURCE_BT, "BT: Got Status %s %s", msgBuf[2], msgBuf[3]);
            } else {
                // The BT Radio is off, likely meaning a reboot
                EventTriggerCallback(BC127Event_BootStatus, 0);
            }
        }
    }
    /* Sometimes there is not more than a Title or Album, which does not give
     * us sufficient information to push a metadata update. Therefore, request
     * the full metadata if we have partial metadata for more than the specified
     * timeout value
     **/
    uint32_t now = TimerGetMillis();
    if ((now - bt->metadataTimestamp) > BC127_METADATA_TIMEOUT &&
        bt->metadataStatus == BC127_METADATA_STATUS_NEW &&
        bt->activeDevice.avrcpLinkId != 0
    ) {
        BC127CommandGetMetadata(bt);
        bt->metadataTimestamp = now;
    }
    UARTReportErrors(&bt->uart);
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
    LogDebug(LOG_SOURCE_BT, "BT: Send Command '%s'", command);
    uint8_t idx = 0;
    for (idx = 0; idx < strlen(command); idx++) {
        CharQueueAdd(&bt->uart.txQueue, command[idx]);
    }
    CharQueueAdd(&bt->uart.txQueue, BC127_MSG_END_CHAR);
    // Set the interrupt flag
    SetUARTTXIE(bt->uart.moduleIndex, 1);
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
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
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
 *         uint8_t Device connection status
 */
uint8_t BC127ConnectionCloseProfile(BC127Connection_t *conn, char *profile)
{
    if (strcmp(profile, "A2DP") == 0) {
        conn->a2dpLinkId = 0;
    } else if (strcmp(profile, "AVRCP") == 0) {
        conn->avrcpLinkId = 0;
    } else if (strcmp(profile, "HFP") == 0) {
        conn->hfpLinkId = 0;
    }
    // Clear the connection once all the links are closed
    if (conn->a2dpLinkId == 0 && conn->avrcpLinkId == 0 && conn->hfpLinkId == 0) {
        memset(conn->macId, 0, 13);
        memset(conn->deviceName, 0, 33);
        conn->deviceId = 0;
        return BC127_CONN_STATE_DISCONNECTED;
    }
    return BC127_CONN_STATE_CONNECTED;
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
    } else if (strcmp(profile, "HFP") == 0) {
        conn->hfpLinkId = linkId;
    }
}
