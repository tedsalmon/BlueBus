/*
 * File:   log.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Logging mechanisms that we can use throughout the project
 */
#ifndef LOG_H
#define LOG_H
#include <stdarg.h>
#include <stdio.h>
#include "../mappings.h"
#include "config.h"
#include "timer.h"
#include "uart.h"
#define LOG_SOURCE_BT CONFIG_DEVICE_LOG_BT
#define LOG_SOURCE_IBUS CONFIG_DEVICE_LOG_IBUS
#define LOG_SOURCE_SYSTEM CONFIG_DEVICE_LOG_SYSTEM
#define LOG_SOURCE_UI CONFIG_DEVICE_LOG_UI
void LogMessage(const char *, char *);
void LogRaw(const char *, ...);
void LogRawDebug(uint8_t, const char *, ...);
void LogError(const char *, ...);
void LogDebug(uint8_t, const char *, ...);
void LogInfo(uint8_t, const char *, ...);
void LogWarning(const char *, ...);
#endif /* LOG_H */
