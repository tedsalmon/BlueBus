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

// 32 Events Max
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
#define BT_EVENT_PBAP_CONTACT_RECEIVED 18
#define BT_EVENT_PBAP_SESSION_STATUS 19
#define BT_EVENT_PAIRINGS_LOADED 20

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
#define BT_LINK_TYPE_PBAP 7

#define BT_LIST_STATUS_OFF 0
#define BT_LIST_STATUS_RUNNING 1
#define BT_LIST_STATUS_RAN 2

#define BT_DEVICE_STATUS_DISCONNECTED 0
#define BT_DEVICE_STATUS_CONNECTED 1
#define BT_DEVICE_MAC_ID_LEN 6
#define BT_DEVICE_NAME_LEN 32
#define BT_DEVICE_RECORD_LEN (BT_DEVICE_MAC_ID_LEN + BT_DEVICE_NAME_LEN)

#define BT_PBAP_CONTACT_MAX_NUMBERS 3
#define BT_PBAP_CONTACT_NAME_LEN 32
#define BT_PBAP_LINE_BUFFER_SIZE 64
#define BT_PBAP_MAX_CONTACTS 8

#define BT_PBAP_OBJ_PHONEBOOK 0x00
#define BT_PBAP_OBJ_INCOMING 0x01
#define BT_PBAP_OBJ_OUTGOING 0x02
#define BT_PBAP_OBJ_MISSED 0x03
#define BT_PBAP_OBJ_COMBINED 0x04
#define BT_PBAP_OBJ_FAVORITES 0x05
#define BT_PBAP_OBJ_SPEEDDIAL 0x06

#define BT_PBAP_FRAME_DELIM 0x0D // <CR>

#define BT_PBAP_STATUS_IDLE 0
#define BT_PBAP_STATUS_PENDING 1
#define BT_PBAP_STATUS_HEADER_WAIT 2
#define BT_PBAP_STATUS_WAITING 3

#define BT_PBAP_BCD_STAR 0x0A
#define BT_PBAP_BCD_HASH 0x0B
#define BT_PBAP_BCD_PLUS 0x0C
#define BT_PBAP_BCD_UNUSED 0x0F

#define BT_PBAP_TEL_LEN 8
#define BT_PBAP_TEL_TYPE_CELL 1
#define BT_PBAP_TEL_TYPE_HOME 2
#define BT_PBAP_TEL_TYPE_UNKNOWN 0
#define BT_PBAP_TEL_TYPE_WORK 3


#define BT_VOICE_RECOG_OFF 0
#define BT_VOICE_RECOG_ON 1

#define BT_CONNECTION_TIMEOUT_MS 15000

/**
 * BTPBAPContactTelephone_t
 *     Description:
 *         Phone number stored in BCD format with type
 *     Fields:
 *         number - BCD encoded phone number (16 digits max)
 *         type - Telephone type (CELL, HOME, WORK, UNKNOWN)
 */
typedef struct BTPBAPContactTelephone_t {
    uint8_t number[8];
    uint8_t type;
} BTPBAPContactTelephone_t;

/**
 * BTPBAPContact_t
 *     Description:
 *         A contact entry from PBAP with name and up to 3 numbers
 *     Fields:
 *         name - The contact name
 *         numbers - Up to 3 phone numbers with types
 *         numberCount - The number of numbers for the given contact
 */
typedef struct BTPBAPContact_t {
    char name[BT_PBAP_CONTACT_NAME_LEN];
    BTPBAPContactTelephone_t numbers[BT_PBAP_CONTACT_MAX_NUMBERS];
    uint8_t numberCount;
} BTPBAPContact_t;

/**
 * BTPBAPParserState_t
 *     Description:
 *         PBAP Parser State Machine
 *     Fields:
 *         subEvent - Sub-event code for fragmented packets
 *         inVCard - Currently parsing a vCard
 *         isDelim - Previous char was CR (for CRLF handling)
 *         isEndOfBody - At the END:VCARD of the vCard
 *         bufferIdx - Current line buffer length
 *         buffer - Buffer for incomplete line
 */
typedef struct BTPBAPParserState_t {
    uint8_t subEvent;
    uint8_t inVCard: 1;
    uint8_t isDelim: 3;
    uint8_t isEndOfBody: 1;
    uint8_t bufferIdx;
    char buffer[BT_PBAP_LINE_BUFFER_SIZE];

} BTPBAPParserState_t;

/**
 * BTPBAP_t
 *     Description:
 *         PBAP state and contact data
 *     Fields:
 *         active - Is a PBAP session active
 *         status - BT_PBAP_STATUS_IDLE, BT_PBAP_STATUS_WAITING
 *         contactCount - Number of contacts in buffer
 *         contactIdx - The index of the contact we are currently copying
 *         contacts - Buffer for contact entries
 */
typedef struct BTPBAP_t {
    uint8_t active: 1;
    uint8_t status: 2;
    uint8_t contactCount;
    uint8_t contactIdx;
    BTPBAPContact_t contacts[BT_PBAP_MAX_CONTACTS];
    BTPBAPParserState_t parser;
} BTPBAP_t;

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
    uint8_t mapId: 5;
    uint8_t pbapId: 5;
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
 *         lastConnection - The last time a connection was initiated. This
 *             allows us to add some backpressure to connection attempts
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
    uint32_t lastConnection;
    uint32_t metadataTimestamp;
    uint32_t rxQueueAge;
    char title[BT_METADATA_FIELD_SIZE];
    char artist[BT_METADATA_FIELD_SIZE];
    char album[BT_METADATA_FIELD_SIZE];
    char callerId[BT_CALLER_ID_FIELD_SIZE];
    char dialBuffer[BT_DIAL_BUFFER_FIELD_SIZE];
    BTPBAP_t pbap;
    UART_t uart;
} BT_t;

void BTClearActiveDevice(BT_t *);
void BTClearMetadata(BT_t *);
void BTClearPairedDevices(BT_t *);
BTConnection_t BTConnectionInit();
void BTPairedDeviceClearRecords(void);
uint8_t BTPairedDeviceFind(BT_t *, uint8_t *);
void BTPairedDeviceInit(BT_t *, uint8_t *, uint8_t);
void BTPairedDeviceLoadRecord(BTPairedDevice_t *, uint8_t);
void BTPairedDeviceSave(uint8_t *, char *, uint8_t);
void BTPBAPParseVCard(BT_t *);
void BTPBAPTelephoneFromBCD(const uint8_t *, char *, uint8_t);
uint8_t BTPBAPTelephoneToBCD(const char *, uint8_t *);
#endif /* BT_COMMON_H */
