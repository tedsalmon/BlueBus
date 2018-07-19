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
#include "char_queue.h"
#include "debug.h"
#include "sfr_setters.h"

#define UART_BAUD_115200 8
#define UART_BAUD_9600 103
#define UART_MODULES_COUNT 4
#define UART_PARITY_NONE 0
#define UART_PARITY_EVEN 1

/**
 * UART_t
 *     Description:
 *         This object defines helper functionality to allow us to read and
 *         write data from the UART module
 */
typedef struct UART_t {
    CharQueue_t rxQueue;
    CharQueue_t txQueue;
    uint8_t moduleNumber;
    uint8_t moduleIndex;
    UART *registers;
} UART_t;

UART_t UARTInit(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
void UARTAddModuleHandler(UART_t *uart);
UART_t * UARTGetModuleHandler(uint8_t);
void UARTHandleRXInterrupt(uint8_t);
void UARTHandleTXInterrupt(uint8_t);
void UARTSendData(UART_t *, unsigned char *);
void UARTSendString(UART_t *, char *);
#endif /* UART_H */
