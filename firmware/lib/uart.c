/*
 * File:   uart.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the PIC UART modules in a structured way to allow
 *     easier, and consistent data r/w
 */
#include "uart.h"

/* Return a reprogrammable port register */
#define GET_RPOR(num) (((uint16_t *) &RPOR0) + num)

/* Check if a bit is set */
#define CHECK_BIT(var, pos) ((var) & (1 <<(pos)))

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

static UART_t *UARTModules[UART_MODULES_COUNT];
static uint8_t UARTModulesStatus[UART_MODULES_COUNT];

// These values constitute the TX mode for each UART module
static const uint8_t UART_TX_MODES[] = {3, 5, 19, 21};

UART_t UARTInit(
    uint8_t uartModule,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t rxPriority,
    uint8_t txPriority,
    uint8_t baudRate,
    uint8_t parity
) {
    UART_t uart;
    uart.txQueue = CharQueueInit();
    uart.rxQueue = CharQueueInit();
    uart.moduleIndex = uartModule - 1;
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
        case 3:
            uart.registers = (volatile UART *) &U3MODE;
            _U3RXR = rxPin;
            break;
        case 4:
            uart.registers = (volatile UART *) &U4MODE;
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
    // Disable the TX ISR, since we handle it manually and Enable the RX ISR
    SetUARTTXIE(uart.moduleIndex, 0);
    SetUARTRXIE(uart.moduleIndex, 1);
    // Set the ISR Flag to disabled for TX (as it should be when the hardware
    // buffers are empty) and enable the RX ISR Flag
    SetUARTRXIF(uart.moduleIndex, 0);
    SetUARTTXIF(uart.moduleIndex, 1);
    // Enable UART, keep the module in 3-wire mode
    uart.registers->uxmode ^= 0b1000000000000000;
    if (parity == UART_PARITY_EVEN) {
        uart.registers->uxmode ^= 0b0000000000000010;
    }
    // Enable transmit and receive on the module
    uart.registers->uxsta ^= 0b0001010000000000;
    // Set the Interrupt Priority
    SetUARTRXIP(uart.moduleIndex, rxPriority);
    SetUARTTXIP(uart.moduleIndex, txPriority);
    return uart;
}

void UARTAddModuleHandler(UART_t *uart)
{
    UARTModules[uart->moduleIndex] = uart;
}

UART_t * UARTGetModuleHandler(uint8_t moduleIndex)
{
    return UARTModules[moduleIndex - 1];
}

uint8_t UARTGetModuleState(UART_t *uart)
{
    return UARTModulesStatus[uart->moduleIndex];
}

void UARTSetModuleStateByIdx(uint8_t moduleIndex, uint8_t state)
{
    UARTModulesStatus[moduleIndex] = state;
}

void UARTSetModuleState(UART_t *uart, uint8_t state)
{
    UARTModulesStatus[uart->moduleIndex] = state;
}

static void UARTRXInterruptHandler(uint8_t moduleIndex)
{
    UARTSetModuleStateByIdx(moduleIndex, UART_STATE_RX);
    UART_t *uart = UARTModules[moduleIndex];
    if (uart != 0) {
        // While there's data on the RX buffer
        while (uart->registers->uxsta & 0x1) {
            // Reading the byte is sometimes a requirement to clear an error
            unsigned char byte = uart->registers->uxrxreg;
            // No frame or parity errors
            if ((uart->registers->uxsta & 0xC) == 0) {
                // Clear the buffer overflow error, if it exists
                if (CHECK_BIT(uart->registers->uxsta, 1) != 0) {
                    uart->registers->uxsta ^= 0x2;
                }
                CharQueueAdd(&uart->rxQueue, byte);
            } else {
                // Clear the buffer overflow error, if it is set
                if (CHECK_BIT(uart->registers->uxsta, 1) != 0) {
                    uart->registers->uxsta ^= 0x2;
                } else {
                    LogError(
                        "UART[%d]: FERR/PERR -> 0x%X",
                        moduleIndex + 1,
                        uart->registers->uxsta
                    );
                }
            }
        }
    }
    // Clear the interrupt flag unconditionally, since we will be recalled to
    // this handler if there's additional data
    SetUARTRXIF(moduleIndex, 0);
}

static void UARTTXInterruptHandler(uint8_t moduleIndex)
{
    UARTSetModuleStateByIdx(moduleIndex, UART_STATE_TX);
    UART_t *uart = UARTModules[moduleIndex];
    if (uart != 0) {
        while (uart->txQueue.size > 0) {
            // TXIF is 1 if the queue is empty, set it before pushing data
            SetUARTTXIF(moduleIndex, 0);
            unsigned char c = CharQueueNext(&uart->txQueue);
            uart->registers->uxtxreg = c;
            // Wait for the data to leave the tx buffer
            while ((uart->registers->uxsta & (1 << 9)) != 0);
        }
    }
    // Disable the interrupt after flushing the queue
    SetUARTTXIE(moduleIndex, 0);
    // The data was sent, so set us idle again
    UARTSetModuleStateByIdx(moduleIndex, UART_STATE_IDLE);
}

void UARTSendData(UART_t *uart, unsigned char *data)
{
    unsigned char c;
    while ((c = *data++)) {
        CharQueueAdd(&uart->txQueue, c);
    }
    // Set the interrupt flag
    SetUARTTXIE(uart->moduleIndex, 1);
}

void UARTSendString(UART_t *uart, char *data)
{
    char c;
    while ((c = *data++)) {
        // Print only readable and newline characters
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
    UARTRXInterruptHandler(0);
}
void __attribute__((__interrupt__, auto_psv)) _U1TXInterrupt()
{
    UARTTXInterruptHandler(0);
}
void __attribute__((__interrupt__, auto_psv)) _U2RXInterrupt()
{
    UARTRXInterruptHandler(1);
}
void __attribute__((__interrupt__, auto_psv)) _U2TXInterrupt()
{
    UARTTXInterruptHandler(1);
}
void __attribute__((__interrupt__, auto_psv)) _U3RXInterrupt()
{
    UARTRXInterruptHandler(2);
}
void __attribute__((__interrupt__, auto_psv)) _U3TXInterrupt()
{
    UARTTXInterruptHandler(2);
}
void __attribute__((__interrupt__, auto_psv)) _U4RXInterrupt()
{
    UARTRXInterruptHandler(3);
}
void __attribute__((__interrupt__, auto_psv)) _U4TXInterrupt()
{
    UARTTXInterruptHandler(3);
}
