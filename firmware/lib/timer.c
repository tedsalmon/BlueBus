/*
 * File: char_queue.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer that fires every millisecond so that we can 
 *     time events in the application
 */
#include <xc.h>
#include "timer.h"

void TimerInit()
{
    IPC2bits.T3IP = TIMER_INTERRUPT_PRIORITY;
    IFS0bits.T3IF = 0;
    TMR3 = 0;
    PR3 = PR3_SETTING;
    T3CON = TIMER_ON | STOP_TIMER_IN_IDLE_MODE | TIMER_SOURCE_INTERNAL | GATED_TIME_DISABLED | TIMER_16BIT_MODE | CLOCK_DIVIDER;
    IEC0bits.T3IE = 1;

}

void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void)
{    
    IFS0bits.T3IF = 0;
}
