/*
 * File: char_queue.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a FIFO queue to store bytes read from UART into
 */
#include <stdint.h>
#include <string.h>

#ifndef CHAR_QUEUE_H
#define	CHAR_QUEUE_H
// The maximum amount of elements that the queue can hold
#define CHAR_QUEUE_SIZE 384

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
    uint16_t capacity;
    uint16_t size;
    uint16_t readCursor;
    uint16_t writeCursor;
    unsigned char data[CHAR_QUEUE_SIZE];

} CharQueue_t;

struct CharQueue_t CharQueueInit();

void CharQueueAdd(struct CharQueue_t *queue, unsigned char value);
unsigned char CharQueueGet(struct CharQueue_t *queue, uint16_t idx);
unsigned char CharQueueNext(struct CharQueue_t *queue);

#endif	/* CHAR_QUEUE_H */
