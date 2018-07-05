/*
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Sierra Wireless BC127 Bluetooth UART API
 */
#ifndef BC127_H
#define BC127_H
#include <stdint.h>
#include "ibus.h"
#include "uart.h"

#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1
#define BC127_MSG_END_CHAR 0x0D
#define BC127_MSG_LF_CHAR 0x0A
#define BC127_MSG_DELIMETER 0x20
#define BC127_METADATA_FIELD_SIZE 64

/**
 * BC127_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the BC127
 */
typedef struct BC127_t {
    uint8_t avrcpStatus;
    char artist[BC127_METADATA_FIELD_SIZE];
    char title[BC127_METADATA_FIELD_SIZE];
    char album[BC127_METADATA_FIELD_SIZE];
    uint8_t selectedDevice;
    struct UART_t uart;
} BC127_t;

struct BC127_t BC127Init();
void BC127Process(struct BC127_t *, struct IBus_t *);
void BC127SendCommand(struct BC127_t *, char *);
void BC127Startup(struct BC127_t *);
void BC127Pause(struct BC127_t *);
void BC127Play(struct BC127_t *);
#endif /* BC127_H */
