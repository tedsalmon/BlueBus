/*
 * File: char_queue.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a FIFO queue to store bytes read from UART into
 */
#ifndef CHAR_QUEUE_H
#define CHAR_QUEUE_H
#include <stdint.h>
/* The maximum amount of elements that the queue can hold. It can be this small
 * because the I-Bus specification dictates that messages not be larger than
 * the maximum 8-bit unsigned integer. The BC127 doesn't seem to spout off messages
 * larger than this as well, but sometimes things do get busy on these busses.
 */
#define CHAR_QUEUE_SIZE 255

/**
 * CharQueue_t
 *     Description:
 *         This object holds QUEUE_SIZE amounts of unsigned chars. It operates
 *         with a read and write cursor to keep track of where the next byte
 *         needs to be read from and where the next byte should be added.
 *         Once those cursors are exhausted, meaning they've hit capacity, they
 *         are reset. If data is not removed from the buffer before it hits
 *         capacity, the data will be lost and an error will be logged.
 */
typedef struct CharQueue_t {
    uint8_t capacity;
    uint8_t size;
    uint8_t readCursor;
    uint8_t writeCursor;
    unsigned char data[CHAR_QUEUE_SIZE];
} CharQueue_t;

struct CharQueue_t CharQueueInit();

void CharQueueAdd(CharQueue_t *, unsigned char);
unsigned char CharQueueGet(CharQueue_t *, uint8_t);
unsigned char CharQueueNext(CharQueue_t *);
int16_t CharQueueSeek(CharQueue_t *, unsigned char);
#endif /* CHAR_QUEUE_H */
