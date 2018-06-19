/*
 * File:   uart.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Summary:
 *      Provides an API for UART module interactions
 */

#ifndef UART_H
#define	UART_H
#define UART_BAUD_9600 8
#define UART_BAUD_115200 103
#define UART1_RX_PIN RPINR18bits.U1RXR
#define UART2_RX_PIN RPINR19bits.U2RXR

#include "char_queue.h"

typedef struct UART_t{
    CharQueue_t *messageQueue;
    void (*destroy) (struct UART_t *);
    void (*sendData) (struct UART_t *, unsigned char *);
} UART_t;

void UARTDestroy(struct UART_t *);
UART_t *UARTInit(uint8_t, uint8_t, uint8_t);
void UARTSendData(struct UART_t *, unsigned char *);

#endif	/* UART_H */
