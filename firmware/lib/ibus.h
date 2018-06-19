/* 
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * This implements the BMW IBus
 */

#ifndef IBUS_H
#define	IBUS_H
#include <stdint.h>
#include "uart.h"

typedef struct IBus_t {
    UART_t *uart;
    void (*destroy) (struct IBus_t *);
    void (*process) (struct IBus_t *);
} IBus_t;

IBus_t *IBusInit();
void IBusDestroy(struct IBus_t *ibus);
void IBusProcess(struct IBus_t *ibus);

#endif	/* IBUS_H */

