/*
 * File: timer.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */
#include "timer.h"
volatile uint32_t TimerCurrentMillis = 0;
TimerScheduledTask_t TimerRegisteredTasks[TIMER_TASKS_MAX];
uint8_t TimerRegisteredTasksCount = 0;

/**
 * TimerInit()
 *     Description:
 *         Initialize the system Timer (Timer3)
 *     Params:
 *         None
 *     Returns:
 *         void
 */
void TimerInit()
{
    IPC2bits.T3IP = TIMER_INTERRUPT_PRIORITY;
    IFS0bits.T3IF = 0;
    TMR3 = 0;
    PR3 = PR3_SETTING;
    T3CON = TIMER_ON | STOP_TIMER_IN_IDLE_MODE | TIMER_SOURCE_INTERNAL | GATED_TIME_DISABLED | TIMER_16BIT_MODE | CLOCK_DIVIDER;
    IEC0bits.T3IE = 1;
}

/**
 * TimerGetMillis()
 *     Description:
 *         Return the number of elapsed milliseconds since boot
 *     Params:
 *         None
 *     Returns:
 *         uint32_t - The milliseconds since boot
 */
uint32_t TimerGetMillis()
{
    return (uint32_t) TimerCurrentMillis;
}

/**
 * TimerRegisterScheduledTask()
 *     Description:
 *         Register a function to be called at a given interval with the given
 *         context
 *     Params:
 *         void *task - A pointer to the function to call
 *         void *ctx - A pointer to the context for which to pass to the function
 *         uint16_t interval - The number of milliseconds to elapse before calling
 *     Returns:
 *         uint8_t - The index of the scheduled task in the tasks array
 */
uint8_t TimerRegisterScheduledTask(void *task, void *ctx, uint16_t interval)
{
    TimerScheduledTask_t scheduledTask;
    scheduledTask.task = task;
    scheduledTask.context = ctx;
    scheduledTask.ticks = 0;
    scheduledTask.interval = interval;
    TimerRegisteredTasks[TimerRegisteredTasksCount++] = scheduledTask;
    return TimerRegisteredTasksCount - 1;
}

/**
 * TimerTriggerScheduledTask()
 *     Description:
 *         Call a given scheduled task immediately and reset the interval count
 *     Params:
 *         uint8_t - The index of the scheduled task in the tasks array
 *     Returns:
 *         void
 */
void TimerTriggerScheduledTask(uint8_t taskId)
{
    TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
    if (t != 0) {
        // Prevent it from executing immediately
        t->ticks = 0;
        t->task(t->context);
        // Reset the ticks so it runs exactly `interval` times before firing
        t->ticks = 0;
    }
}
/**
 * T3Interrupt
 *     Description:
 *         Update the milliseconds since boot and run through the scheduled
 *         tasks. If a task is not due, increment the interval counter.
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void)
{
    TimerCurrentMillis++;
    uint8_t idx;
    for (idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        TimerScheduledTask_t *t = &TimerRegisteredTasks[idx];
        t->ticks++;
        if (t->ticks >= t->interval) {
            t->task(t->context);
            t->ticks = 0;
        }
    }
    IFS0bits.T3IF = 0;
}
