/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#include "config.h"

/**
 * ConfigGetUIMode()
 *     Description:
 *         Get the UI mode configured
 *     Params:
 *         None
 *     Returns:
 *         void
 */
unsigned char ConfigGetUIMode()
{
    unsigned char value = EEPROMReadByte(CONFIG_UI_MODE_ADDRESS);
    if (value == IBus_UI_CD53) {
        return IBus_UI_CD53;
    } else if (value == IBus_UI_BMBT) {
        return IBus_UI_BMBT;
    }
    return 0;
}

/**
 * ConfigSetBootloaderMode()
 *     Description:
 *         Set the bootloader mode
 *     Params:
 *         unsigned char bootloaderMode - The Bootloader mode to set
 *     Returns:
 *         void
 */
void ConfigSetBootloaderMode(unsigned char bootloaderMode)
{
    EEPROMWriteByte(CONFIG_BOOTLOADER_MODE_ADDRESS, bootloaderMode);
}

/**
 * ConfigSetUIMode()
 *     Description:
 *         Set the UI mode
 *     Params:
 *         unsigned char uiMode - The UI mode to set
 *     Returns:
 *         void
 */
void ConfigSetUIMode(unsigned char uiMode)
{
    EEPROMWriteByte(CONFIG_UI_MODE_ADDRESS, uiMode);
}
