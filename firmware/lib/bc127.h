/*
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Sierra Wireless BC127 Bluetooth UART API
 */
#ifndef BC127_H
#define	BC127_H
#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1
#include <stdint.h>
#include "uart.h"

/**
 * IBus_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the BC127
 */
typedef struct BC127_t {
    uint8_t avrcpStatus;
    uint8_t newAvrcpMeta;
    char artist[32];
    char title[32];
    char album[32];
    uint8_t selectedDevice;
    struct UART_t uart;
} BC127_t;

struct BC127_t BC127Init();
void BC127Process(struct BC127_t *);
void BC127SendCommand(struct BC127_t *, unsigned char *);

#endif	/* BC127_H */
