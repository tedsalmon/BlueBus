/*
 * File:   uart.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Summary:
 *      Provides an API for UART module interactions
 */
#ifndef UART_H
#define	UART_H
#define UART_BAUD_115200 8
#define UART_BAUD_9600 103
#define UART_MODULES_COUNT 4

#include <p24FJ1024GB610.h>
#include "char_queue.h"
#include "sfr_setters.h"

/**
 * UART_t
 *     Description:
 *         This object defines helper functionality to allow us to read and
 *         write data from the UART module
 */
typedef struct UART_t {
    struct CharQueue_t rxQueue;
    struct CharQueue_t txQueue;
    uint8_t moduleNumber;
    uint8_t moduleIndex;
    UART *registers;
} UART_t;

struct UART_t UARTInit(uint8_t, uint8_t, uint8_t, uint8_t);

void UARTAddModuleHandler(struct UART_t *uart);
struct UART_t * UARTGetModuleHandler(uint8_t);
void UARTHandleRXInterrupt(uint8_t);
void UARTHandleTXInterrupt(uint8_t);
void UARTSendData(struct UART_t *, unsigned char *);
void UARTSendString(struct UART_t *, char *);

#endif	/* UART_H */
