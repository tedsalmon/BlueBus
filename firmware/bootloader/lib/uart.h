/*
 * File:   uart.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Summary:
 *      Provides an API for UART module interactions
 */
#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <xc.h>
#include "../mappings.h"
#include "sfr_setters.h"
#include "timer.h"
#include "utils.h"
#define UART_BAUD_9600 103
#define UART_BAUD_111000 8
#define UART_BAUD_115200 8
#define UART_MODULES_COUNT 2
#define UART_RX_QUEUE_SIZE 512
#define UART_RX_QUEUE_TIMEOUT 500

/**
 * UART_t
 *     Description:
 *         This object defines helper functionality to allow us to read and
 *         write data from the UART module
 */
typedef struct UART_t {
    char rxQueue[UART_RX_QUEUE_SIZE];
    uint16_t rxQueueReadCursor;
    uint16_t rxQueueSize;
    uint16_t rxQueueWriteCursor;
    uint32_t rxLastTimestamp;
    uint8_t moduleIndex;
    uint8_t txPin;
    volatile UART *registers;
} UART_t;

UART_t UARTInit(uint8_t, uint8_t, uint8_t, uint8_t);
void UARTAddModuleHandler(UART_t *uart);
UART_t * UARTGetModuleHandler(uint8_t);
void UARTDestroy(uint8_t);
unsigned char UARTGetNextByte(UART_t *);
unsigned char UARTGetOffsetByte(UART_t *, uint16_t);
void UARTReadData(UART_t *);
void UARTResetRxQueue(UART_t *);
uint16_t UARTRxQueueSeek(UART_t *, const unsigned char);
void UARTSendData(UART_t *, unsigned char *, uint8_t);
#endif /* UART_H */
