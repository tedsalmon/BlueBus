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
#define CHAR_QUEUE_SIZE 768

/**
 * CharQueue_t
 *     Description:
 *         This object holds CHAR_QUEUE_SIZE amounts of uint8_ts. It operates
 *         with a read and write cursor to keep track of where the next byte
 *         needs to be read from and where the next byte should be added.
 *         Once those cursors are exhausted, meaning they've hit capacity, they
 *         are reset. If data is not removed from the buffer before it hits
 *         capacity, the data will be lost.
 */
typedef struct CharQueue_t {
    volatile uint16_t readCursor;
    volatile uint16_t writeCursor;
    volatile uint8_t data[CHAR_QUEUE_SIZE];
} CharQueue_t;

CharQueue_t CharQueueInit();
void CharQueueAdd(volatile CharQueue_t *, const uint8_t);
uint8_t CharQueueGet(volatile CharQueue_t *, uint16_t);
uint16_t CharQueueGetSize(volatile CharQueue_t *);
uint8_t CharQueueGetOffset(volatile CharQueue_t *, uint16_t);
uint8_t CharQueueNext(volatile CharQueue_t *);
void CharQueueRemoveLast(volatile CharQueue_t *);
void CharQueueReset(volatile CharQueue_t *);
uint16_t CharQueueSeek(volatile CharQueue_t *, const uint8_t);
#endif /* CHAR_QUEUE_H */
