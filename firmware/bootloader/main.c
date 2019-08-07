/*
 * File: main.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     The main code for our PIC24FJ bootloader
 */
#include <xc.h>
#include "sysconfig.h"
#include "mappings.h"
#include "lib/eeprom.h"
#include "lib/protocol.h"
#include "lib/timer.h"
#include "lib/uart.h"

int main(void)
{
    // Set the IVT mode to regular
    IVT_MODE = IVT_MODE_BOOT;

    // Set all used ports to digital mode
    ANSB = 0;
    ANSD = 0;
    ANSE = 0;
    ANSF = 0;
    ANSG = 0;

    // Set the pin modes
    UART_SEL_MODE = 0;
    RECOVERY_MODE = 1;
    BT_DATA_SEL_MODE = 0;

    // Set the UART mode to MCU by default
    UART_SEL = UART_SEL_MCU;

    ON_LED_MODE = 0;
    struct UART_t systemUart = UARTInit(
        SYSTEM_UART_MODULE,
        SYSTEM_UART_RX_PIN,
        SYSTEM_UART_TX_PIN,
        UART_BAUD_115200,
        UART_PARITY_ODD
    );

    struct UART_t btUart = UARTInit(
        BC127_UART_MODULE,
        BC127_UART_RX_PIN,
        BC127_UART_TX_PIN,
        UART_BAUD_115200,
        UART_PARITY_NONE
    );

    // Register the module handlers at a global scope
    UARTAddModuleHandler(&systemUart);
    UARTAddModuleHandler(&btUart);

    TimerInit();
    EEPROMInit();

    uint8_t BOOT_MODE = BOOT_MODE_APPLICATION;
    unsigned char configuredBootmode = EEPROMReadByte(CONFIG_BOOTLOADER_MODE);
    // If the bootloader flag is set in the EEPROM or the recovery pin
    // has been pulled, then lock into bootloader mode
    if (configuredBootmode != 0x00 || RECOVERY_STATUS == 1) {
        BOOT_MODE = BOOT_MODE_BOOTLOADER;
        EEPROMWriteByte(CONFIG_BOOTLOADER_MODE, 0x00);
        ON_LED = 1;
        BT_DATA_SEL = 1;
    }

    while (BOOT_MODE == BOOT_MODE_BOOTLOADER && BOOT_MODE != BOOT_MODE_NOW) {
        UARTReadData(&systemUart);
        if (systemUart.rxQueueSize > 0) {
            ProtocolProcessMessage(&systemUart, &BOOT_MODE);
        }
        UARTReadData(&btUart);
        if (btUart.rxQueueSize > 0) {
            ProtocolProcessMessage(&systemUart, &BOOT_MODE);
        }
    }
    // Set the IVT mode to regular
    IVT_MODE = IVT_MODE_APP;

    // Close the UART modules so the application can utilize it
    UARTDestroy(SYSTEM_UART_MODULE);
    UARTDestroy(BC127_UART_MODULE);
    // Close the EEPROM (SPI module) so that the application can utilize it
    EEPROMDestroy();

    // Turn off the LED & Set the BC127 back to normal mode
    ON_LED = 0;
    BT_DATA_SEL = 0;

    // Wait until the specified timeout so that the rest of the board
    // has a chance to power up before we start configuring it in the application
    while (TimerGetMillis() <= BOOTLOADER_TIMEOUT);

    // Call the application code
    void (*appptr)(void);
    appptr = (void (*)(void))BOOTLOADER_APPLICATION_VECTOR;
    appptr();

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
