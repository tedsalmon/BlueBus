/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#include <stdlib.h>
#include "../io_mappings.h"
#include "ibus.h"
#include "debug.h"
#include "uart.h"

/**
 * IBusInit()
 *     Description:
 *         Returns a fresh IBus_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         struct IBus_t *
 */
struct IBus_t IBusInit ()
{
    struct IBus_t ibus;
    ibus.uart = UARTInit(
        IBUS_UART_MODULE,
        IBUS_UART_RX_PIN,
        IBUS_UART_TX_PIN,
        UART_BAUD_9600,
        UART_PARITY_EVEN
    );
    return ibus;
}

/**
 * IBusProcess()
 *     Description:
 *         Process messages in the I-Bus RX queue
 *     Params:
 *         struct IBus_t *ibus
 *     Returns:
 *         void
 */
void IBusProcess(struct IBus_t *ibus)
{
    uint8_t size = ibus->uart.rxQueue.size;
    if (ibus->uart.rxQueue.size > 0) {
        while (size > 0) {
            unsigned char c = CharQueueNext(&ibus->uart.rxQueue);
            LogDebug("Got Char 0x%x", c);
            size--;
        }
    }
}
