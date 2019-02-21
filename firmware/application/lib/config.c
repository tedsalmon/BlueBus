/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#include "config.h"

unsigned char CONFIG_CACHE[CONFIG_CACHE_VALUES] = {};

/**
 * ConfigGetLog()
 *     Description:
 *         Get the log level for different systems
 *     Params:
 *         unsigned char system - The system to get the log mode for
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetLog(unsigned char system)
{
    unsigned char currentSetting = CONFIG_CACHE[
        CONFIG_DEVICE_LOG_SETTINGS_ADDRESS
    ];
    if (currentSetting == 0x00 && currentSetting != 0xFF) {
        unsigned char currentSetting = EEPROMReadByte(
            CONFIG_DEVICE_LOG_SETTINGS_ADDRESS
        );
        if (currentSetting == 0x00) {
            // Prevent from re-reading the byte
            currentSetting = 0xFF;
        }
        CONFIG_CACHE[CONFIG_DEVICE_LOG_SETTINGS_ADDRESS] = currentSetting;
    }
    // The value is fully unset, so default to on
    if (currentSetting == 0xFF) {
        return 1;
    }
    return (currentSetting >> system) & 1;
}

/**
 * ConfigGetUIMode()
 *     Description:
 *         Get the UI mode configured
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetUIMode()
{
    unsigned char value = CONFIG_CACHE[CONFIG_UI_MODE_ADDRESS];
    if (value == 0x00) {
        value = EEPROMReadByte(CONFIG_UI_MODE_ADDRESS);
    }
    return value;
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
 * ConfigSetLog()
 *     Description:
 *         Set the log level for different systems
 *     Params:
 *         unsigned char system - The system to set the log mode for
 *         unsigned char mode - The mode
 *     Returns:
 *         void
 */
void ConfigSetLog(unsigned char system, unsigned char mode)
{
    unsigned char currentSetting = EEPROMReadByte(
        CONFIG_DEVICE_LOG_SETTINGS_ADDRESS
    );
    unsigned char currentVal = (currentSetting >> system) & 1;
    if (mode != currentVal) {
        currentSetting ^= 1 << system;
    }
    EEPROMWriteByte(CONFIG_DEVICE_LOG_SETTINGS_ADDRESS, currentSetting);
    CONFIG_CACHE[CONFIG_DEVICE_LOG_SETTINGS_ADDRESS] = currentSetting;
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
    CONFIG_CACHE[CONFIG_UI_MODE_ADDRESS] = uiMode;
    EEPROMWriteByte(CONFIG_UI_MODE_ADDRESS, uiMode);
}
