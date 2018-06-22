/*
 * File:   ibus.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#ifndef IBUS_H
#define	IBUS_H
#include <stdint.h>
#include "uart.h"

/**
 * IBus_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the I-Bus
 */
typedef struct IBus_t {
    struct UART_t uart;
} IBus_t;

struct IBus_t IBusInit();
void IBusProcess(struct IBus_t *ibus);

#endif	/* IBUS_H */
