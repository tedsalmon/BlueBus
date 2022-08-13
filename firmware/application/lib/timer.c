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
 * TimerDelayMicroseconds()
 *     Description:
 *         Block for the given amount of microseconds. You really should not
 *         block the application for more than 10us at a time.
 *     Params:
 *         uint16_t delay - Delay for x amoount of microseconds
 *     Returns:
 *         void
 */
void TimerDelayMicroseconds(uint16_t delay)
{
    TMR2 = 0;
    // Set the delay to delay * 16 (ticks per microsecond)
    PR2 = delay * 16;
    // Reset interrupt flag
    SetTIMERIF(2, 0);
    T2CONbits.TON = 1;
    while (!IFS0bits.T2IF);
    T2CONbits.TON = 0;
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
        if (t->ticks >= t->interval && t->task != 0) {
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
 * TimerUnregisterScheduledTask()
 *     Description:
 *         Unregister a previously scheduled task
 *     Params:
 *         void *task - A pointer to the function to call
 *     Returns:
 *         uint8_t - The status code
 */
uint8_t TimerUnregisterScheduledTask(void *task)
{
    uint8_t idx;
    for (idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[idx];
        if (t->task == task) {
            memset((void *)t, 0, sizeof(TimerScheduledTask_t));
            return 0;
        }
    }
    return 1;
}

/**
 * TimerUnregisterScheduledTaskById()
 *     Description:
 *         Unregister a previously scheduled task using the given ID
 *     Params:
 *         uint8_t - The index of the scheduled task in the tasks array
 *     Returns:
 *         void
 */
void TimerUnregisterScheduledTaskById(uint8_t taskId)
{
    volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
    memset((void *)t, 0, sizeof(TimerScheduledTask_t));
}

/**
 * TimerResetScheduledTask()
 *     Description:
 *         Reset the ticks on a given task
 *     Params:
 *         uint8_t - The index of the scheduled task in the tasks array
 *     Returns:
 *         void
 */
void TimerResetScheduledTask(uint8_t taskId)
{
    volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
    if (t->task != 0) {
        t->ticks = 0;
    }
}


/**
 * TimerSetTaskInterval()
 *     Description:
 *         Change the timer interval
 *     Params:
 *         uint8_t taskId - The index of the scheduled task in the tasks array
 *         uint16_t interval - The number of milliseconds to elapse before calling
 *     Returns:
 *         void
 */
void TimerSetTaskInterval(uint8_t taskId, uint16_t interval)
{
    volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[taskId];
    if (t->task != 0) {
        t->interval = interval;
    }
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
    if (t->task != 0) {
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
void __attribute__((__interrupt__, auto_psv)) _AltT1Interrupt(void)
{
    TimerCurrentMillis++;
    uint8_t idx;
    for (idx = 0; idx < TimerRegisteredTasksCount; idx++) {
        volatile TimerScheduledTask_t *t = &TimerRegisteredTasks[idx];
        if (t->task != 0) {
            TimerRegisteredTasks[idx].ticks++;
        }
    }
    SetTIMERIF(TIMER_INDEX, 0);
}
