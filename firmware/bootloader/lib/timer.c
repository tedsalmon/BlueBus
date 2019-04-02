/*
 * File: timer.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */
#include "timer.h"
volatile uint32_t TimerCurrentMillis = 0;

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
    SetTIMERIF(TIMER_INDEX, 0);
}
