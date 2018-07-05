/*
 * File:   uart.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the PIC UART modules in a structured way to allow
 *     easier, and consistent data r/w
 */
#include <stdlib.h>
#include <stdint.h>
#include "char_queue.h"
#include "sfr_setters.h"
#include "uart.h"

/* Return a reprogrammable port register */
#define GET_RPOR(num) (((uint16_t *) &RPOR0) + num)

/* Hold a pin to register map for all reprogrammable output pins */
static uint16_t *ROPR_PINS[] = {
    GET_RPOR(0),
    GET_RPOR(0),
    GET_RPOR(1),
    GET_RPOR(1),
    GET_RPOR(2),
    GET_RPOR(2),
    GET_RPOR(3),
    GET_RPOR(3),
    GET_RPOR(4),
    GET_RPOR(4),
    GET_RPOR(5),
    GET_RPOR(5),
    GET_RPOR(6),
    GET_RPOR(6),
    GET_RPOR(7),
    GET_RPOR(7),
    GET_RPOR(8),
    GET_RPOR(8),
    GET_RPOR(9),
    GET_RPOR(9),
    GET_RPOR(10),
    GET_RPOR(10),
    GET_RPOR(11),
    GET_RPOR(11),
    GET_RPOR(12),
    GET_RPOR(12),
    GET_RPOR(13),
    GET_RPOR(13),
    GET_RPOR(14),
    GET_RPOR(14),
    GET_RPOR(15),
    GET_RPOR(15),
    GET_RPOR(16),
    GET_RPOR(16),
    GET_RPOR(17),
    GET_RPOR(17),
    GET_RPOR(18),
    GET_RPOR(18)
};

struct UART_t *UARTModules[UART_MODULES_COUNT];

// These values constitute the TX mode for each UART module
static const uint8_t UART_TX_MODES[] = {3, 5, 19, 21};

struct UART_t UARTInit(
    uint8_t uartModule,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t baudRate,
    uint8_t parity
) {
    struct UART_t uart;
    uart.txQueue = CharQueueInit();
    uart.rxQueue = CharQueueInit();
    uart.moduleNumber = uartModule;
    uart.moduleIndex = uartModule - 1;
    // Unlock the reprogrammable pin register
    __builtin_write_OSCCONL(OSCCON & 0xbf);
    // Set the RX Pin and register. The register comes from the PIC24FJ header
    // It's a pointer that Microchip gives you for easy access.
    switch (uartModule) {
        case 1:
            uart.registers = (UART *) &U1MODE;
            _U1RXR = rxPin;
            break;
        case 2:
            uart.registers = (UART *) &U2MODE;
            _U2RXR = rxPin;
            break;
        case 3:
            uart.registers = (UART *) &U3MODE;
            _U3RXR = rxPin;
            break;
        case 4:
            uart.registers = (UART *) &U4MODE;
            _U4RXR = rxPin;
            break;
    }
    // Set the TX Pin mode
    uint16_t txPinMode = UART_TX_MODES[uart.moduleIndex];
    if ((txPin) % 2 == 0) {
        // Set the least significant bits for the even pin number
        *ROPR_PINS[txPin] ^= txPinMode;
    } else {
        // Set the least significant bits of the register for the odd pin number
        *ROPR_PINS[txPin] ^= txPinMode * 256;
    }
    __builtin_write_OSCCONL(OSCCON & 0x40);
    //Set the BAUD Rate
    uart.registers->uxbrg = baudRate;
    // Enable UART, keep the module in 3-wire mode
    uart.registers->uxmode ^= 0b1000000000000000;
    if (parity == UART_PARITY_EVEN) {
        uart.registers->uxmode ^= 0b0000000000000010;
    }
    // Enable transmit and receive on the module
    uart.registers->uxsta ^= 0b0001010000000000;
    SetUARTRXIE(uart.moduleIndex, 1);
    SetUARTTXIE(uart.moduleIndex, 1);
    // Set the Interrupt Priority
    SetUARTRXIP(uart.moduleIndex, 4);
    SetUARTTXIP(uart.moduleIndex, 3);
    return uart;
}

void UARTAddModuleHandler(struct UART_t *uart)
{
    UARTModules[uart->moduleIndex] = uart;
}

struct UART_t * UARTGetModuleHandler(uint8_t moduleNumber)
{
    return UARTModules[moduleNumber - 1];
}

void UARTHandleRXInterrupt(uint8_t uartModuleNumber)
{
    if (UARTModules[uartModuleNumber] != 0x00) {
        struct UART_t *uart = UARTModules[uartModuleNumber];
        // While there's data on the RX buffer
        while (uart->registers->uxsta & 0x1) {
            unsigned char byte = uart->registers->uxrxreg;
            // No frame or parity errors
            if ((uart->registers->uxsta & 0xC) == 0) {
                // Clear the buffer overflow error, if it exists
                if ((uart->registers->uxsta & 0x2) == 1) {
                    uart->registers->uxsta ^= 0x2;
                }
                CharQueueAdd(&uart->rxQueue, byte);
            }
        }
    }
    // Clear the interrupt flag unconditionally, since we will be recalled to
    // this handler if there's additional data
    SetUARTRXIF(uartModuleNumber, 0);
}

void UARTHandleTXInterrupt(uint8_t uartModuleNumber)
{
    // If the module is NULL, it hasn't been initialized. This
    // interrupt is triggered when we setup the UART module.
    if (UARTModules[uartModuleNumber] != 0x00) {
        struct UART_t *uart = UARTModules[uartModuleNumber];
        while (uart->txQueue.size > 0) {
            unsigned char c = CharQueueNext(&uart->txQueue);
            uart->registers->uxtxreg = c;
            // Wait for the data to leave the tx buffer
            while ((uart->registers->uxsta & (1 << (9) )) != 0);
        }
        // If the queue has data, do not clear the interrupt flag
        SetUARTTXIE(uart->moduleIndex, uart->txQueue.size != 0);
    } else {
        // Remove the interrupt, since there's nothing to do
        SetUARTTXIE(uartModuleNumber, 0);
    }
}

void UARTSendData(struct UART_t *uart, unsigned char *data)
{
    unsigned char c;
    while ((c = *data++)) {
        CharQueueAdd(&uart->txQueue, c);
    }
    // Set the interrupt flag
    SetUARTTXIE(uart->moduleIndex, 1);
}

void UARTSendString(struct UART_t *uart, char *data)
{
    char c;
    while ((c = *data++)) {
        // This sucks. Print only ASCII characters, CR and LF
        if ((c >= 0x20 && c <= 0x7E) || c == 0x0D || c == 0x0A) {
            CharQueueAdd(&uart->txQueue, c);
        }
    }
    // Set the interrupt flag
    SetUARTTXIE(uart->moduleIndex, 1);
}

/*
 * Define the interrupt handlers that will pass off to our
 * handlers above.
 */
void __attribute__((__interrupt__, auto_psv)) _U1RXInterrupt()
{
    UARTHandleRXInterrupt(0);
}
void __attribute__((__interrupt__, auto_psv)) _U1TXInterrupt()
{
    UARTHandleTXInterrupt(0);
}
void __attribute__((__interrupt__, auto_psv)) _U2RXInterrupt()
{
    UARTHandleRXInterrupt(1);
}
void __attribute__((__interrupt__, auto_psv)) _U2TXInterrupt()
{
    UARTHandleTXInterrupt(1);
}
void __attribute__((__interrupt__, auto_psv)) _U3RXInterrupt()
{
    UARTHandleRXInterrupt(2);
}
void __attribute__((__interrupt__, auto_psv)) _U3TXInterrupt()
{
    UARTHandleTXInterrupt(2);
}
void __attribute__((__interrupt__, auto_psv)) _U4RXInterrupt()
{
    UARTHandleRXInterrupt(3);
}
void __attribute__((__interrupt__, auto_psv)) _U4TXInterrupt()
{
    UARTHandleTXInterrupt(3);
}
