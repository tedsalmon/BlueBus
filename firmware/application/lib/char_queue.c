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
 *         volatile CharQueue_t *
 */
CharQueue_t CharQueueInit()
{
    volatile CharQueue_t queue;
    // Initialize size and cursors
    CharQueueReset(&queue);
    return queue;
}

/**
 * CharQueueAdd()
 *     Description:
 *         Adds a byte to the queue. If the queue is full, the byte is discarded.
 *     Params:
 *         volatile CharQueue_t *queue - The queue
 *         const uint8_t value - The value to add
 *     Returns:
 *         None
 */
void CharQueueAdd(volatile CharQueue_t *queue, const uint8_t value)
{
    uint16_t size = CharQueueGetSize(queue);
    if (size < CHAR_QUEUE_SIZE) {
        queue->data[queue->writeCursor] = value;
        queue->writeCursor++;
        if (queue->writeCursor >= CHAR_QUEUE_SIZE) {
            queue->writeCursor = 0;
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
 *         uint8_t
 */
uint8_t CharQueueGet(volatile CharQueue_t *queue, const uint16_t idx)
{
    if (idx >= CHAR_QUEUE_SIZE) {
        return 0x00;
    }
    return queue->data[idx];
}

/**
 * CharQueueGetOffset()
 *     Description:
 *         Returns the byte at the location n steps away from the current index
 *     Params:
 *         CharQueue_t queue - The queue
 *         uint16_t idx - The index to return data for
 *     Returns:
 *         uint8_t
 */
uint8_t CharQueueGetOffset(volatile CharQueue_t *queue, const uint16_t offset)
{
    uint16_t queueSize = CharQueueGetSize(queue);
    if (offset > queueSize) {
        return 0x00;
    }
    uint16_t offsetCursor = queue->readCursor + offset;
    if (offsetCursor >= CHAR_QUEUE_SIZE) {
        offsetCursor = offsetCursor - CHAR_QUEUE_SIZE;
    }
    return queue->data[offsetCursor];
}

/**
 * CharQueueGetSize()
 *     Description:
 *         Returns the size of the queue based on the difference between
 *         read and write cursors
 *     Params:
 *         CharQueue_t queue - The queue
 *     Returns:
 *         uint16_t - The size
 */
uint16_t CharQueueGetSize(volatile CharQueue_t *queue)
{
    uint16_t queueSize = 0;
    // Keep the cursor values in the registers to avoid transient values
    uint16_t rCursor = queue->readCursor;
    uint16_t wCursor = queue->writeCursor;
    if (wCursor >= rCursor) {
        queueSize = wCursor - rCursor;
    } else {
        queueSize = (CHAR_QUEUE_SIZE - rCursor) + wCursor;
    }
    return queueSize;
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
 *         uint8_t
 */
uint8_t CharQueueNext(volatile CharQueue_t *queue)
{
    uint16_t size = CharQueueGetSize(queue);
    if (size <= 0) {
        return 0x00;
    }
    uint8_t data = queue->data[queue->readCursor];
    // Remove the byte from memory
    queue->data[queue->readCursor] = 0x00;
    queue->readCursor++;
    if (queue->readCursor >= CHAR_QUEUE_SIZE) {
        queue->readCursor = 0;
    }
    return data;
}

/**
 * CharQueueRemoveLast()
 *     Description:
 *         Remove the last byte in the buffer
 *     Params:
 *         CharQueue_t queue - The queue
 *     Returns:
 *         void
 */
void CharQueueRemoveLast(volatile CharQueue_t *queue)
{
    if (CharQueueGetSize(queue) > 0) {
        queue->data[queue->writeCursor] = 0x00;
        if (queue->writeCursor == 0) {
            queue->writeCursor = CHAR_QUEUE_SIZE - 1;
        } else {
            queue->writeCursor--;
        }
    }
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
void CharQueueReset(volatile CharQueue_t *queue)
{
    queue->readCursor = 0;
    queue->writeCursor = 0;
    memset((void *) queue->data, 0, CHAR_QUEUE_SIZE);
}

/**
 * CharQueueSeek()
 *     Description:
 *         Checks if a given byte is in the queue and return the length of
 *         characters prior to it.
 *     Params:
 *         volatile CharQueue_t *queue - The queue
 *         const uint8_t needle - The character to look for
 *     Returns:
 *         uint16_t - The length of characters prior to the needle or zero if
 *                   the needle wasn't found
 */
uint16_t CharQueueSeek(volatile CharQueue_t *queue, const uint8_t needle)
{
    uint16_t readCursor = queue->readCursor;
    uint16_t size = CharQueueGetSize(queue);
    uint16_t cnt = 1;
    while (size > 0) {
        if (queue->data[readCursor] == needle) {
            return cnt;
        }
        readCursor++;
        if (readCursor >= CHAR_QUEUE_SIZE) {
            readCursor = 0;
        }
        cnt++;
        size--;
    }
    return 0;
}
