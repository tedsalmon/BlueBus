/*
 * File: timer.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a timer to keep track of events
 */
#ifndef TIMER_H
#define TIMER_H
#define SYS_CLOCK 16000000
#define STOP_TIMER_IN_IDLE_MODE 0x2000
#define TIMER_SOURCE_INTERNAL 0x0000
#define TIMER_SOURCE_EXTERNAL 0x0002
#define TIMER_ON 0x8000
#define GATED_TIME_DISABLED 0x0000
#define TIMER_16BIT_MODE 0x0000

#define TIMER_PRESCALER_1 0x0000
#define TIMER_PRESCALER_8 0x0010
#define TIMER_PRESCALER_64 0x0020
#define TIMER_PRESCALER_256 0x0030
#define TIMER_INTERRUPT_PRIORITY 0x0004

#define CLOCK_DIVIDER TIMER_PRESCALER_1
#define PR3_SETTING (SYS_CLOCK / 1000 / 1)

#if (PR3_SETTING > 0xFFFF)
#undef CLOCK_DIVIDER
#undef PR3_SETTING
#define CLOCK_DIVIDER TIMER_PRESCALER_8
#define PR3_SETTING (SYSTEM_PERIPHERAL_CLOCK/1000/8)
#endif

#if (PR3_SETTING > 0xFFFF)
#undef CLOCK_DIVIDER
#undef PR3_SETTING
#define CLOCK_DIVIDER TIMER_PRESCALER_64
#define PR3_SETTING (SYSTEM_PERIPHERAL_CLOCK/1000/64)
#endif

#if (PR3_SETTING > 0xFFFF)
#undef CLOCK_DIVIDER
#undef PR3_SETTING
#define CLOCK_DIVIDER TIMER_PRESCALER_256
#define PR3_SETTING (SYSTEM_PERIPHERAL_CLOCK/1000/256)
#endif
#define TIMER_TASKS_MAX 16

typedef struct TimerScheduledTask_t {
    uint32_t lastRun;
    uint16_t timeout;
    void *context;
    void (*task)(void *);
} TimerScheduledTask_t;

void TimerInit();
void TimerRegisterScheduledTask(void *, void *, uint16_t);
uint32_t TimerGetMillis();
#endif /* TIMER_H */
