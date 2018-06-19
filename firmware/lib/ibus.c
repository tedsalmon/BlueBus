/*
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * This implements the BMW IBus
 */

#include <stdlib.h>
#include "ibus.h"
#include "uart.h"

IBus_t *IBusInit ()
{
    IBus_t *ibus = malloc(sizeof(IBus_t));
    if (ibus != NULL) {
        ibus->uart = UARTInit(1, 2, UART_BAUD_9600);
        // Function pointers
        ibus->destroy = &IBusDestroy;
        ibus->process = &IBusProcess;
    } else {
        // Log("Failed to malloc() IBus struct");
    }
    return ibus;
}

void IBusDestroy(struct IBus_t *ibus)
{
    ibus->uart->destroy(ibus->uart);
    free(ibus);
}

void IBusProcess(struct IBus_t *ibus)
{

}
