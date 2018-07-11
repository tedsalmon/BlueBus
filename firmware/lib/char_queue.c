/*
 * File: char_queue.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a FIFO queue to store bytes read from UART into
 */
#include "char_queue.h"

/**
 * CharQueueInit()
 *     Description:
 *         Returns a fresh CharQueue_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         CharQueue_t *
 */
CharQueue_t CharQueueInit()
{
    CharQueue_t queue;
    // Initialize size, capacity and cursors
    queue.size = 0;
    queue.readCursor = 0;
    queue.writeCursor = 0;
    queue.capacity = CHAR_QUEUE_SIZE;
    queue.maxCap = 0;
    return queue;
}

/**
 * CharQueueAdd()
 *     Description:
 *         Adds a byte to the queue. If the queue is full, the byte is discarded.
 *     Params:
 *         CharQueue_t *queue - The queue
 *         unsigned char value - The value to add
 *     Returns:
 *         None
 */
void CharQueueAdd(CharQueue_t *queue, unsigned char value)
{
    if (queue->size != queue->capacity) {
        // Reset the write cursor before it goes out of bounds
        if (queue->writeCursor == queue->capacity) {
            queue->writeCursor = 0;
        }
        queue->data[queue->writeCursor] = value;
        queue->writeCursor++;
        queue->size++;
        if (queue->size > queue->maxCap) {
            queue->maxCap = queue->size;
        }
    }
}

/**
 * CharQueueGet()
 *     Description:
 *         Returns the byte at location idx. Returning the byte does not remove
 *         it from the queue.
 *     Params:
 *         CharQueue_t queue - The queue
 *         uint16_t idx - The index to return data for
 *     Returns:
 *         unsigned char
 */
unsigned char CharQueueGet(CharQueue_t *queue, uint16_t idx)
{
    if (idx >= queue->capacity) {
        return 0x00;
    }
    return queue->data[idx];
}

/**
 * CharQueueNext()
 *     Description:
 *         Shifts the next byte in the queue out, as seen by the read cursor.
 *         Once the byte is returned, it should be considered destroyed from the
 *         queue.
 *     Params:
 *         CharQueue_t queue - The queue
 *     Returns:
 *         unsigned char
 */
unsigned char CharQueueNext(CharQueue_t *queue)
{
    unsigned char data = queue->data[queue->readCursor];
    // Remove the byte from memory
    queue->data[queue->readCursor] = 0x00;
    queue->readCursor++;
    if (queue->readCursor >= queue->capacity) {
        queue->readCursor = 0;
    }
    if (queue->size > 0) {
        queue->size--;
    }
    return data;
}

/**
 * CharQueueSeek()
 *     Description:
 *         Checks if a given byte is in the queue and return the length of
 *         characters prior to it.
 *     Params:
 *         CharQueue_t queue - The queue
 *     Returns:
 *         int16_t - The length of characters prior to the needle or zero if
 *                   the needle wasn't found
 */
int16_t CharQueueSeek(CharQueue_t *queue, unsigned char needle)
{
    uint16_t readCursor = (int16_t) queue->readCursor;
    uint16_t size = queue->size;
    uint16_t cnt = 0;
    while (size > 0) {
        cnt++;
        if (readCursor >= queue->capacity) {
            readCursor = 0;
        }
        if (queue->data[readCursor] == needle) {
            return cnt;
        }
        size--;
        readCursor++;
    }
    return 0;
}
