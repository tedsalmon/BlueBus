/*
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * This implements the Sierra Wireless BC127 Bluetooth API
 */
#include <stdlib.h>
#include "bc127.h"

BC127_t *BC127Init()
{
    BC127_t *bt = malloc(sizeof(BC127_t));
    if (bt != NULL) {
        bt->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
        bt->newAvrcpMeta = 0;
        bt->uart = UARTInit(1, 2, UART_BAUD_9600);

        // Function pointers
        bt->destroy = &BC127Destroy;
        bt->sendCommand = &BC127SendCommand;
    } else {
        // Log("Failed to malloc() BC127 Object");
    }
    return bt;
}

/**
 * BC127 Struct Destructor - Takes an object of BC127_t and frees the memory in use
 *
 */
void BC127Destroy(struct BC127_t *bt)
{
    bt->uart->destroy(bt->uart);
    free(bt);
}

void BC127SendCommand(struct BC127_t *bt, unsigned char *command)
{
    bt->uart->sendData(bt->uart, command);
}
