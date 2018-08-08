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
#include "../io_mappings.h"
#include "debug.h"
#include "event.h"
#include "uart.h"
#include "utils.h"

#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1
#define BC127_CLOSE_ALL 255
#define BC127_CONFIG_STATE_NEVER "0"
#define BC127_CONFIG_STATE_ALWAYS "1"
#define BC127_CONFIG_STATE_STARTUP "2"
#define BC127_CONN_STATE_NEW 0
#define BC127_CONN_STATE_CONNECTED 1
#define BC127_CONN_STATE_DISCONNECTED 2
#define BC127_MAX_DEVICE_CONN 3
#define BC127_MAX_DEVICE_PROFILES 5
#define BC127_METADATA_FIELD_SIZE 128
#define BC127_METADATA_TITLE_OFFSET 19
#define BC127_METADATA_ARTIST_OFFSET 20
#define BC127_METADATA_ALBUM_OFFSET 19
#define BC127_MSG_END_CHAR 0x0D
#define BC127_MSG_LF_CHAR 0x0A
#define BC127_MSG_DELIMETER 0x20
#define BC127_SHORT_NAME_MAX_LEN 7
#define BC127_STATE_OFF 0
#define BC127_STATE_ON 1

const static uint8_t BC127Event_Startup = 0;
const static uint8_t BC127Event_MetadataChange = 1;
const static uint8_t BC127Event_PlaybackStatusChange = 2;
const static uint8_t BC127Event_DeviceConnected = 3;
const static uint8_t BC127Event_DeviceLinkConnected = 4;
const static uint8_t BC127Event_DeviceReady = 5;

/**
 * BC127Connection_t
 *     Description:
 *         This object defines an open device connection
 */
typedef struct BC127Connection_t {
    char macId[13];
    char deviceName[33];
    uint8_t deviceId;
    uint8_t avrcpLink;
    uint8_t a2dpLink;
    uint8_t hfpLink;
    uint8_t state;
} BC127Connection_t;

/**
 * BC127_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the BC127
 */
typedef struct BC127_t {
    uint8_t avrcpStatus;
    char title[BC127_METADATA_FIELD_SIZE];
    char artist[BC127_METADATA_FIELD_SIZE];
    char album[BC127_METADATA_FIELD_SIZE];
    BC127Connection_t connections[BC127_MAX_DEVICE_CONN];
    BC127Connection_t *activeDevice;
    uint8_t connectable;
    uint8_t discoverable;
    UART_t uart;
} BC127_t;

BC127_t BC127Init();
void BC127ClearConnections(BC127_t *);
void BC127ClearMetadata(BC127_t *);
void BC127CommandBackward(BC127_t *);
void BC127CommandClose(BC127_t *, uint8_t);
void BC127CommandConnectable(BC127_t *, uint8_t);
void BC127CommandDiscoverable(BC127_t *, uint8_t);
void BC127CommandForward(BC127_t *);
void BC127CommandGetDeviceName(BC127_t *, char *);
void BC127CommandGetMetadata(BC127_t *);
void BC127CommandPause(BC127_t *);
void BC127CommandPlay(BC127_t *);
void BC127CommandProfileClose(BC127_t *, uint8_t);
void BC127CommandProfileOpen(BC127_t *, char *, char *);
void BC127CommandReset(BC127_t *);
void BC127CommandSetAutoConnect(BC127_t *, uint8_t);
void BC127CommandSetConnTo(BC127_t *, uint8_t);
void BC127CommandSetDiscoverable(BC127_t *, uint8_t, uint8_t);
void BC127CommandSetMetadata(BC127_t *, uint8_t);
void BC127CommandSetModuleName(BC127_t *, char *);
void BC127CommandSetPin(BC127_t *, char *);
void BC127CommandStatus(BC127_t *);
void BC127CommandWrite(BC127_t *);
uint8_t BC127GetConnectedDeviceCount(BC127_t *);
uint8_t BC127GetDeviceId(char *);
void BC127Process(BC127_t *);
void BC127SendCommand(BC127_t *, char *);
void BC127Startup();

BC127Connection_t BC127ConnectionInit(char *, uint8_t);
BC127Connection_t *BC127ConnectionGet(BC127_t *, char *, uint8_t);
void BC127ConnectionCloseProfile(BC127Connection_t *, char *);
void BC127ConnectionOpenProfile(BC127Connection_t *, char *, uint8_t);
#endif /* BC127_H */
