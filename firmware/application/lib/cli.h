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
#include "bc127.h"
#include "char_queue.h"
#include "config.h"
#include "i2c.h"
#include "ibus.h"
#include "uart.h"

#define CLI_MSG_END_CHAR 0x0D
#define CLI_MSG_DELIMETER 0x20
/**
 * CLI_t
 *     Description:
 *         This object defines our CLI object
 *     Fields:
 *         UART_t *uart - A pointer to the UART module object
 *         BC127_t *bt - A pointer to the BC127 object
 *         IBus_t *bt - A pointer to the IBus object
 */
typedef struct CLI_t {
    UART_t *uart;
    BC127_t *bt;
    IBus_t *ibus;
    uint8_t lastChar;
} CLI_t;
CLI_t CLIInit(UART_t *, BC127_t *, IBus_t *);
void CLIProcess(CLI_t *);
#endif /* CLI_H */
