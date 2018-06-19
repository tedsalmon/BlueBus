/* 
 * File:   char_queue.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "char_queue.h"


CharQueue_t *CharQueueInit()
{
    CharQueue_t *queue = malloc(sizeof(CharQueue_t));
    if (queue != NULL) {
        // Initialize size and capacity
        queue->size = 0;
        queue->capacity = QUEUE_INIT_SIZE;
        queue->data = malloc(queue->capacity);
        if (queue->data == NULL) {
            // LogError("Failed to malloc() the char queue");
        }
        queue->append = &CharQueueAppend;
        queue->destroy = &CharQueueDestroy;
        queue->get = &CharQueueGet;
        queue->next = &CharQueueNext;
    } else {
        // LogError("Failed to malloc() CharQueue_t");
    }
    return queue;
}

void CharQueueAppend(struct CharQueue_t *queue, unsigned char value)
{
    // Resize the queue if it's at capacity
    if (queue->size >= queue->capacity) {
        queue->capacity *= 2;
        queue->data = realloc(queue->data, queue->capacity);
    }
    queue->data[queue->size] = value;
    queue->size++;
}

void CharQueueDestroy(struct CharQueue_t *queue)
{
    free(queue->data);
    free(queue);
}

unsigned char CharQueueGet(struct CharQueue_t *queue, uint16_t idx)
{
    if (idx >= queue->size) {
        return 0;
    }
    return queue->data[idx];
}

unsigned char CharQueueNext(struct CharQueue_t *queue)
{
    unsigned char data = queue->data[0];
    // Shift all elements one index to the left. 
    // If that index is empty, fill it with 0x00
    uint16_t curIdx = 1;
    while (curIdx <= queue->capacity) {
        unsigned char curByte = 0x00;
        if (curIdx < queue->size) {
            curByte = queue->data[curIdx];
        }
        queue->data[curIdx - 1] = curByte;
        curIdx++;
    }
    queue->size--;
    return data;
}
