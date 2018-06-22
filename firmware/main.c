/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     The main loop for our PIC24FJ
 */
#include <stdlib.h>
#include "io_mappings.h"
#include "lib/debug.h"
#include "lib/bc127.h"
#include "lib/uart.h"

int main(void)
{
    // Initialize the system UART first, since we needed it for debug
    struct UART_t systemUart = UARTInit(
        SYSTEM_UART_MODULE,
        SYSTEM_UART_RX_PIN,
        SYSTEM_UART_TX_PIN,
        UART_BAUD_115200
    );
    // All UART handler registrations need to be done at
    // this level to maintain a global scope
    UARTAddModuleHandler(&systemUart);
    LogInfo("***** BlueBus Init *****\r\n");

    struct BC127_t bt = BC127Init();
    UARTAddModuleHandler(&bt.uart);

    //struct IBus_t ibus = IBusInit();

    // Turn on LED D10 on our Dev Board
    LogInfo("***** LED ENABLED *****\r\n");
    TRISAbits.TRISA7 = 0;
    LATAbits.LATA7 = 1;

    // Process Synchronous events
    while (1) {
        BC127Process(&bt);
    }

    return 0;
}
