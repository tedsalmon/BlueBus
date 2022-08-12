/*
 * File: timer.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can
 *     time events in the application. Implement a scheduled task queue.
 */
#include "timer.h"
volatile uint32_t TimerCurrentMillis = 0;
volatile uint8_t TimerEnableLEDValue = TIMER_LED_DISABLED;

/**
 * TimerInit()
 *     Description:
 *         Initialize the system Timer (Timer1) and the LED Timer (Timer2)
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
    SetTIMERIP(TIMER_1_INDEX, TIMER_1_INTERRUPT_PRIORITY);
    SetTIMERIF(TIMER_1_INDEX, 0);
    SetTIMERIE(TIMER_1_INDEX, 1);
    T2CON = 0;
    T2CON = TIMER_ON | STOP_TIMER_IN_IDLE_MODE | TIMER_SOURCE_INTERNAL | GATED_TIME_DISABLED | TIMER_PRESCALER_256 | TIMER_16BIT_MODE | TIMER_SOURCE_INTERNAL;
    PR2 = PR2_SETTING;
    SetTIMERIP(TIMER_2_INDEX, TIMER_2_INTERRUPT_PRIORITY);
    SetTIMERIF(TIMER_2_INDEX, 0);
    SetTIMERIE(TIMER_2_INDEX, 1);
}

/**
 * TimerDestory()
 *     Description:
 *         Disable both timers that we initialized originally
 *     Params:
 *         None
 *     Returns:
 *         void
 */
void TimerDestroy()
{
    T1CON = 0;
    PR1 = 0;
    SetTIMERIP(TIMER_1_INDEX, 0);
    SetTIMERIF(TIMER_1_INDEX, 0);
    SetTIMERIE(TIMER_1_INDEX, 0);
    T2CON = 0;
    PR2 = 0;
    SetTIMERIP(TIMER_2_INDEX, 0);
    SetTIMERIF(TIMER_2_INDEX, 0);
    SetTIMERIE(TIMER_2_INDEX, 0);
}

/**
 * TimerDisableLED()
 *     Description:
 *         Disable the LED Flashing Timer
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void TimerDisableLED()
{
    TimerEnableLEDValue = TIMER_LED_DISABLED;
    ON_LED = 0;
}

/**
 * TimerEnableLED()
 *     Description:
 *         Enable the LED Flashing Timer
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void TimerEnableLED()
{
    TimerEnableLEDValue = TIMER_LED_OFF;
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
    SetTIMERIF(TIMER_1_INDEX, 0);
}

/**
 * T2Interrupt
 *     Description:
 *         Flash the LED every half second
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void __attribute__((__interrupt__, auto_psv)) _T2Interrupt(void)
{
    if (TimerEnableLEDValue != TIMER_LED_DISABLED) {
        if (TimerEnableLEDValue == TIMER_LED_ON) {
            ON_LED = 0;
            TimerEnableLEDValue = TIMER_LED_OFF;
        } else {
            ON_LED = 1;
            TimerEnableLEDValue = TIMER_LED_ON;
        }
    }
    SetTIMERIF(TIMER_2_INDEX, 0);
}
