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

    // Set all ports to digital mode
    ANSB = 0;
    ANSC = 0;
    ANSD = 0;
    ANSE = 0;
    ANSF = 0;
    ANSG = 0;

    // Set all ports as outputs
    TRISB = 0;
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0;

    // Drive all ports to logic low
    LATB = 0;
    LATC = 0;
    LATD = 0;
    LATE = 0;
    LATF = 0;
    LATG = 0;

    // Set the I/O ports to outputs
    ON_LED_MODE = 0;
    BT_DATA_SEL_MODE = 0;
    BT_MFB_MODE = 0;
    BT_RST_MODE = 0;
    IBUS_EN_MODE = 0;
    PAM_SHDN_MODE = 0;
    SPDIF_RST_MODE = 0;
    TEL_ON_MODE_V1 = 0;
    TEL_ON_MODE_V2 = 0;
    TEL_MUTE_MODE_V1 = 0;
    TEL_MUTE_MODE_V2 = 0;
    UART_SEL_MODE = 0;

    // Set the RX / input pins to inputs
    BOARD_VERSION_MODE = 1;
    BT_UART_RX_PIN_MODE = 1;
    EEPROM_SDI_PIN_MODE = 1;
    IBUS_UART_RX_PIN_MODE = 1;
    // The I-Bus UART TX pin should be an input so we don't accidentally
    // keep the bus low and block frames from transmitting
    IBUS_UART_TX_PIN_MODE = 1;
    IBUS_UART_STATUS_MODE = 1;
    RECOVERY_MODE = 1;
    SYSTEM_UART_RX_PIN_MODE = 1;
    SYS_DTR_MODE = 1;

    // Enable the pull-down on the board version pin
    BOARD_VERSION_PD = 1;
    // Enable the IBus Regulator
    IBUS_EN = 1;
    // Turn the PAM8406 off until it's required
    PAM_SHDN = 0;
    // Set the I/O state to low
    TEL_ON_V1 = 0;
    TEL_ON_V2 = 0;
    TEL_MUTE_V1 = 0;
    TEL_MUTE_V2 = 0;
    if (UtilsGetBoardVersion() == BOARD_VERSION_ONE) {
        // Set the TMUX154E switch to MCU UART mode
        UART_SEL = UART_SEL_MCU;
    }

    // Init the timer interrupt and the SPI module for the EEPROM
    TimerInit();
    EEPROMInit();

    uint8_t STATUS_FLAG = 0;

    UART_t systemUart = UARTInit(
        SYSTEM_UART_MODULE,
        SYSTEM_UART_RX_RPIN,
        SYSTEM_UART_TX_RPIN,
        SYSTEM_UART_RX_PRIORITY,
        UART_BAUD_115200,
        UART_PARITY_ODD
    );
    // Register the module handlers at a global scope
    UARTAddModuleHandler(&systemUart);
    uint8_t BOOT_MODE = BOOT_MODE_APPLICATION;
    unsigned char bootMode = EEPROMReadByte(CONFIG_BOOTLOADER_MODE);
    // If the bootloader flag is set in the EEPROM or the recovery pin
    // is set, then lock into bootloader mode
    if (bootMode != 0x00 || RECOVERY_STATUS == 0) {
        BOOT_MODE = BOOT_MODE_BOOTLOADER;
        EEPROMWriteByte(CONFIG_BOOTLOADER_MODE, 0x00);
        TimerEnableLED();
        STATUS_FLAG = BOOTLOADER_STATUS_LED_ON;
    }

    while (
        (BOOT_MODE == BOOT_MODE_BOOTLOADER && BOOT_MODE != BOOT_MODE_NOW) ||
        TimerGetMillis() <= BOOTLOADER_TIMEOUT
    ) {
        uint16_t queueSize = CharQueueGetSize(&systemUart.rxQueue);
        if (queueSize >= PROTOCOL_PACKET_MIN_SIZE) {
            uint8_t pktStatus = ProtocolProcessMessage(&systemUart, &BOOT_MODE);
            if (pktStatus == PROTOCOL_PACKET_STATUS_OK && STATUS_FLAG == 0) {
                TimerEnableLED();
                STATUS_FLAG = BOOTLOADER_STATUS_LED_ON;
            }
        } else if (queueSize > 0) {
            ProtocolProcessQueue(&systemUart);
        }
    }

    // Set the bootloader mode to "Application" mode so if there is an issue
    // with the application we can lock the user in the bootloader to
    // accept a different firmware
    EEPROMWriteByte(CONFIG_BOOTLOADER_MODE, CONFIG_BOOTLODADER_MODE_APP);

    // Close the UART module so the application can utilize it
    UARTDestroy(SYSTEM_UART_MODULE);

    // Close the EEPROM (SPI module) so that the application can utilize it
    EEPROMDestroy();

    // Turn off the LED
    ON_LED = 0;

    // Disable the timers we set up
    TimerDestroy();

    // Set the IVT mode to regular
    IVT_MODE = IVT_MODE_APP;

    // Load the a pointer to the application initialization vector
    void (*appptr)(void);
    appptr = (void (*)(void))BOOTLOADER_APPLICATION_VECTOR;
    // Call the application code
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
