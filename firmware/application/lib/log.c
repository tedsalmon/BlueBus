/*
 * File:   log.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of logging mechanisms that we can use throughout the project
 */
#include "log.h"

/**
 * LogMessage()
 *     Description:
 *         Send a message over the system UART, for the given log level.
 *         Implicitly adds CRLF
 *     Params:
 *         const char *type
 *         const char *data
 *     Returns:
 *         void
 */
void LogMessage(const char *type, const char *data)
{
    UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (debugger != 0) {
        char output[LOG_MESSAGE_SIZE] = {0};
        long long unsigned int ts = (long long unsigned int) TimerGetMillis();
        snprintf(output, LOG_MESSAGE_SIZE - 1 , "[%llu] %s: %s\r\n", ts, type, data);
        UARTSendString(debugger, output);
    }
}

/**
 * LogRaw()
 *     Description:
 *         Sends the given data over to the debug UART.
 *     Params:
 *         const char *format - The string format
 *         va_args ...
 *     Returns:
 *         void
 */
void LogRaw(const char *format, ...)
{
    UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    if (debugger != 0) {
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        UARTSendString(debugger, buffer);
    }
}

/**
 * LogRawDebug()
 *     Description:
 *         Sends the given data over to the debug UART.
 *     Params:
 *         uint8_t source - The source system
 *         const char *format - The string format
 *         va_args ...
 *     Returns:
 *         void
 */
void LogRawDebug(uint8_t source, const char *format, ...)
{
    UART_t *debugger = UARTGetModuleHandler(SYSTEM_UART_MODULE);
    unsigned char canLog = ConfigGetLog(source);
    if (debugger != 0 && canLog != 0) {
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        UARTSendString(debugger, buffer);
    }
}

/**
 * LogDebug()
 *     Description:
 *         Send a debug message over the system UART
 *     Params:
 *         uint8_t source - The source system
 *         const char *format
 *         va_args ...
 *     Returns:
 *         void
 */
void LogDebug(uint8_t source, const char *format, ...)
{
    unsigned char canLog = ConfigGetLog(source);
    if (canLog != 0) {
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        va_end(args);
        LogMessage("DEBUG", buffer);
    }
}

/**
 * LogError()
 *     Description:
 *         Send an error message over the system UART
 *         va_args ...
 *     Params:
 *         const char *format
 *         va_args ...
 *     Returns:
 *         void
 */
void LogError(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    LogMessage("ERROR", buffer);
}

/**
 * LogInfo()
 *     Description:
 *         Send an info message over the system UART
 *         va_args ...
 *     Params:
 *         uint8_t source - The source system
 *         const char *format
 *         va_args ...
 *     Returns:
 *         void
 */
void LogInfo(uint8_t source, const char *format, ...)
{
    unsigned char canLog = ConfigGetLog(source);
    if (canLog != 0) {
        char buffer[LOG_MESSAGE_SIZE] = {0};
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
        va_end(args);
        LogMessage("INFO", buffer);
    }
}

/**
 * LogWarning()
 *     Description:
 *         Send a warning message over the system UART
 *     Params:
 *         const char *format
 *         va_args ...
 *     Returns:
 *         void
 */
void LogWarning(const char *format, ...)
{
    char buffer[LOG_MESSAGE_SIZE] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    LogMessage("WARNING", buffer);
}
