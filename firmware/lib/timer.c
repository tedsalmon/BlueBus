/*
 * File: char_queue.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application
 */
#include <stdint.h>
#include <xc.h>
#include "timer.h"

uint32_t TimerCurrentMillis = 0;
struct TimerScheduledTask_t TimerRegisteredTasks[TIMER_TASKS_MAX];
uint8_t TimerRegisteredTasksCount = 0;

void TimerInit()
{
    IPC2bits.T3IP = TIMER_INTERRUPT_PRIORITY;
    IFS0bits.T3IF = 0;
    TMR3 = 0;
    PR3 = PR3_SETTING;
    T3CON = TIMER_ON | STOP_TIMER_IN_IDLE_MODE | TIMER_SOURCE_INTERNAL | GATED_TIME_DISABLED | TIMER_16BIT_MODE | CLOCK_DIVIDER;
    IEC0bits.T3IE = 1;
}

void TimerRegisterScheduledTask(void *task, void *ctx, uint16_t timeout)
{
    uint32_t now = TimerGetMillis();
    struct TimerScheduledTask_t scheduledTask;
    scheduledTask.lastRun = now;
    scheduledTask.timeout = timeout;
    scheduledTask.context = ctx;
    scheduledTask.task = task;
    TimerRegisteredTasks[TimerRegisteredTasksCount++] = scheduledTask;
}

uint32_t TimerGetMillis()
{
    return TimerCurrentMillis;
}

void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void)
{
    TimerCurrentMillis++;
    uint8_t idx;
    for (idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        struct TimerScheduledTask_t t = TimerRegisteredTasks[idx];
        if ((TimerCurrentMillis - t.lastRun) > t.timeout) {
            t.task(t.context);
        }
    }
    IFS0bits.T3IF = 0;
}
