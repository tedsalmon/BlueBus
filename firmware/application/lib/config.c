/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#include "config.h"

unsigned char CONFIG_CACHE[CONFIG_CACHE_VALUES] = {};

/**
 * ConfigGetByte()
 *     Description:
 *         Pull a byte from the EEPROM. If that byte is 0xFF, assume it's 0x00
 *     Params:
 *         unsigned char address - The address to read from
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetByte(unsigned char address)
{
    unsigned char value = EEPROMReadByte(address);
    if (value == 0xFF) {
        value = 0x00;
    }
    return value;
}

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
    unsigned char currentSetting = CONFIG_CACHE[CONFIG_SETTING_LOG_ADDRESS];
    if (currentSetting == 0x00 && currentSetting != 0xFF) {
        unsigned char currentSetting = ConfigGetByte(
            CONFIG_SETTING_LOG_ADDRESS
        );
        if (currentSetting == 0x00) {
            // Prevent from re-reading the byte
            currentSetting = 0xFF;
        }
        CONFIG_CACHE[CONFIG_SETTING_LOG_ADDRESS] = currentSetting;
    }
    // The value is fully unset, so default to on
    if (currentSetting == 0xFF) {
        return 1;
    }
    return (currentSetting >> system) & 1;
}

/**
 * ConfigGetNavType()
 *     Description:
 *         Get the Nav Type discovered
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetNavType()
{
    unsigned char value = CONFIG_CACHE[CONFIG_NAV_TYPE_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_NAV_TYPE_ADDRESS);
        CONFIG_CACHE[CONFIG_NAV_TYPE_ADDRESS] = value;
    }
    return value;
}

/**
 * ConfigGetPoweroffTimeout()
 *     Description:
 *         Get the time in minutes that we should wait before powering off
 *     Params:
 *         void
 *     Returns:
 *         void
 */
unsigned char ConfigGetPoweroffTimeout()
{
    return ConfigGetByte(CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS);
}

/**
 * ConfigGetSetting()
 *     Description:
 *         Get a given setting from the EEPROM
 *     Params:
 *         unsigned char setting - The setting to set
 *     Returns:
 *         unsigned char - The value
 */
unsigned char ConfigGetSetting(unsigned char setting)
{
    unsigned char value = 0x00;
    // Catch invalid setting addresses
    if (setting >= 0x0F && setting <= 0x19) {
        value = CONFIG_CACHE[setting];
        if (value == 0x00) {
            value = ConfigGetByte(setting);
            CONFIG_CACHE[setting] = value;
        }
    }
    return value;
}

/**
 * ConfigGetTrapCount()
 *     Description:
 *         Get the number of times a trap has been triggered
 *     Params:
 *         unsigned char trap - The trap
 *     Returns:
 *         void
 */
unsigned char ConfigGetTrapCount(unsigned char trap)
{
    return ConfigGetByte(trap);
}

/**
 * ConfigGetTrapLast()
 *     Description:
 *         Get the last raised trap
 *     Params:
 *         void
 *     Returns:
 *         void
 */
unsigned char ConfigGetTrapLast()
{
    return ConfigGetByte(CONFIG_TRAP_LAST_ERR);
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
    if (value == 0x00 || 1) {
        value = ConfigGetByte(CONFIG_UI_MODE_ADDRESS);
        CONFIG_CACHE[CONFIG_UI_MODE_ADDRESS] = value;
    }
    return value;
}

/**
 * ConfigGetVehicleType()
 *     Description:
 *         Get the vehicle type
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetVehicleType()
{
    unsigned char value = CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_VEHICLE_TYPE_ADDRESS);
        CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS] = value;
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
    unsigned char currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
    unsigned char currentVal = (currentSetting >> system) & 1;
    if (mode != currentVal) {
        currentSetting ^= 1 << system;
    }
    EEPROMWriteByte(CONFIG_SETTING_LOG_ADDRESS, currentSetting);
    CONFIG_CACHE[CONFIG_SETTING_LOG_ADDRESS] = currentSetting;
}


/**
 * ConfigSetNavType()
 *     Description:
 *         Set the Nav Type discovered
 *     Params:
 *         unsigned char version
 *     Returns:
 *         void
 */
void ConfigSetNavType(unsigned char type)
{
    CONFIG_CACHE[CONFIG_NAV_TYPE_ADDRESS] = type;
    EEPROMWriteByte(CONFIG_NAV_TYPE_ADDRESS, type);
}

/**
 * ConfigSetPoweroffTimeout()
 *     Description:
 *         Set the time in minutes that we should wait before powering off
 *     Params:
 *         unsigned char timeout - The timeout
 *     Returns:
 *         void
 */
void ConfigSetPoweroffTimeout(unsigned char timeout)
{
    EEPROMWriteByte(CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS, timeout);
}

/**
 * ConfigSetSetting()
 *     Description:
 *         Set a given setting into the EEPROM
 *     Params:
 *         unsigned char setting - The setting to set
 *         unsigned char value - The value to set
 *     Returns:
 *         void
 */
void ConfigSetSetting(unsigned char setting, unsigned char value)
{
    // Catch invalid setting addresses
    if (setting >= 0x0A && setting <= 0x14) {
        CONFIG_CACHE[setting] = value;
        EEPROMWriteByte(setting, value);
    }
}

/**
 * ConfigSetTrapCount()
 *     Description:
 *         Set the trap count for the given trap
 *     Params:
 *         unsigned char trap - The trap triggered
 *         uint8_t count - The number 
 *     Returns:
 *         void
 */
void ConfigSetTrapCount(unsigned char trap, unsigned char count)
{
    if (count >= 0xFE) {
        // Reset the count so we don't overflow
        count = 1;
    }
    EEPROMWriteByte(trap, count);
}

/**
 * ConfigSetTrapIncrement()
 *     Description:
 *         Increment the trap count for the given trap
 *     Params:
 *         unsigned char trap - The trap triggered
 *     Returns:
 *         void
 */
void ConfigSetTrapIncrement(unsigned char trap)
{
    unsigned char count = ConfigGetTrapCount(trap);
    ConfigSetTrapCount(trap, count + 1);
    ConfigSetTrapLast(trap);
}

/**
 * ConfigSetTrapLast()
 *     Description:
 *         Set the last trap that was raised
 *     Params:
 *         unsigned char trap - The trap triggered
 *     Returns:
 *         void
 */
void ConfigSetTrapLast(unsigned char trap)
{
    EEPROMWriteByte(CONFIG_TRAP_LAST_ERR, trap);
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

/**
 * ConfigSetUIMode()
 *     Description:
 *         Set the vehicle type
 *     Params:
 *         unsigned char vehicleType - The vehicle type
 *     Returns:
 *         void
 */
void ConfigSetVehicleType(unsigned char vehicleType)
{
    CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS] = vehicleType;
    EEPROMWriteByte(CONFIG_VEHICLE_TYPE_ADDRESS, vehicleType);
}
