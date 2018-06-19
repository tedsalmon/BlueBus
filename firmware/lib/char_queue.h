#include <stdint.h>
#include <string.h>

#ifndef CHAR_QUEUE_H
#define	CHAR_QUEUE_H
#define QUEUE_INIT_SIZE 256

typedef struct CharQueue_t{
    uint16_t size;
    uint16_t capacity;
    unsigned char *data;
    void (*append) (struct CharQueue_t *, unsigned char);
    void (*destroy) (struct CharQueue_t *);
    unsigned char (*get) (struct CharQueue_t *, uint16_t);
    unsigned char (*next) (struct CharQueue_t *);
} CharQueue_t;

CharQueue_t *CharQueueInit();

void CharQueueAppend(struct CharQueue_t *queue, unsigned char value);

void CharQueueDestroy(struct CharQueue_t *queue);

unsigned char CharQueueGet(struct CharQueue_t *queue, uint16_t idx);

unsigned char CharQueueNext(struct CharQueue_t *queue);

#endif	/* CHAR_QUEUE_H */