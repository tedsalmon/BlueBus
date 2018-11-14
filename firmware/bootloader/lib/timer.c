/*
 * File: timer.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */
#include "timer.h"
uint32_t TimerCurrentMillis = 0;

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
}

/**
 * TimerUpdate()
 *     Description:
 *         Since TMR1 increments every cycle, we use the PR_SETTING (16,000)
 *         as the basis of 1ms. If TMR1 is >= PR_SETTING, then 1ms has passed.
 *         This allows us to get away without having to use an ISR to track
 *         elapsed time.
 *     Params:
 *         None
 *     Returns:
 *         None
 */
void TimerUpdate()
{
    if (TMR1 >= PR_SETTING) {
        TMR1 = 0;
        TimerCurrentMillis++;
    }
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
    return TimerCurrentMillis;
}
