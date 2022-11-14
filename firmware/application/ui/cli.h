/*
 * File: cli.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a CLI to pass commands to the device
 */
#ifndef CLI_H
#define CLI_H
#define _ADDED_C_LIB 1
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../mappings.h"
#include "../lib/bt/bt_bc127.h"
#include "../lib/bt/bt_bm83.h"
#include "../lib/bt.h"
#include "../lib/char_queue.h"
#include "../lib/config.h"
#include "../lib/i2c.h"
#include "../lib/ibus.h"
#include "../lib/pcm51xx.h"
#include "../lib/timer.h"
#include "../lib/uart.h"

// Banner timeout is in seconds
#define CLI_BANNER_TIMEOUT 300
#define CLI_MSG_END_CHAR 0x0D
#define CLI_MSG_DELIMETER 0x20
#define CLI_MSG_DELETE_CHAR 0x7F
/**
 * CLI_t
 *     Description:
 *         This object defines our CLI object
 *     Fields:
 *         UART_t *uart - A pointer to the UART module object
 *         BT_t *bt - A pointer to the Blueooth module object
 *         IBus_t *bt - A pointer to the IBus object
 *         uint16_t lastChar - The last character
 */
typedef struct CLI_t {
    UART_t *uart;
    BT_t *bt;
    IBus_t *ibus;
    uint8_t terminalReadyTaskId;
    uint16_t lastChar;
    uint32_t lastRxTimestamp;
    uint8_t terminalReady;
} CLI_t;
void CLIInit(UART_t *, BT_t *, IBus_t *);
void CLICommandBTBC127(char **, uint8_t *, uint8_t);
void CLICommandBTBM83(char **, uint8_t *, uint8_t);
void CLIEventBTBTMAddress(void *, uint8_t *);
void CLIProcess();
void CLITimerTerminalReady(void *);
#endif /* CLI_H */
