/*
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Sierra Wireless BC127 Bluetooth UART API
 */
#ifndef BC127_H
#define BC127_H
#include <stdint.h>
#include "uart.h"

#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1
#define BC127_MSG_END_CHAR 0x0D
#define BC127_MSG_LF_CHAR 0x0A
#define BC127_MSG_DELIMETER 0x20
#define BC127_METADATA_TITLE_FIELD_SIZE 100
#define BC127_METADATA_FIELD_SIZE 32

const static uint8_t BC127Event_Startup = 0;
const static uint8_t BC127Event_MetadataChange = 1;
const static uint8_t BC127Event_PlaybackStatusChange = 2;


/**
 * BC127_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the BC127
 */
typedef struct BC127_t {
    uint8_t avrcpStatus;
    char title[BC127_METADATA_TITLE_FIELD_SIZE];
    char artist[BC127_METADATA_FIELD_SIZE];
    char album[BC127_METADATA_FIELD_SIZE];
    uint8_t selectedDevice;
    struct UART_t uart;
} BC127_t;

struct BC127_t BC127Init();
void BC127CommandBackward(struct BC127_t *);
void BC127CommandForward(struct BC127_t *);
void BC127CommandPause(struct BC127_t *);
void BC127CommandPlay(struct BC127_t *);
void BC127CommandStatus(struct BC127_t *);
void BC127Process(struct BC127_t *);
void BC127SendCommand(struct BC127_t *, char *);
void BC127Startup();
#endif /* BC127_H */
