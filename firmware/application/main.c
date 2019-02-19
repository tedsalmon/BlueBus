/*
 * File: main.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     The main loop for our PIC24FJ
 */
#include <stdlib.h>
#include <stdio.h>
#include <xc.h>
#include "sysconfig.h"
#include "handler.h"
#include "mappings.h"
#include "lib/bc127.h"
#include "lib/cli.h"
#include "lib/config.h"
#include "lib/debug.h"
#include "lib/eeprom.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/uart.h"

int main(void)
{
    // Set all used ports to digital mode
    ANSB = 0;
    ANSD = 0;
    ANSE = 0;
    ANSG = 0;
    // Initialize the system UART first, since we needed it for debug
    struct UART_t systemUart = UARTInit(
        SYSTEM_UART_MODULE,
        SYSTEM_UART_RX_PIN,
        SYSTEM_UART_TX_PIN,
        SYSTEM_UART_RX_PRIORITY,
        SYSTEM_UART_TX_PRIORITY,
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

    struct CLI_t cli = CLIInit(&systemUart, &bt, &ibus);

    ON_LED_MODE = 0;
    ON_LED = 1;

    EEPROMInit();
    TimerInit();

    unsigned char UI_MODE = ConfigGetUIMode();
    // Send the module objects to the application implementation handler
    HandlerInit(&bt, &ibus, UI_MODE);

    // Process events
    while (1) {
        BC127Process(&bt);
        IBusProcess(&ibus);
        TimerProcessScheduledTasks();
        CLIProcess(&cli);
    }

    return 0;
}

// Trap Catches
void __attribute__ ((__interrupt__, auto_psv)) _OscillatorFail(void)
{
    // Clear the trap flag
    INTCON1bits.OSCFAIL = 0;
    ON_LED = 0;
    while (1);
}

void __attribute__ ((__interrupt__, auto_psv)) _AddressError(void)
{
    // Clear the trap flag
    INTCON1bits.ADDRERR = 0;
    ON_LED = 0;
    while (1);
}


void __attribute__ ((__interrupt__, auto_psv)) _StackError(void)
{
    // Clear the trap flag
    INTCON1bits.STKERR = 0;
    ON_LED = 0;
    while (1);
}

void __attribute__ ((__interrupt__, auto_psv)) _MathError(void)
{
    // Clear the trap flag
    INTCON1bits.MATHERR = 0;
    ON_LED = 0;
    while (1);
}

void __attribute__ ((__interrupt__, auto_psv)) _NVMError(void)
{
    ON_LED = 0;
    while (1);
}

void __attribute__ ((__interrupt__, auto_psv)) _GeneralError(void)
{
    ON_LED = 0;
    while (1);
}
