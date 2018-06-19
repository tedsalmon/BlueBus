/* 
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * This code implements the Sierra Wireless BC127 Bluetooth API
 */

#ifndef BC127_H
#define	BC127_H

#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1
#include <stdint.h>
#include "uart.h"

typedef struct BC127_t{
    uint8_t avrcpStatus;
    uint8_t newAvrcpMeta;
    char *artist;
    char *title;
    char *album;
    uint8_t selectedDevice;
    UART_t *uart;
    void (*destroy) (struct BC127_t *);
    void (*process) (struct BC127_t *);
    void (*sendCommand) (struct BC127_t *, unsigned char *);
} BC127_t;

BC127_t *BC127Init();
void BC127Destroy(struct BC127_t *);
void BC127Process(struct BC127_t *);
void BC127SendCommand(struct BC127_t *, unsigned char *);

#endif	/* BC127_H */