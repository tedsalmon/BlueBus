/*
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Sierra Wireless BC127 Bluetooth UART API
 */
#ifndef BC127_H
#define BC127_H
#define _ADDED_C_LIB 1
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../mappings.h"
#include "log.h"
#include "event.h"
#include "uart.h"
#include "utils.h"

#define BC127_AUDIO_I2S "0"
#define BC127_AUDIO_SPDIF "2"
#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1
#define BC127_CALL_ACTIVE 1
#define BC127_CALL_INACTIVE 0
#define BC127_CALL_INCOMING 2
#define BC127_CALL_OUTGOING 3
#define BC127_CLOSE_ALL 255
#define BC127_CONFIG_STATE_NEVER "0"
#define BC127_CONFIG_STATE_ALWAYS "1"
#define BC127_CONFIG_STATE_STARTUP "2"
#define BC127_CONN_STATE_NEW 0
#define BC127_CONN_STATE_CONNECTED 1
#define BC127_CONN_STATE_DISCONNECTED 2
#define BC127_MAX_DEVICE_PAIRED 8
#define BC127_MAX_DEVICE_PROFILES 5
#define BC127_METADATA_FIELD_SIZE 128
#define BC127_METADATA_TITLE_OFFSET 22
#define BC127_METADATA_ARTIST_OFFSET 23
#define BC127_METADATA_ALBUM_OFFSET 22
#define BC127_METADATA_STATUS_NEW 1
#define BC127_METADATA_STATUS_CUR 2
#define BC127_METADATA_TIMEOUT_STAGE_1 500
#define BC127_METADATA_TIMEOUT_STAGE_2 750
#define BC127_MSG_END_CHAR 0x0D
#define BC127_MSG_LF_CHAR 0x0A
#define BC127_MSG_DELIMETER 0x20
#define BC127_SHORT_NAME_MAX_LEN 7
#define BC127_STATE_OFF 0
#define BC127_STATE_ON 1
#define BC127_PROFILE_COUNT 4
#define BC127_LINK_A2DP 0
#define BC127_LINK_AVRCP 1
#define BC127_LINK_HFP 3

#define BC127Event_MetadataChange 0
#define BC127Event_PlaybackStatusChange 1
#define BC127Event_DeviceConnected 2
#define BC127Event_DeviceLinkConnected 3
#define BC127Event_DeviceDisconnected 4
#define BC127Event_Boot 5
#define BC127Event_DeviceFound 6
#define BC127Event_CallStatus 7
#define BC127Event_BootStatus 8
/**
 * BC127PairedDevice_t
 *     Description:
 *         This object defines a previously paired device
 *     Fields:
 *         macId - The MAC ID of the device (12 hexadecimal characters)
 *         deviceName - The friendly name of the device (32 ASCII characters)
 */
typedef struct BC127PairedDevice_t {
    char macId[13];
    char deviceName[33];
} BC127PairedDevice_t;

/**
 * BC127Device_t
 *     Description:
 *         This object defines the actively connected device
 *     Fields:
 *         macId - The MAC ID of the device (12 hexadecimal characters)
 *         deviceName - The friendly name of the device (32 ASCII characters)
 *         playbackStatus - Current Playback status - BC127_AVRCP_STATUS_PAUSED
 *                          or BC127_AVRCP_STATUS_PLAYING
 *         title - The title of the currently playing media
 *         artist - The artist of the currently playing media
 *         album - The album of the currently playing media
 *         deviceId - The Melody device ID (1-3)
 *         avrcpLinkId - The Melody Link ID for the AVRCP connection
 *         a2dpLinkId - The Melody Link ID for the A2DP connection
 *         hfpLinkId - The Melody Link ID for the HFP connection
 */
typedef struct BC127Connection_t {
    char macId[13];
    char deviceName[33];
    uint8_t deviceId;
    uint8_t avrcpLinkId;
    uint8_t a2dpLinkId;
    uint8_t hfpLinkId;
} BC127Connection_t;

/**
 * BC127_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the BC127
 *     Fields:
 *         activeDevice - The currently paired device
 *         pairedDevices - The list of devices we have paired with that are
 *             in range as of boot time or the last time the key was put in
 *             position 0.
 *         connectable - The current connectable state (0 = Off, 1 = On)
 *         discoverable - The current discoverable state (0 = Off, 1 = On)
 *         metadataStatus - Tracks if the metadata is new, so we can publish it
 *         pairedDevicesCount - The number of devices that have paired with us
 *            in all of time. The max is 8.
 *         pairingErrors - The key indicates the profile in error and the value
 *             in error. This is used to track what profiles we need to re-attempt
 *             a connection with.
 */
typedef struct BC127_t {
    BC127Connection_t activeDevice;
    BC127PairedDevice_t pairedDevices[BC127_MAX_DEVICE_PAIRED];
    uint8_t connectable;
    uint8_t discoverable;
    uint8_t pairedDevicesCount;
    uint8_t pairingErrors[BC127_PROFILE_COUNT];
    uint8_t callStatus;
    uint8_t metadataStatus;
    uint8_t playbackStatus;
    char title[BC127_METADATA_FIELD_SIZE];
    char artist[BC127_METADATA_FIELD_SIZE];
    char album[BC127_METADATA_FIELD_SIZE];
    uint32_t metadataTimestamp;
    UART_t uart;
} BC127_t;

BC127_t BC127Init();
void BC127ClearConnections(BC127_t *);
void BC127ClearMetadata(BC127_t *);
void BC127ClearPairedDevices(BC127_t *);
void BC127ClearInactivePairedDevices(BC127_t *);
void BC127ClearPairingErrors(BC127_t *);
void BC127CommandBackward(BC127_t *);
void BC127CommandBackwardSeekPress(BC127_t *);
void BC127CommandBackwardSeekRelease(BC127_t *);
void BC127CommandBtState(BC127_t *, uint8_t, uint8_t);
void BC127CommandCallAnswer(BC127_t *);
void BC127CommandCallEnd(BC127_t *);
void BC127CommandCallReject(BC127_t *);
void BC127CommandClose(BC127_t *, uint8_t);
void BC127CommandForward(BC127_t *);
void BC127CommandForwardSeekPress(BC127_t *);
void BC127CommandForwardSeekRelease(BC127_t *);
void BC127CommandGetDeviceName(BC127_t *, char *);
void BC127CommandGetMetadata(BC127_t *);
void BC127CommandList(BC127_t *);
void BC127CommandPause(BC127_t *);
void BC127CommandPlay(BC127_t *);
void BC127CommandProfileClose(BC127_t *, uint8_t);
void BC127CommandProfileOpen(BC127_t *, char *, char *);
void BC127CommandReset(BC127_t *);
void BC127CommandSetAudio(BC127_t *, uint8_t, uint8_t);
void BC127CommandSetAudioAnalog(BC127_t *, char *, char *, char *, char *);
void BC127CommandSetAudioDigital(BC127_t *, char *,char *, char *, char *);
void BC127CommandSetAutoConnect(BC127_t *, uint8_t);
void BC127CommandSetBtState(BC127_t *, uint8_t, uint8_t);
void BC127CommandSetBtVolConfig(BC127_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void BC127CommandSetCodec(BC127_t *, uint8_t, char *);
void BC127CommandSetMetadata(BC127_t *, uint8_t);
void BC127CommandSetModuleName(BC127_t *, char *);
void BC127CommandSetPin(BC127_t *, char *);
void BC127CommandSetProfiles(BC127_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void BC127CommandSetUART(BC127_t *, uint32_t, char *, uint8_t);
void BC127CommandStatus(BC127_t *);
void BC127CommandToggleVR(BC127_t *);
void BC127CommandTone(BC127_t *, char *);
void BC127CommandUnpair(BC127_t *);
void BC127CommandVolume(BC127_t *, uint8_t, uint8_t);
void BC127CommandWrite(BC127_t *);
uint8_t BC127GetConnectedDeviceCount(BC127_t *);
uint8_t BC127GetDeviceId(char *);
void BC127Process(BC127_t *);
void BC127SendCommand(BC127_t *, char *);
void BC127SendCommandEmpty(BC127_t *);

void BC127PairedDeviceInit(BC127_t *, char *, char *);
char *BC127PairedDeviceGetName(BC127_t *, char *);

BC127Connection_t BC127ConnectionInit();
uint8_t BC127ConnectionCloseProfile(BC127Connection_t *, char *);
void BC127ConnectionOpenProfile(BC127Connection_t *, char *, uint8_t);
#endif /* BC127_H */
