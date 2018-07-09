/*
 * File: event.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement an event system so that modules can interact with each other
 */
#include "event.h"
#include "debug.h"
#include "../handler.h"
struct Event_t EVENT_CALLBACKS[EVENT_MAX_CALLBACKS];
uint8_t EVENT_CALLBACKS_COUNT = 0;

void EventRegisterCallback(uint8_t eventType, void *callback, void *context)
{
    struct Event_t cb;
    cb.type = eventType;
    cb.callback = callback;
    cb.context = context;
    EVENT_CALLBACKS[EVENT_CALLBACKS_COUNT++] = cb;
}

void EventTriggerCallback(uint8_t eventType, unsigned char *data)
{
    uint8_t idx;
    for (idx = 0; idx < EVENT_CALLBACKS_COUNT; idx++) {
        struct Event_t cb = EVENT_CALLBACKS[idx];
        if (cb.type == eventType) {
            cb.callback(cb.context, data);
        }
    }
}
