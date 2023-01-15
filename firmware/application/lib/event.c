/*
 * File: event.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement an event system so that modules can interact with each other
 */
#include "event.h"
volatile Event_t EVENT_CALLBACKS[EVENT_MAX_CALLBACKS];
uint8_t EVENT_CALLBACKS_COUNT = 0;

/**
 * EventRegisterCallback()
 *     Description:
 *         Adds a callback of event type to the event queue. Any triggers of
 *         this event type will result in the execution of the function,
 *         with the given context being passed through.
 *     Params:
 *         uint8_t eventType
 *         void *callback - Pointer to the function to call when triggered
 *         void *context - The object to pass to the function. This needs to be
 *         cast to the appropriate type on the functions end.
 *     Returns:
 *         void
 */
void EventRegisterCallback(uint8_t eventType, void *callback, void *context)
{
    Event_t cb;
    cb.type = eventType;
    cb.callback = callback;
    cb.context = context;
    EVENT_CALLBACKS[EVENT_CALLBACKS_COUNT++] = cb;
}

/**
 * EventUnregisterCallback()
 *     Description:
 *         Unregister a callback
 *     Params:
 *         uint8_t eventType
 *         void *callback - Pointer to the function to call when triggered
 *     Returns:
 *         uint8_t - The status code
 */
uint8_t EventUnregisterCallback(uint8_t eventType, void *callback)
{
    uint8_t idx;
    for (idx = 0; idx < EVENT_CALLBACKS_COUNT; idx++) {
        volatile Event_t *cb = &EVENT_CALLBACKS[idx];
        if (cb->type == eventType &&
            cb->callback == callback
        ) {
            memset((void *) cb, 0, sizeof(Event_t));
            return 0;
        }
    }
    return 1;
}

/**
 * EventTriggerCallback()
 *     Description:
 *         Triggers all registered callbacks of eventType
 *     Params:
 *         uint8_t eventType - The Event type to trigger
 *         unsigned char *data
 *     Returns:
 *         void
 */
void EventTriggerCallback(uint8_t eventType, unsigned char *data)
{
    uint8_t idx;
    for (idx = 0; idx < EVENT_CALLBACKS_COUNT; idx++) {
        volatile Event_t *cb = &EVENT_CALLBACKS[idx];
        if (cb->type == eventType) {
            cb->callback(cb->context, data);
        }
    }
}
