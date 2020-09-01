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
 * ConfigGetBuildWeek()
 *     Description:
 *         Get the build week
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetBuildWeek()
{
    unsigned char value = CONFIG_CACHE[CONFIG_BUILD_DATE_ADDRESS_WEEK];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_WEEK);        
        CONFIG_CACHE[CONFIG_BUILD_DATE_ADDRESS_WEEK] = value;
    }
    return value;
}

/**
 * ConfigGetBuildYear()
 *     Description:
 *         Get the build year
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetBuildYear()
{
    unsigned char value = CONFIG_CACHE[CONFIG_BUILD_DATE_ADDRESS_YEAR];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_YEAR);        
        CONFIG_CACHE[CONFIG_BUILD_DATE_ADDRESS_YEAR] = value;
    }
    return value;
}

/**
 * ConfigGetComfortLock()
 *     Description:
 *         Get the Comfort Lock Setting
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetComfortLock()
{
    unsigned char value = CONFIG_CACHE[CONFIG_SETTING_COMFORT_LOCKS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_SETTING_COMFORT_LOCKS);        
        CONFIG_CACHE[CONFIG_SETTING_COMFORT_LOCKS] = value;
    }
    return (value & 0xF0) >> 4;
}

/**
 * ConfigGetComfortUnlock()
 *     Description:
 *         Get the Comfort Unlock Setting
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetComfortUnlock()
{
    unsigned char value = CONFIG_CACHE[CONFIG_SETTING_COMFORT_LOCKS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_SETTING_COMFORT_LOCKS);        
        CONFIG_CACHE[CONFIG_SETTING_COMFORT_LOCKS] = value;
    }
    return value & 0x0F;
}

/**
 * ConfigGetFirmwareVersionMajor()
 *     Description:
 *         Get the major firmware version
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetFirmwareVersionMajor()
{
    unsigned char value = CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS);        
        CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS] = value;
    }
    return value;
}

/**
 * ConfigGetFirmwareVersionMinor()
 *     Description:
 *         Get the minor firmware version
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetFirmwareVersionMinor()
{
    unsigned char value = CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS);        
        CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS] = value;
    }
    return value;
}

/**
 * ConfigGetFirmwareVersionPatch()
 *     Description:
 *         Get the patch firmware version
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetFirmwareVersionPatch()
{
    unsigned char value = CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS);        
        CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS] = value;
    }
    return value;
}

/**
 * ConfigGetFirmwareVersionString()
 *     Description:
 *         Get the firmware version as a string
 *     Params:
 *         char *version - The string to copy the version into
 *     Returns:
 *         void
 */
void ConfigGetFirmwareVersionString(char *version)
{
    snprintf(
        version,
        9,
        "%d.%d.%d",
        ConfigGetFirmwareVersionMajor(),
        ConfigGetFirmwareVersionMinor(),
        ConfigGetFirmwareVersionPatch()
    );
}

/**
 * ConfigGetIKEType()
 *     Description:
 *         Get the IKE type
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetIKEType()
{
    unsigned char value = CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_VEHICLE_TYPE_ADDRESS);        
        CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS] = value;
    }
    return (value & 0xF0) >> 4;
}

/**
 * ConfigGetLMVariant()
 *     Description:
 *         Get the Light Module variant
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetLMVariant()
{
    unsigned char value = CONFIG_CACHE[CONFIG_LM_VARIANT_ADDRESS];
    if (value == 0x00) {
        value = ConfigGetByte(CONFIG_LM_VARIANT_ADDRESS);
        CONFIG_CACHE[CONFIG_LM_VARIANT_ADDRESS] = value;
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
    if (currentSetting == 0x00) {
        currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
        if (currentSetting == 0x00) {
            // Prevent from re-reading the byte
            currentSetting = 0x01;
            EEPROMWriteByte(CONFIG_SETTING_LOG_ADDRESS, currentSetting);
        }
        CONFIG_CACHE[CONFIG_SETTING_LOG_ADDRESS] = currentSetting;
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
 *         Check if Auto-Power Off is disabled
 *     Params:
 *         void
 *     Returns:
 *         void
 */
unsigned char ConfigGetPoweroffTimeout()
{
    unsigned char poweroffValue = EEPROMReadByte(
        CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS
    );
    if (poweroffValue == CONFIG_SETTING_DISABLED) {
        return CONFIG_SETTING_DISABLED;
    } else {
        return CONFIG_SETTING_ENABLED;
    }
}

/**
 * ConfigGetSerialNumber()
 *     Description:
 *         Get a given setting from the EEPROM
 *     Params:
 *         None
 *     Returns:
 *         uint16_t - The serial number
 */
uint16_t ConfigGetSerialNumber()
{
    // Do not use ConfigGetByte() because our LSB could very well be 0xFF
    unsigned char snMSB = EEPROMReadByte(CONFIG_SN_ADDRESS_MSB);
    unsigned char snLSB = EEPROMReadByte(CONFIG_SN_ADDRESS_LSB);
    uint16_t serialNumber = (snMSB << 8) + snLSB;
    return serialNumber;
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
    if (setting >= CONFIG_SETTING_START_ADDRESS &&
        setting <= CONFIG_SETTING_END_ADDRESS
    ) {
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
    if (value == 0x00) {
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
    return value & 0x0F;
}

/**
 * ConfigGetVehicleIdentity()
 *     Description:
 *         Get the vehicle VIN from the EEPROM
 *     Params:
 *         unsigned char *
 *     Returns:
 *         void
 */
void ConfigGetVehicleIdentity(unsigned char *vin)
{
    unsigned char vinAddress[] = CONFIG_VEHICLE_VIN_ADDRESS;
    uint8_t i;
    for (i = 0; i < 5; i++) {
        vin[i] = ConfigGetByte(vinAddress[i]);
    }
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
 * ConfigSetComfortLock()
 *     Description:
 *         Set the comfort lock setting
 *     Params:
 *         unsigned char comfortLock - The comfort lock setting
 *     Returns:
 *         void
 */
void ConfigSetComfortLock(unsigned char comfortLock)
{
    unsigned char currentValue = ConfigGetByte(CONFIG_SETTING_COMFORT_LOCKS);
    // Store the value in the upper nibble of the comfort locks setting
    currentValue &= 0x0F;
    currentValue |= (comfortLock << 4) & 0xF0;
    CONFIG_CACHE[CONFIG_SETTING_COMFORT_LOCKS] = currentValue;
    EEPROMWriteByte(CONFIG_SETTING_COMFORT_LOCKS, currentValue);
}

/**
 * ConfigSetComfortUnlock()
 *     Description:
 *         Set the comfort unlock setting
 *     Params:
 *         unsigned char comfortUnlock - The comfort unlock setting
 *     Returns:
 *         void
 */
void ConfigSetComfortUnlock(unsigned char comfortUnlock)
{
    unsigned char currentValue = ConfigGetByte(CONFIG_SETTING_COMFORT_LOCKS);
    // Store the value in the lower nibble of the comfort locks setting
    currentValue &= 0xF0;
    currentValue |= comfortUnlock & 0x0F;
    CONFIG_CACHE[CONFIG_SETTING_COMFORT_LOCKS] = currentValue;
    EEPROMWriteByte(CONFIG_SETTING_COMFORT_LOCKS, currentValue);
}

/**
 * ConfigSetFirmwareVersion()
 *     Description:
 *         Set the firmware version
 *     Params:
 *         unsigned char major - The major version
 *         unsigned char minor - The minor version
 *         unsigned char patch - The patch version
 *     Returns:
 *         void
 */
void ConfigSetFirmwareVersion(
    unsigned char major,
    unsigned char minor,
    unsigned char patch
) {
    CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS] = major;
    EEPROMWriteByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS, major);
    CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS] = minor;
    EEPROMWriteByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS, minor);
    CONFIG_CACHE[CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS] = patch;
    EEPROMWriteByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS, patch);
}

/**
 * ConfigSetIKEType()
 *     Description:
 *         Set the IKE type
 *     Params:
 *         unsigned char ikeType - The IKE type
 *     Returns:
 *         void
 */
void ConfigSetIKEType(unsigned char ikeType)
{
    unsigned char currentValue = ConfigGetByte(CONFIG_VEHICLE_TYPE_ADDRESS);
    // Store the value in the upper nibble of the vehicle type byte
    currentValue &= 0x0F;
    currentValue |= (ikeType << 4) & 0xF0;
    CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS] = currentValue;
    EEPROMWriteByte(CONFIG_VEHICLE_TYPE_ADDRESS, currentValue);
}

/***
 * ConfigSetLMVariant()
 *     Description:
 *         Set the Light Module variant
 *     Params:
 *         unsigned char version
 *     Returns:
 *         void
 */
void ConfigSetLMVariant(unsigned char variant)
{
    CONFIG_CACHE[CONFIG_LM_VARIANT_ADDRESS] = variant;
    EEPROMWriteByte(CONFIG_LM_VARIANT_ADDRESS, variant);
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
 *         Disable Auto-Power Off
 *     Params:
 *         unsigned char status - Enable / Disable Auto Power off
 *     Returns:
 *         void
 */
void ConfigSetPoweroffTimeout(unsigned char status)
{
    EEPROMWriteByte(
        CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS,
        status
    );
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
    if (setting >= CONFIG_SETTING_START_ADDRESS &&
        setting <= CONFIG_SETTING_END_ADDRESS
    ) {
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
 * ConfigSetVehicleType()
 *     Description:
 *         Set the vehicle type
 *     Params:
 *         unsigned char vehicleType - The vehicle type
 *     Returns:
 *         void
 */
void ConfigSetVehicleType(unsigned char vehicleType)
{
    unsigned char currentValue = ConfigGetByte(CONFIG_VEHICLE_TYPE_ADDRESS);
    currentValue &= 0xF0;
    currentValue |= vehicleType & 0x0F;
    CONFIG_CACHE[CONFIG_VEHICLE_TYPE_ADDRESS] = currentValue;
    EEPROMWriteByte(CONFIG_VEHICLE_TYPE_ADDRESS, currentValue);
}

/**
 * ConfigSetVehicleIdentity()
 *     Description:
 *         Set the vehicle VIN
 *     Params:
 *         unsigned char *vin - The array to populate
 *     Returns:
 *         void
 */
void ConfigSetVehicleIdentity(unsigned char *vin)
{
    unsigned char vinAddress[] = CONFIG_VEHICLE_VIN_ADDRESS;
    uint8_t i;
    for (i = 0; i < 5; i++) {
        EEPROMWriteByte(vinAddress[i], vin[i]);
    }
}
