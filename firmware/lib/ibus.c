/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#include <stdlib.h>
#include "../io_mappings.h"
#include "ibus.h"
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
    //ibus.uart = UARTInit(1, 1, 2, UART_BAUD_9600);
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

}
