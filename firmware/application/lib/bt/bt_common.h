/*
 * File:   bt_common.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Structs, defines and common functions shared by Bluetooth Module Stacks
 */
#ifndef BT_COMMON_H
#define BT_COMMON_H
#include "../../mappings.h"
#include "../config.h"
#include "../eeprom.h"
#include "../log.h"
#include "../event.h"
#include "../uart.h"

#define BT_AVRCP_ACTION_GET_METADATA 0
#define BT_AVRCP_ACTION_SET_TRACK_CHANGE_NOTIF 1

#define BT_AVRCP_STATUS_PAUSED 0
#define BT_AVRCP_STATUS_PLAYING 1

#define BT_BTM_TYPE_BC127 BOARD_VERSION_ONE
#define BT_BTM_TYPE_BM83 BOARD_VERSION_TWO

#define BT_CALL_INACTIVE 0
#define BT_CALL_ACTIVE 1
#define BT_CALL_VR 2
#define BT_CALL_INCOMING 3
#define BT_CALL_OUTGOING 4
#define BT_CALL_SCO_CLOSE 5
#define BT_CALL_SCO_OPEN 6
#define BT_CALLER_ID_FIELD_SIZE 32
#define BT_DIAL_BUFFER_FIELD_SIZE 32
#define BT_CLOSE_ALL 255

#define BT_STATUS_OFF 0
#define BT_STATUS_DISCONNECTED 1
#define BT_STATUS_CONNECTED 2
#define BT_STATUS_CONNECTING 3

#define BT_TYPE_CLEAR_ALL 0

#define BT_EVENT_METADATA_UPDATE 0
#define BT_EVENT_PLAYBACK_STATUS_CHANGE 1
#define BT_EVENT_DEVICE_CONNECTED 2
#define BT_EVENT_DEVICE_DISCONNECTED 4
#define BT_EVENT_DEVICE_LINK_CONNECTED 5
#define BT_EVENT_DEVICE_LINK_DISCONNECTED 6
#define BT_EVENT_BOOT 7
#define BT_EVENT_DEVICE_FOUND 8
#define BT_EVENT_CALL_STATUS_UPDATE 9
#define BT_EVENT_BOOT_STATUS 10
#define BT_EVENT_CALLER_ID_UPDATE 11
#define BT_EVENT_VOLUME_UPDATE 12
#define BT_EVENT_AVRCP_PDU_CHANGE 13
#define BT_EVENT_LINK_BACK_STATUS 14
#define BT_EVENT_BTM_ADDRESS 15
#define BT_EVENT_TIME_UPDATE 16
#define BT_EVENT_DSP_STATUS 17

#define BT_LEN_MAC_ID 6

#define BT_MAX_PAIRINGS 8
#define BT_DEVICE_NAME_LEN 32
#define BT_METADATA_MAX_SIZE 384
#define BT_METADATA_FIELD_SIZE 128
// BC127-specific
#define BT_METADATA_STATUS_CUR 0
#define BT_METADATA_STATUS_UPD 1
// BM83-specific

#define BT_STATE_OFF 0
#define BT_STATE_ON 1
#define BT_STATE_STANDBY 2
#define BT_PROFILE_COUNT 9

#define BT_LINK_TYPE_ACL 1
#define BT_LINK_TYPE_A2DP 2
#define BT_LINK_TYPE_AVRCP 3
#define BT_LINK_TYPE_HFP 4
#define BT_LINK_TYPE_BLE 5
#define BT_LINK_TYPE_MAP 6

#define BT_LIST_STATUS_OFF 0
#define BT_LIST_STATUS_RUNNING 1
#define BT_LIST_STATUS_RAN 2

#define BT_DEVICE_STATUS_DISCONNECTED 0
#define BT_DEVICE_STATUS_CONNECTED 1
#define BT_DEVICE_MAC_ID_LEN 6
#define BT_DEVICE_NAME_LEN 32
#define BT_DEVICE_RECORD_LEN (BT_DEVICE_MAC_ID_LEN + BT_DEVICE_NAME_LEN)

#define BT_VOICE_RECOG_OFF 0
#define BT_VOICE_RECOG_ON 1

/**
 * BTPairedDevice_t
 *     Description:
 *         This object defines a previously paired device
 *     Fields:
 *         macId - The MAC ID of the device (6 bytes)
 *         deviceName - The friendly name of the device
 */
typedef struct BTPairedDevice_t {
    uint8_t macId[BT_DEVICE_MAC_ID_LEN];
    char deviceName[BT_DEVICE_NAME_LEN];
    uint8_t number;
} BTPairedDevice_t;

/**
 * BTConnectionAVRCPCapbilities_t
 *     Description:
 *         This object defines the capabilities of the currently connected
 *         devices
 *     Fields:
 */
typedef struct BTConnectionAVRCPCapabilities_t {
    uint8_t playbackChanged: 1;
    uint8_t trackChanged: 1;
    uint8_t trackReachedEnd: 1;
    uint8_t trackReachedStart: 1;
    uint8_t playbackPosChanged: 1;
    uint8_t nowPlayingChanged: 1;
    uint8_t volumeChanged: 1;
} BTConnectionAVRCPCapabilities_t;

/**
 * BTConnection_t
 *     Description:
 *         This object defines the actively connected device
 *     Fields:
 *         deviceId - The device ID (1-3)
 *         deviceIndex - The PDL database index for this device
 *         avrcpId - The Link ID for the AVRCP connection
 *         a2dpId - The Link ID for the A2DP connection
 *         hfpId - The Link ID for the HFP connection
 *         bleId - The Link ID for the BLE connection
 *         mapId - The Link ID for the MAP connection
 *         pbapId - The Link ID for the PBAP connection
 *         a2dpVolume - A2DP volume
 *         avrcpCaps - Available AVRCP Events
 */
typedef struct BTConnection_t {
    uint8_t status: 1;
    uint8_t deviceId;
    uint8_t deviceIndex;
    uint8_t avrcpId: 4;
    uint8_t a2dpId: 4;
    uint8_t hfpId: 4;
    uint8_t bleId: 4;
    uint8_t mapId: 4;
    uint8_t pbapId: 4;
    uint8_t a2dpVolume;
    BTConnectionAVRCPCapabilities_t avrcpCaps;
} BTConnection_t;

/**
 * BT_t
 *     Description:
 *         This object defines status functionality to help us interact with
 *         our Bluetooth Module
 *     Fields:
 *         activeDevice - The currently paired device
 *         pairedDevices - The list of devices we have paired with that are
 *             in range as of boot time or the last time the key was put in
 *             position 0.
 *         status - The state of the module (Connecting, connected, etc)
 *         type - BM83 or BC127
 *         connectable - The current connectable state (0 = Off, 1 = On)
 *         discoverable - The current discoverable state (0 = Off, 1 = On)
 *         avrcpStatus - The required AVRCP updates
 *         metadataStatus - Tracks if the metadata is new, so we can publish it
 *         playbackStatus - If we're paused or playing
 *         vrStatus- If Voice Recognition is on or off
 *         callStatus - The call status
 *         scoStatus - If the SCO channel is open or closed
 *         powerState - 2/1/0 1 Standby, On, Off
 *         pairedDevicesCheck - For the BC127, count the number of devices
 *             return by the LIST command
 *         pairedDevicesCount - The number of devices that have paired with us
 *            in all of time. The max is 8.
 *         pairedDeviceFound - For the BC127, the total number of items in the
 *            LIST response.
 *         pairingErrors - The key indicates the profile in error and the value
 *             in error. This is used to track what profiles we need to re-attempt
 *             a connection with.
 *         metadataTimestamp - The last time we got metadata of any kind
 *         rxQueueAge - Used to track how long data has been sitting on the
 *             RX queue without getting a MSG_END_CHAR.
 */
typedef struct BT_t {
    BTConnection_t activeDevice;
    BTPairedDevice_t pairedDevices[BT_MAX_PAIRINGS];
    uint8_t status: 2;
    uint8_t type: 1;
    uint8_t connectable: 1;
    uint8_t discoverable: 1;
    uint8_t avrcpUpdates: 2;
    uint8_t metadataStatus: 1;
    uint8_t playbackStatus: 1;
    uint8_t vrStatus: 1;
    uint8_t callStatus: 3;
    uint8_t scoStatus: 3;
    uint8_t powerState: 2;
    uint8_t pairedDevicesCheck: 2;
    uint8_t pairedDevicesCount: 4;
    uint8_t pairedDevicesFound: 4;
    uint8_t pairingErrors[BT_PROFILE_COUNT];
    uint32_t metadataTimestamp;
    uint32_t rxQueueAge;
    char title[BT_METADATA_FIELD_SIZE];
    char artist[BT_METADATA_FIELD_SIZE];
    char album[BT_METADATA_FIELD_SIZE];
    char callerId[BT_CALLER_ID_FIELD_SIZE];
    char dialBuffer[BT_DIAL_BUFFER_FIELD_SIZE];
    UART_t uart;
} BT_t;

void BTClearActiveDevice(BT_t *);
void BTClearMetadata(BT_t *);
void BTClearPairedDevices(BT_t *);
uint8_t BTPairedDevicesFind(BT_t *, uint8_t *);
BTConnection_t BTConnectionInit();
void BTPairedDeviceInit(BT_t *, uint8_t *, uint8_t);
void BTPairedDeviceSave(uint8_t *, char *, uint8_t);
void BTPairedDeviceLoadRecord(BTPairedDevice_t *, uint8_t);
void BTPairedDeviceClearRecords(void);
#endif /* BT_COMMON_H */
