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
    unsigned char currentSetting = CONFIG_CACHE[CONFIG_SETTING_LOG_ADDRESS];
    if (currentSetting == 0x00 && currentSetting != 0xFF) {
        unsigned char currentSetting = EEPROMReadByte(
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
        value = EEPROMReadByte(CONFIG_NAV_TYPE_ADDRESS);
        CONFIG_CACHE[CONFIG_NAV_TYPE_ADDRESS] = value;
    }
    return value;
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
    if (setting >= 0x0A && setting <= 0x14) {
        value = CONFIG_CACHE[setting];
        if (value == 0x00) {
            value = EEPROMReadByte(setting);
            CONFIG_CACHE[setting] = value;
        }
    }
    return value;
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
        value = EEPROMReadByte(CONFIG_VEHICLE_TYPE_ADDRESS);
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
    unsigned char currentSetting = EEPROMReadByte(CONFIG_SETTING_LOG_ADDRESS);
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
