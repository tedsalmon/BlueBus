/*
 * File:   log.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Logging mechanisms that we can use throughout the project
 */
#ifndef LOG_H
#define LOG_H
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "../mappings.h"
#include "config.h"
#include "timer.h"
#include "uart.h"
// Metadata is the largest single buffer at 384 bytes, so add another 32 bytes
// to that in order to get a usable buffer size
#define LOG_MESSAGE_SIZE 416
#define LOG_SOURCE_BT CONFIG_DEVICE_LOG_BT
#define LOG_SOURCE_IBUS CONFIG_DEVICE_LOG_IBUS
#define LOG_SOURCE_SYSTEM CONFIG_DEVICE_LOG_SYSTEM
#define LOG_SOURCE_UI CONFIG_DEVICE_LOG_UI

#define LOG_EVENT_STATUS 129

void LogMessage(const char *, const char *);
void LogRaw(const char *, ...);
void LogRawDebug(uint8_t, const char *, ...);
void LogError(const char *, ...);
void LogDebug(uint8_t, const char *, ...);
void LogInfo(uint8_t, const char *, ...);
void LogWarning(const char *, ...);
#endif /* LOG_H */
