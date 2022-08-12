/*
 * File:   uart.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Summary:
 *      Provides an API for UART module interactions
 */
#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <string.h>
#include <xc.h>
#include "../mappings.h"
#include "char_queue.h"
#include "sfr_setters.h"
#include "timer.h"
#include "utils.h"
#define UART_BAUD_115200 34
#define UART_BAUD_9600 103
#define UART_MODULES_COUNT 2
#define UART_PARITY_NONE 0
#define UART_PARITY_EVEN 1
#define UART_PARITY_ODD 2

/**
 * UART_t
 *     Description:
 *         This object defines helper functionality to allow us to read and
 *         write data from the UART module
 */
typedef struct UART_t {
    volatile CharQueue_t rxQueue;
    uint8_t moduleIndex;
    uint8_t txPin;
    volatile uint32_t rxTimestamp;
    volatile UART *registers;
} UART_t;

UART_t UARTInit(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
void UARTAddModuleHandler(UART_t *uart);
void UARTDestroy(uint8_t);
UART_t * UARTGetModuleHandler(uint8_t);
void UARTRXQueueReset(UART_t *);
void UARTSendChar(UART_t *, uint8_t);
void UARTSendData(UART_t *, uint8_t *, uint16_t);
#endif /* UART_H */
