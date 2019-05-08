/*
 * File: mid.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#ifndef MID_H
#define MID_H
#include "../lib/bc127.h"
#include "../lib/config.h"
#include "../lib/event.h"
#include "../lib/ibus.h"
#include "../lib/log.h"
#include "../lib/timer.h"

#define MID_MODE_OFF 0
#define MID_MODE_ACTIVE 1

/*
 * MIDContext_t
 *  This is a struct to hold the context of the MID UI implementation
 *  BC127_t *bt: A pointer to the Bluetooth struct
 *  IBus_t *ibus: A pointer to the IBus struct
 *  mode: Track the state of the radio to see what we should display to the user.
 *  seekMode: The current seek mode (forwards or backwards)
 */
typedef struct MIDContext_t {
    IBus_t *ibus;
    BC127_t *bt;
    uint8_t trash; /* Something keeps overflowing to this memory address :( */
    int8_t btDeviceIndex;
    uint8_t mode;
    uint8_t screenUpdated;
} MIDContext_t;
void MIDBC127MetadataUpdate(void *, unsigned char *);
void MIDBC127PlaybackStatus(void *, unsigned char *);
void MIDInit(BC127_t *, IBus_t *);
void MIDIBusCDChangerStatus(void *, unsigned char *);
void MIDIIBusRADMIDDisplayText(void *, unsigned char *);
#endif /* MID_H */
