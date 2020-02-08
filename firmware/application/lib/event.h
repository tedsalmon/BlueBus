/*
 * File: event.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement an event system so that modules can interact with each other
 */
#ifndef EVENT_H
#define EVENT_H
#define EVENT_MAX_CALLBACKS 192
#include <stdint.h>
#include <string.h>
typedef struct Event_t {
    uint8_t type;
    void *context;
    void (*callback) (void *, unsigned char *);
} Event_t;
void EventRegisterCallback(uint8_t, void *, void *);
uint8_t EventUnregisterCallback(uint8_t, void *);
void EventTriggerCallback(uint8_t, unsigned char *);
#endif /* EVENT_H */
