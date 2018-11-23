/*
 * File:   uart.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the PIC UART modules in a structured way to allow
 *     easier, and consistent data r/w
 */
#include "uart.h"

/* Hold a pin to register map for all programmable output pins */
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

// These values constitute the TX mode for each UART module
static const uint8_t UART_TX_MODES[] = {3, 5};

/**
 * UARTInit()
 *     Description:
 *         Generate a UART module to work with
 *     Params:
 *         uint8_t baudRate - The calculated baud rate to run at
 *     Returns:
 *         UART_t
 */
UART_t UARTInit(
    uint8_t uartModule,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t baudRate
) {
    UART_t uart;
    uart.moduleNumber = uartModule;
    uart.txPin = txPin;
    // Unlock the reprogrammable pin register
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    // Set the RX Pin and register. The register comes from the PIC24FJ header
    // It's a pointer that Microchip gives you for easy access.
    switch (uartModule) {
        case 1:
            uart.registers = (volatile UART *) &U1MODE;
            _U1RXR = rxPin;
            break;
        case 2:
            uart.registers = (volatile UART *) &U2MODE;
            _U2RXR = rxPin;
            break;
    }
    // Set the TX Pin Mode
    UARTSetRPORMode(txPin, UART_TX_MODES[uart.moduleNumber - 1]);
    //Set the BAUD Rate
    uart.registers->uxbrg = baudRate;
    // Enable UART, keep the module in 3-wire mode
    uart.registers->uxmode ^= 0b1000000000000000;
    // Enable transmit and receive on the module
    uart.registers->uxsta ^= 0b0001010000000000;
    return uart;
}

/**
 * UARTDestroy()
 *     Description:
 *         Reset the UART module that was used by the code
 *     Params:
 *         UART_t uart - The uart module object
 *     Returns:
 *         void
 */
void UARTDestroy(UART_t *uart) {
    // Unlock the reprogrammable pin register and set the pins to zero
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    switch (uart->moduleNumber) {
        case 1:
            _U1RXR = 0;
            break;
        case 2:
            _U2RXR = 0;
            break;
    }
    UARTSetRPORMode(uart->txPin, 0);
    __builtin_write_OSCCONL(OSCCON & 0x40);
    //Set the BAUD Rate back to 0
    uart->registers->uxbrg = 0;
    // Disable UART
    uart->registers->uxmode ^= 0b1000000000000000;
    // Disable transmit and receive on the module
    uart->registers->uxsta = 0;
}


/**
 * UARTGetNextByte()
 *     Description:
 *         Shifts the next byte in the queue out, as seen by the read cursor.
 *         Once the byte is returned, it should be considered destroyed from the
 *         queue.
 *     Params:
 *         UART_t *uart - The UART object
 *     Returns:
 *         unsigned char
 */
unsigned char UARTGetNextByte(UART_t *uart)
{
    unsigned char data = uart->rxQueue[uart->rxQueueReadCursor];
    // Remove the byte from memory
    uart->rxQueue[uart->rxQueueReadCursor] = 0x00;
    uart->rxQueueReadCursor++;
    if (uart->rxQueueReadCursor >= UART_RX_QUEUE_SIZE) {
        uart->rxQueueReadCursor = 0;
    }
    if (uart->rxQueueSize > 0) {
        uart->rxQueueSize--;
    }
    return data;
}

/**
 * UARTGetOffsetByte()
 *     Description:
 *         Return the byte at the current index plus the given offset
 *     Params:
 *         UART_t *uart - The UART object
 *         uint8_t offset - The number to offset from the current index
 *     Returns:
 *         unsigned char
 */
unsigned char UARTGetOffsetByte(UART_t *uart, uint8_t offset)
{
    uint8_t cursor = uart->rxQueueReadCursor;
    while (offset) {
        cursor++;
        if (cursor >= UART_RX_QUEUE_SIZE) {
            cursor = 0;
        }
        offset--;
    }
    return uart->rxQueue[cursor];
}

/**
 * UARTReadData()
 *     Description:
 *         Read any pending bytes from the UART module buffer and place them
 *         into our circular buffer
 *     Params:
 *         UART_t *uart - The UART object
 *     Returns:
 *         void
 */
void UARTReadData(UART_t *uart)
{
    // While there is data in the RX buffer
    while ((uart->registers->uxsta & 0x1) == 1) {
        uint8_t hasErr = (uart->registers->uxsta & 0xE) != 0;
        // Clear the buffer overflow error, if it exists
        if ((uart->registers->uxsta & 0x2) != 0) {
            uart->registers->uxsta ^= 0x2;
        }
        unsigned char byte = uart->registers->uxrxreg;
        if (uart->rxQueueSize != (UART_RX_QUEUE_SIZE + 1) && !hasErr) {
            if (uart->rxQueueWriteCursor == UART_RX_QUEUE_SIZE) {
                uart->rxQueueWriteCursor = 0;
            }
            uart->rxQueue[uart->rxQueueWriteCursor] = byte;
            uart->rxQueueWriteCursor++;
            uart->rxQueueSize++;
            uart->rxLastTimestamp = TimerGetMillis();
        }
    }
}

/**
 * UARTResetRxQueue()
 *     Description:
 *         Clear all bytes from the Rx Queue
 *     Params:
 *         UART_t *uart - The UART object
 *     Returns:
 *         void
 */
void UARTResetRxQueue(UART_t *uart)
{
    uart->rxQueueSize = 0;
    uart->rxQueueWriteCursor = 0;
    uart->rxQueueReadCursor = 0;
}

/**
 * UARTSendData()
 *     Description:
 *         Send the given char array via UART
 *     Params:
 *         UART_t *uart - The UART object
 *         unsigned char *data - The stream to send
 *         uint16_t length - The count of bytes in the packet
 *     Returns:
 *         void
 */
void UARTSendData(UART_t *uart, unsigned char *data, uint8_t length)
{
    uint8_t i;
    for (i = 0; i < length; i++){
        uart->registers->uxtxreg = data[i];
        // Wait for the data to leave the tx buffer
        while ((uart->registers->uxsta & (1 << 9)) != 0);
    }
}

/**
 * UARTSendData()
 *     Description:
 *         Send the given string over UART
 *     Params:
 *         UART_t *uart - The UART object
 *         char *data - The stream to send
 *     Returns:
 *         void
 */
void UARTSendString(UART_t *uart, char *data)
{
    char c;
    while ((c = *data++)) {
        uart->registers->uxtxreg = c;
        // Wait for the data to leave the tx buffer
        while ((uart->registers->uxsta & (1 << 9)) != 0);
    }
}

/**
 * UARTSetRPORMode()
 *     Description:
 *         Set the mode of a programmable output pin
 *     Params:
 *         uint8_t pin - The pin to set
 *         uint8_t mode - The mode to set the given pin to
 *     Returns:
 *         void
 */
void UARTSetRPORMode(uint8_t pin, uint16_t mode)
{
    if ((pin % 2) == 0) {
        uint16_t msb = *ROPR_PINS[pin] >> 8;
        // Set the least significant bits for the even pin number
        *ROPR_PINS[pin] = (msb << 8) + mode;
    } else {
        uint16_t lsb = *ROPR_PINS[pin] & 0xFF;
        // Set the least significant bits of the register for the odd pin number
        *ROPR_PINS[pin] = (mode << 8) + lsb;
    }
}
