/*
 * File: char_queue.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a FIFO queue to store bytes read from UART into
 */
#ifndef CHAR_QUEUE_H
#define CHAR_QUEUE_H
#include <stdint.h>
#include <string.h>
/* The maximum amount of elements that the queue can hold */
#define CHAR_QUEUE_SIZE 640

/**
 * CharQueue_t
 *     Description:
 *         This object holds CHAR_QUEUE_SIZE amounts of unsigned chars. It operates
 *         with a read and write cursor to keep track of where the next byte
 *         needs to be read from and where the next byte should be added.
 *         Once those cursors are exhausted, meaning they've hit capacity, they
 *         are reset. If data is not removed from the buffer before it hits
 *         capacity, the data will be lost.
 */
typedef struct CharQueue_t {
    uint16_t size;
    uint16_t readCursor;
    uint16_t writeCursor;
    unsigned char data[CHAR_QUEUE_SIZE];
} CharQueue_t;

struct CharQueue_t CharQueueInit();
void CharQueueAdd(CharQueue_t *, const unsigned char);
unsigned char CharQueueGet(CharQueue_t *, uint16_t);
unsigned char CharQueueNext(CharQueue_t *);
void CharQueueRemoveLast(CharQueue_t *);
void CharQueueReset(CharQueue_t *);
uint16_t CharQueueSeek(CharQueue_t *, const unsigned char);
#endif /* CHAR_QUEUE_H */
