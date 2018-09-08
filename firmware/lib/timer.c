/*
 * File: timer.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */
#include "timer.h"
volatile uint32_t TimerCurrentMillis = 0;
volatile TimerScheduledTask_t TimerRegisteredTasks[TIMER_TASKS_MAX];
uint8_t TimerRegisteredTasksCount = 0;

/**
 * TimerInit()
 *     Description:
 *         Initialize the system Timer (Timer1)
 *     Params:
 *         None
 *     Returns:
 *         void
 */
void TimerInit()
{
    T1CON = 0;
    T1CON = TIMER_ON | STOP_TIMER_IN_IDLE_MODE | TIMER_SOURCE_INTERNAL | GATED_TIME_DISABLED | TIMER_16BIT_MODE | CLOCK_DIVIDER;
    PR1 = PR1_SETTING;
    SetTIMERIP(TIMER_INDEX, TIMER_INTERRUPT_PRIORITY);
    SetTIMERIF(TIMER_INDEX, 0);
    SetTIMERIE(TIMER_INDEX, 1);
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
 * TimerProcessScheduledTasks()
 *     Description:
 *         Run through the scheduled tasks and run any that are due.
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void TimerProcessScheduledTasks()
{
    uint8_t idx;
    for (idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[idx];
        if (t->ticks >= t->interval) {
            t->task(t->context);
            t->ticks = 0;
        }
    }
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
    volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
    if (t != 0) {
        // Prevent it from executing immediately
        t->ticks = 0;
        t->task(t->context);
        // Reset the ticks so it runs exactly `interval` times before firing
        t->ticks = 0;
    }
}

/**
 * T1Interrupt
 *     Description:
 *         Update the milliseconds since boot. Iterate through the scheduled
 *         tasks and update their ticks.
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void)
{
    TimerCurrentMillis++;
    uint8_t idx;
    for (idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        TimerRegisteredTasks[idx].ticks++;
    }
    SetTIMERIF(TIMER_INDEX, 0);
}
