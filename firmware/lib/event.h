/*
 * File: event.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement an event system so that modules can interact with each other
 */
#ifndef EVENTS_H
#define EVENTS_H
#include "bc127.h"
#include "ibus.h"
#define MAX_EVENT_CALLBACKS 128

Event_t EVENT_CALLBACKS[MAX_EVENT_CALLBACKS];

typedef struct EventContext_t {
    struct BC127_t *bt;
    struct IBus_t *ibus;
    unsigned char data[128];
} EventContext_t;

typedef struct Event_t {
    uint8_t eventType;
    void (*callback)(char *, EventContext_t *);
} Event_t;

void EventRegisterCallback(char *, void *);
void EventTriggerCallback(char *, EventContext_t *);
#endif /* EVENTS_H */
