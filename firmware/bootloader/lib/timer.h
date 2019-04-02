/*
 * File: timer.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */
#ifndef TIMER_H
#define TIMER_H
#define SYS_CLOCK 16000000
#define STOP_TIMER_IN_IDLE_MODE 0x2000
#define TIMER_SOURCE_INTERNAL 0x0000
#define TIMER_ON 0x8000
#define GATED_TIME_DISABLED 0x0000
#define TIMER_16BIT_MODE 0x0000
#define TIMER_PRESCALER 0x0000
#define TIMER_INTERRUPT_PRIORITY 0x0002
#define CLOCK_DIVIDER TIMER_PRESCALER
#define PR1_SETTING (SYS_CLOCK / 1000 / 1)
#define TIMER_TASKS_MAX 16
#define TIMER_INDEX 0
#include <stdint.h>
#include <xc.h>
#include "sfr_setters.h"

void TimerInit();
uint32_t TimerGetMillis();
#endif /* TIMER_H */
