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
    uart.rxQueue = CharQueueInit();
    uart.moduleIndex = uartModule - 1;
    uart.rxError = 0;
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
        case 3:
            uart.registers = (volatile UART *) &U3MODE;
            _U3RXR = rxPin;
            break;
        case 4:
            uart.registers = (volatile UART *) &U4MODE;
            _U4RXR = rxPin;
            break;
    }
    // Set the TX Pin Mode
    UtilsSetRPORMode(txPin, UART_TX_MODES[uart.moduleIndex]);
    __builtin_write_OSCCONL(OSCCON & 0x40);
    //Set the BAUD Rate
    uart.registers->uxbrg = baudRate;
    // Disable the TX ISR, since we handle it manually and Enable the RX ISR
    SetUARTTXIE(uart.moduleIndex, 0);
    SetUARTRXIE(uart.moduleIndex, 1);
    // Set the ISR Flag to disabled for RX (as it should be when the hardware
    // buffers are empty) and disable the TX ISR Flag
    SetUARTRXIF(uart.moduleIndex, 0);
    SetUARTTXIF(uart.moduleIndex, 0);
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
    // Set the Interrupt Priority
    SetUARTRXIP(uart.moduleIndex, rxPriority);
    SetUARTTXIP(uart.moduleIndex, txPriority);
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
        case 3:
            _U3RXR = 0;
            break;
        case 4:
            _U4RXR = 0;
            break;
    }
    UtilsSetRPORMode(uart->txPin, 0);
    __builtin_write_OSCCONL(OSCCON & 0x40);
    // Disable ISRs for the module
    SetUARTTXIE(uart->moduleIndex, 0);
    SetUARTRXIE(uart->moduleIndex, 0);
    SetUARTRXIF(uart->moduleIndex, 0);
    SetUARTTXIF(uart->moduleIndex, 0);
    //Set the BAUD Rate back to 0
    uart->registers->uxbrg = 0;
    // Disable UART
    uart->registers->uxmode = 0;
    // Disable transmit and receive on the module
    uart->registers->uxsta = 0;
    // Pull down the RX and TX pins for the module
    switch (uartModule) {
        case IBUS_UART_MODULE:
            IBUS_UART_RX_PIN_MODE = 1;
            IBUS_UART_TX_PIN_MODE = 1;
            break;
        case BT_UART_MODULE:
            BT_UART_RX_PIN_MODE = 1;
            BT_UART_TX_PIN_MODE = 1;
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

static uint8_t UARTRXInterruptHandler(uint8_t moduleIndex)
{
    UART_t *uart = UARTModules[moduleIndex];
    if (uart == 0) {
        // Nothing to do -- Clear the interrupt flag
        SetUARTRXIF(moduleIndex, 0);
        return 0;
    }
    // While there is data in the RX buffer
    while ((uart->registers->uxsta & 0x1) == 1) {
        // No frame or parity errors
        if ((uart->registers->uxsta & 0xC) == 0) {
            // Clear the buffer overflow error, if it exists
            if (CHECK_BIT(uart->registers->uxsta, 1) != 0) {
                uart->rxError ^= UART_ERR_OERR;
                uart->registers->uxsta ^= 0x2;
            }
            CharQueueAdd(&uart->rxQueue, uart->registers->uxrxreg);
        } else {
            // Set a "General" Error
            uart->rxError ^= UART_ERR_GERR;
            // Clear the buffer overflow error, if it is set
            if (CHECK_BIT(uart->registers->uxsta, 1) != 0) {
                uart->rxError ^= UART_ERR_OERR;
                uart->registers->uxsta ^= 0x2;
            }
            if (CHECK_BIT(uart->registers->uxsta, 2) != 0) {
                uart->rxError ^= UART_ERR_FERR;
            }
            if (CHECK_BIT(uart->registers->uxsta, 3) != 0) {
                uart->rxError ^= UART_ERR_PERR;
            }
            // Clear the byte in the RX buffer
            // DO NOT use uart->registers->uxrxreg. As of xc16 2.0.0 it will
            // not clear the byte when an error has occurred
            if (moduleIndex == 0) {
                U1RXREG;
            }
            if (moduleIndex == 1) {
                U2RXREG;
            }
            if (moduleIndex == 2) {
                U3RXREG;
            }
            if (moduleIndex == 3) {
                U4RXREG;
            }
        }
        if ((uart->registers->uxsta & 0x1) == 0) {
            // Buffer is clear -- immediately clear the interrupt flag
            SetUARTRXIF(moduleIndex, 0);
            return 0;
        }
    }
    SetUARTRXIF(moduleIndex, 0);
    return 0;
}

void UARTReportErrors(UART_t *uart)
{
    if (uart->rxError != 0) {
        long long unsigned int ts = (long long unsigned int) TimerGetMillis();
        LogRawDebug(
            LOG_SOURCE_SYSTEM,
            "[%llu] ERROR: UART[%d]: ",
            ts,
            uart->moduleIndex + 1
        );
        if ((uart->rxError & UART_ERR_GERR) != 0) {
            LogRawDebug(LOG_SOURCE_SYSTEM, "GERR ");
        }
        if ((uart->rxError & UART_ERR_OERR) != 0) {
            LogRawDebug(LOG_SOURCE_SYSTEM, "OERR ");
        }
        if ((uart->rxError & UART_ERR_FERR) != 0) {
            LogRawDebug(LOG_SOURCE_SYSTEM, "FERR ");
        }
        if ((uart->rxError & UART_ERR_PERR) != 0) {
            LogRawDebug(LOG_SOURCE_SYSTEM, "PERR ");
        }
        LogRawDebug(LOG_SOURCE_SYSTEM, "\r\n");
        uart->rxError = 0;
    }
}

void UARTRXQueueReset(UART_t *uart)
{
    CharQueueReset(&uart->rxQueue);
}

void UARTSendChar(UART_t *uart, unsigned char data)
{
    uart->registers->uxtxreg = data;
    // Wait for the data to leave the tx buffer
    while ((uart->registers->uxsta & (1 << 9)) != 0);
}

void UARTSendData(UART_t *uart, unsigned char *data, uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length; i++) {
        uart->registers->uxtxreg = data[i];
        // Wait for the data to leave the tx buffer
        while ((uart->registers->uxsta & (1 << 9)) != 0);
    }
}

void UARTSendString(UART_t *uart, char *data)
{
    uint16_t stringLength = strlen(data);
    uint16_t i = 0;
    for (i = 0; i < stringLength; i++) {
        char c = data[i];
        // Print only readable and newline characters
        if ((c >= 0x20 && c <= 0x7E) || c == 0x0D || c == 0x0A) {
            uart->registers->uxtxreg = c;
            // Wait for the data to leave the tx buffer
            while ((uart->registers->uxsta & (1 << 9)) != 0);
        }
    }
}

/*
 * Define the RX interrupt handlers that will pass off to our handler above
 */
void __attribute__((__interrupt__, auto_psv)) _AltU1RXInterrupt()
{
    UARTRXInterruptHandler(0);
}
void __attribute__((__interrupt__, auto_psv)) _AltU2RXInterrupt()
{
    UARTRXInterruptHandler(1);
}
void __attribute__((__interrupt__, auto_psv)) _AltU3RXInterrupt()
{
    UARTRXInterruptHandler(2);
}
void __attribute__((__interrupt__, auto_psv)) _AltU4RXInterrupt()
{
    UARTRXInterruptHandler(3);
}