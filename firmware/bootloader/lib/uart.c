/*
 * File:   uart.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the PIC UART modules in a structured way to allow
 *     easier, and consistent data r/w
 */
#include "uart.h"

static UART_t *UARTModules[UART_MODULES_COUNT];

// These values constitute the TX mode for each UART module
static const uint8_t UART_TX_MODES[] = {3, 5};

UART_t UARTInit(
    uint8_t uartModule,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t baudRate,
    uint8_t parity
) {
    UART_t uart;
    uart.moduleIndex = uartModule - 1;
    uart.txPin = txPin;
    UARTResetRxQueue(&uart);
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
    UtilsSetRPORMode(txPin, UART_TX_MODES[uart.moduleIndex]);
    __builtin_write_OSCCONL(OSCCON & 0x40);
    //Set the BAUD Rate
    uart.registers->uxbrg = baudRate;
    // Set the initial UART state
    uart.registers->uxmode = 0;
    // Enable UART, keep the module in 3-wire mode
    uart.registers->uxmode ^= 0b1000000000000000;
    if (parity == UART_PARITY_EVEN) {
        uart.registers->uxmode ^= 0b0000000000000010;
    } else if (parity == UART_PARITY_ODD) {
        uart.registers->uxmode ^= 0b0000000000000100;
    }
    if (baudRate == UART_BAUD_115200) {
        // Set high baud rate to enabled
        uart.registers->uxmode ^= 0b0000000000001000;
    }
    // Enable transmit and receive on the module
    uart.registers->uxsta ^= 0b0001010000000000;
    return uart;
}

void UARTAddModuleHandler(UART_t *uart)
{
    UARTModules[uart->moduleIndex] = uart;
}

/**
 * UARTDestroy()
 *     Description:
 *         Reset the UART module that was used by the code
 *     Params:
 *         uint8_t uartModule - The UART Module Number
 *     Returns:
 *         void
 */
void UARTDestroy(uint8_t uartModule) {
    UART_t *uart = UARTGetModuleHandler(uartModule);
    // Unlock the reprogrammable pin register and set the pins to zero
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    switch (uartModule) {
        case 1:
            _U1RXR = 0;
            break;
        case 2:
            _U2RXR = 0;
            break;
    }
    UtilsSetRPORMode(uart->txPin, 0);
    __builtin_write_OSCCONL(OSCCON & 0x40);
    //Set the BAUD Rate back to 0
    uart->registers->uxbrg = 0;
    // Disable UART
    uart->registers->uxmode = 0;
    // Disable transmit and receive on the module
    uart->registers->uxsta = 0;
    // Pull down the RX and TX pins for the module
    switch (uartModule) {
        case BC127_UART_MODULE:
            BC127_UART_RX_PIN_MODE = 1;
            BC127_UART_TX_PIN_MODE = 1;
            break;
        case SYSTEM_UART_MODULE:
            SYSTEM_UART_RX_PIN_MODE = 1;
            SYSTEM_UART_TX_PIN_MODE = 1;
            break;
    }
}

UART_t * UARTGetModuleHandler(uint8_t moduleIndex)
{
    return UARTModules[moduleIndex - 1];
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
unsigned char UARTGetOffsetByte(UART_t *uart, uint16_t offset)
{
    uint16_t cursor = uart->rxQueueReadCursor;
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
 * UARTRxQueueSeek()
 *     Description:
 *         Checks if a given byte is in the queue and return the length of
 *         characters prior to it.
 *     Params:
 *         CharQueue_t *queue - The queue
 *         const unsigned char needle - The character to look for
 *     Returns:
 *         uint16_t - The length of characters prior to the needle or zero if
 *                   the needle wasn't found
 */
uint16_t UARTRxQueueSeek(UART_t *uart, const unsigned char needle)
{
    uint16_t readCursor = uart->rxQueueReadCursor;
    uint16_t size = uart->rxQueueSize;
    uint16_t cnt = 1;
    while (size > 0) {
        if (uart->rxQueue[readCursor] == needle) {
            return cnt;
        }
        if (readCursor >= UART_RX_QUEUE_SIZE) {
            readCursor = 0;
        }
        cnt++;
        size--;
        readCursor++;
    }
    return 0;
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
