/*
 * File:   debug.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of logging mechanisms that we can use throughout the project
 */
#include "../io_mappings.h"
#include "uart.h"

/**
 * LogDebug()
 *     Description:
 *         Send a debug message over the system UART
 *     Params:
 *         char *data
 *     Returns:
 *         void
 */
void LogDebug(char *data)
{
    struct UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (debugger != 0x00 ) {
        UARTSendString(debugger, "DEBUG: ");
        UARTSendString(debugger, data);
    }
}

/**
 * LogError()
 *     Description:
 *         Send an error message over the system UART
 *     Params:
 *         char *data
 *     Returns:
 *         void
 */
void LogError(char *data)
{
    struct UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (debugger != 0x00 ) {
        UARTSendString(debugger, "ERROR: ");
        UARTSendString(debugger, data);
    }
}

/**
 * LogInfo()
 *     Description:
 *         Send an info message over the system UART
 *     Params:
 *         char *data
 *     Returns:
 *         void
 */
void LogInfo(char *data)
{
    struct UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (debugger != 0x00 ) {
        UARTSendString(debugger, "INFO: ");
        UARTSendString(debugger, data);
    }
}

/**
 * LogWarning()
 *     Description:
 *         Send a warning message over the system UART
 *     Params:
 *         char *data
 *     Returns:
 *         void
 */
void LogWarning(char *data)
{
    struct UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (debugger != 0x00 ) {
        UARTSendString(debugger, "WARNING: ");
        UARTSendString(debugger, data);
    }
}
