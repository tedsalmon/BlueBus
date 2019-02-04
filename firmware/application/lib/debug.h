/*
 * File:   debug.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Logging mechanisms that we can use throughout the project
 */
#ifndef DEBUG_H
#define DEBUG_H
#include <stdarg.h>
#include <stdio.h>
#include "../mappings.h"
#include "timer.h"
#include "uart.h"
void LogMessage(const char *, char *);
void LogRaw(const char *, ...);
void LogError(const char *, ...);
void LogDebug(const char *, ...);
void LogInfo(const char *, ...);
void LogWarning(const char *, ...);
#endif /* DEBUG_H */
