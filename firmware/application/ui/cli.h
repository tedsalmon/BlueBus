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
#include "../lib/bc127.h"
#include "../lib/char_queue.h"
#include "../lib/config.h"
#include "../lib/i2c.h"
#include "../lib/ibus.h"
#include "../lib/timer.h"
#include "../lib/uart.h"

// Banner timeout is in seconds
#define CLI_BANNER_TIMEOUT 300
#define CLI_MSG_END_CHAR 0x0D
#define CLI_MSG_DELIMETER 0x20
#define CLI_MSG_DELETE_CHAR 0x7F
#define CLI_VERSION_BANNER "BlueBus Firmware: 1.0.9.11\r\n"
/**
 * CLI_t
 *     Description:
 *         This object defines our CLI object
 *     Fields:
 *         UART_t *uart - A pointer to the UART module object
 *         BC127_t *bt - A pointer to the BC127 object
 *         IBus_t *bt - A pointer to the IBus object
 *         uint16_t lastChar - The last character
 */
typedef struct CLI_t {
    UART_t *uart;
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t terminalReadyTaskId;
    uint16_t lastChar;
    uint32_t lastRxTimestamp;
    uint8_t terminalReady;
} CLI_t;
void CLIInit(UART_t *, BC127_t *, IBus_t *);
void CLIProcess();
void CLITimerTerminalReady(void *);
#endif /* CLI_H */
