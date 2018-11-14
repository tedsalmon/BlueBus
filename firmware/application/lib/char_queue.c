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
    return queue;
}

/**
 * CharQueueAdd()
 *     Description:
 *         Adds a byte to the queue. If the queue is full, the byte is discarded.
 *     Params:
 *         CharQueue_t *queue - The queue
 *         const unsigned char value - The value to add
 *     Returns:
 *         None
 */
void CharQueueAdd(CharQueue_t *queue, const unsigned char value)
{
    if (queue->size != CHAR_QUEUE_SIZE) {
        if (queue->writeCursor == CHAR_QUEUE_SIZE) {
            queue->writeCursor = 0;
        }
        queue->data[queue->writeCursor] = value;
        queue->writeCursor++;
        queue->size++;
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
unsigned char CharQueueGet(CharQueue_t *queue, uint8_t idx)
{
    if (idx >= CHAR_QUEUE_SIZE) {
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
    if (queue->readCursor >= CHAR_QUEUE_SIZE) {
        queue->readCursor = 0;
    }
    if (queue->size > 0) {
        queue->size--;
    }
    return data;
}

/**
 * CharQueueReset()
 *     Description:
 *         Empty a char queue
 *     Params:
 *         CharQueue_t queue - The queue
 *     Returns:
 *         void
 */

void CharQueueReset(CharQueue_t *queue)
{
    memset(queue->data, 0, CHAR_QUEUE_SIZE);
    queue->readCursor = 0;
    queue->size = 0;
    queue->writeCursor = 0;
}

/**
 * CharQueueSeek()
 *     Description:
 *         Checks if a given byte is in the queue and return the length of
 *         characters prior to it.
 *     Params:
 *         CharQueue_t *queue - The queue
 *         const unsigned char needle - The character to look for
 *     Returns:
 *         uint8_t - The length of characters prior to the needle or zero if
 *                   the needle wasn't found
 */
uint8_t CharQueueSeek(CharQueue_t *queue, const unsigned char needle)
{
    uint8_t readCursor = queue->readCursor;
    uint8_t size = queue->size;
    uint8_t cnt = 1;
    while (size > 0) {
        if (queue->data[readCursor] == needle) {
            return cnt;
        }
        if (readCursor >= CHAR_QUEUE_SIZE) {
            readCursor = 0;
        }
        cnt++;
        size--;
        readCursor++;
    }
    return 0;
}
