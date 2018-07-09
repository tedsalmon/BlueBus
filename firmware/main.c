/*
 * File: ibus.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     The main loop for our PIC24FJ
 */
#include <stdlib.h>
#include <stdio.h>
#include <xc.h>
#include "handler.h"
#include "io_mappings.h"
#include "lib/debug.h"
#include "lib/bc127.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/uart.h"

int main(void)
{
    // Initialize the system UART first, since we needed it for debug
    struct UART_t systemUart = UARTInit(
        SYSTEM_UART_MODULE,
        SYSTEM_UART_RX_PIN,
        SYSTEM_UART_TX_PIN,
        UART_BAUD_115200,
        UART_PARITY_NONE
    );
    // All UART handler registrations need to be done at
    // this level to maintain a global scope
    UARTAddModuleHandler(&systemUart);
    LogInfo("***** BlueBus Init *****");

    struct BC127_t bt = BC127Init();
    UARTAddModuleHandler(&bt.uart);

    struct IBus_t ibus = IBusInit();
    UARTAddModuleHandler(&ibus.uart);

    TRISAbits.TRISA7 = 0;
    LATAbits.LATA7 = 1;

    TimerInit();
    // Send the module structs to the event handlers
    HandlerRegister(&bt, &ibus);

    // Trigger the event callbacks for the module Start Up
    BC127Startup();
    IBusStartup();

    uint32_t lastDebugReport = TimerGetMillis();
    // Process Synchronous events
    while (1) {
        BC127Process(&bt);
        IBusProcess(&ibus);
        uint32_t now = TimerGetMillis();
        if ((now - lastDebugReport) >= 30000) {
            LogDebug("IBus Rx Queue Max: %d", ibus.uart.rxQueue.maxCap);
            LogDebug("IBus Tx Queue Max: %d", ibus.uart.txQueue.maxCap);
            LogDebug("BC127 Rx Queue Max: %d", bt.uart.rxQueue.maxCap);
            LogDebug("BC127 Tx Queue Max: %d", bt.uart.txQueue.maxCap);
            LogDebug("System Rx Queue Max: %d", systemUart.rxQueue.maxCap);
            LogDebug("System Tx Queue Max: %d", systemUart.txQueue.maxCap);
            lastDebugReport = now;
        }
    }

    return 0;
}
