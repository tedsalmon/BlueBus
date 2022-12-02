/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#include "config.h"

unsigned char CONFIG_SETTING_CACHE[CONFIG_SETTING_CACHE_SIZE] = {0};
unsigned char CONFIG_VALUE_CACHE[CONFIG_VALUE_CACHE_SIZE] = {0};

/**
 * ConfigGetBC127BootFailures()
 *     Description:
 *         Get the count of BC127 boot failures
 *     Params:
 *         None
 *     Returns:
 *         uint16_t
 */
uint16_t ConfigGetBC127BootFailures()
{
    uint8_t lsb = ConfigGetValue(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_LSB);
    uint8_t msb = ConfigGetValue(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_MSB);
    // 0xFFFF is the max allowable value, so reset the counter
    if (lsb == 0xFF && msb == 0xFF) {
        return 0;
    }
    return (msb << 8) + lsb;
}

/**
 * ConfigGetByte()
 *     Description:
 *         Pull a byte from the EEPROM. If that byte is 0xFF, assume it's 0x00
 *     Params:
 *         unsigned char address - The address to read from
 *     Returns:
 *         unsigned char
 */
static inline unsigned char ConfigGetByte(unsigned char address)
{
    unsigned char value = 0;
    if (address < CONFIG_SETTING_CACHE_SIZE)
        value = CONFIG_SETTING_CACHE[address];
    if (value == 0x00) {
        value = EEPROMReadByte(address);
        if (value == 0xFF) {
            value = 0x00;
        }
        if (address < CONFIG_SETTING_CACHE_SIZE) {
            CONFIG_SETTING_CACHE[address] = value;
        }
    }
    return value;
}

/**
 * ConfigSetByte()
 *     Description:
 *         Set a byte into the EEPROM and update cache
 *     Params:
 *         unsigned char address - The address to read from
 *         unsigned char value - Value to set
 */
static inline void ConfigSetByte(unsigned char address, unsigned char value)
{
    if (address < CONFIG_SETTING_CACHE_SIZE)
        CONFIG_SETTING_CACHE[address] = value;
    EEPROMWriteByte(address, value);
}

/**
 * ConfigGetByteLowerNibble()
 *     Description:
 *         Get the lower nibble of a given byte from the EEPROM
 *     Params:
 *         unsigned char byte - The byte to get
 *     Returns:
 *         unsigned char - The value
 */
unsigned char ConfigGetByteLowerNibble(unsigned char byte)
{
    unsigned char value = ConfigGetByte(byte);
    return value & 0x0F;
}

/**
 * ConfigGetByteUpperNibble()
 *     Description:
 *         Get the upper nibble of a given byte from the EEPROM
 *     Params:
 *         unsigned char byte - The byte to get
 *     Returns:
 *         unsigned char - The value
 */
unsigned char ConfigGetByteUpperNibble(unsigned char byte)
{
    unsigned char value = ConfigGetByte(byte);
    return (value & 0xF0) >> 4;
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
    unsigned char value = ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_WEEK);
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
    unsigned char value = ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_YEAR);
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
    return ConfigGetByteUpperNibble(CONFIG_SETTING_COMFORT_LOCKS);
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
    return ConfigGetByteLowerNibble(CONFIG_SETTING_COMFORT_LOCKS);
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
    unsigned char value = ConfigGetByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS);
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
    unsigned char value = ConfigGetByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS);
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
    unsigned char value = ConfigGetByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS);
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
    return ConfigGetByteUpperNibble(CONFIG_VEHICLE_TYPE_ADDRESS);
}

/**
 * ConfigGetLightingFeaturesActive()
 *     Description:
 *         Check if any lighting features are active and return a boolean
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetLightingFeaturesActive()
{
    if (ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS) > 0x01 ||
        ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS) > 0x01
    ) {
        return CONFIG_SETTING_ON;
    }
    return CONFIG_SETTING_OFF;
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
    unsigned char value = ConfigGetByte(CONFIG_LM_VARIANT_ADDRESS);
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
    unsigned char currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
    if (currentSetting == 0x00) {
        currentSetting = 0x01;
        ConfigSetByte(CONFIG_SETTING_LOG_ADDRESS, currentSetting);
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
    unsigned char value = ConfigGetByte(CONFIG_NAV_TYPE_ADDRESS);
    return value;
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
    return (snMSB << 8) + snLSB;
}

/**
 * ConfigGetSetting()
 *     Description:
 *         Get a given setting from the EEPROM
 *     Params:
 *         unsigned char setting - The setting to get
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
        value = ConfigGetByte(setting);
    }
    return value;
}

/**
 * ConfigGetTelephonyFeaturesActive()
 *     Description:
 *         Check if any telephone features are active and return a boolean
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetTelephonyFeaturesActive()
{
    if (ConfigGetSetting(CONFIG_SETTING_HFP_ADDRESS) == CONFIG_SETTING_ON ||
        ConfigGetSetting(CONFIG_SETTING_SELF_PLAY_ADDRESS) == CONFIG_SETTING_ON
    ) {
        return CONFIG_SETTING_ON;
    }
    return CONFIG_SETTING_OFF;
}

/**
 * ConfigGetTempDisplay()
 *     Description:
 *         Return the temperature display configuration value
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetTempDisplay()
{
    return ConfigGetByteLowerNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY);
}

/**
 * ConfigGetTempUnit()
 *     Description:
 *         Return the temperature units that the vehicle is configured for
 *     Params:
 *         None
 *     Returns:
 *         unsigned char
 */
unsigned char ConfigGetTempUnit()
{
    return ConfigGetByteUpperNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY);
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
    unsigned char value = ConfigGetByte(CONFIG_UI_MODE_ADDRESS);
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
    return ConfigGetByteLowerNibble(CONFIG_VEHICLE_TYPE_ADDRESS);
}

/**
 * ConfigGetValue()
 *     Description:
 *         Get a given value from the EEPROM
 *     Params:
 *         unsigned char value - The value to get
 *     Returns:
 *         unsigned char - The value
 */
unsigned char ConfigGetValue(unsigned char value)
{
    unsigned char data = 0x00;
    // Catch invalid setting addresses
    if (value >= CONFIG_VALUE_START_ADDRESS &&
        value <= CONFIG_VALUE_END_ADDRESS
    ) {
        data = CONFIG_VALUE_CACHE[value - CONFIG_VALUE_START_ADDRESS];
        if (data == 0x00) {
            data = EEPROMReadByte(value);
            CONFIG_VALUE_CACHE[value - CONFIG_VALUE_START_ADDRESS] = data;
        }
    }
    return data;
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
 * ConfigSetBC127BootFailures()
 *     Description:
 *         Set count of BC127 boot failures
 *     Params:
 *         uint16_t failureCount - The number of failed boots
 *     Returns:
 *         void
 */
void ConfigSetBC127BootFailures(uint16_t failureCount)
{
    ConfigSetSetting(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_MSB, failureCount >> 8);
    ConfigSetSetting(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_LSB, failureCount & 0xFF);
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
    ConfigSetByte(CONFIG_BOOTLOADER_MODE_ADDRESS, bootloaderMode);
}

/**
 * ConfigSetByteLowerNibble()
 *     Description:
 *         Set a given setting into the lower nibble of a byte in the EEPROM
 *     Params:
 *         unsigned char setting - The setting to set
 *         unsigned char value - The value to set
 *     Returns:
 *         void
 */
void ConfigSetByteLowerNibble(unsigned char setting, unsigned char value)
{
    unsigned char currentValue = ConfigGetByte(setting);
    // Store the value in the lower nibble of the comfort locks setting
    currentValue &= 0xF0;
    currentValue |= value & 0x0F;
    ConfigSetByte(setting, currentValue);
}

/**
 * ConfigSetByteUpperNibble()
 *     Description:
 *         Set a given setting into the upper nibble of a byte in the EEPROM
 *     Params:
 *         unsigned char setting - The setting to set
 *         unsigned char value - The value to set
 *     Returns:
 *         void
 */
void ConfigSetByteUpperNibble(unsigned char setting, unsigned char value)
{
    unsigned char currentValue = ConfigGetByte(setting);
    // Store the value in the upper nibble of the vehicle type byte
    currentValue &= 0x0F;
    currentValue |= (value << 4) & 0xF0;
    ConfigSetByte(setting, currentValue);
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
    ConfigSetByteUpperNibble(CONFIG_SETTING_COMFORT_LOCKS, comfortLock);
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
    ConfigSetByteLowerNibble(CONFIG_SETTING_COMFORT_LOCKS, comfortUnlock);
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
    ConfigSetByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS, major);
    ConfigSetByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS, minor);
    ConfigSetByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS, patch);
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
    ConfigSetByteUpperNibble(CONFIG_VEHICLE_TYPE_ADDRESS, ikeType);
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
    ConfigSetByte(CONFIG_LM_VARIANT_ADDRESS, variant);
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
    ConfigSetByte(CONFIG_SETTING_LOG_ADDRESS, currentSetting);
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
    ConfigSetByte(CONFIG_NAV_TYPE_ADDRESS, type);
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
        ConfigSetByte(setting, value);
    }
}

/**
 * ConfigSetTempDisplay()
 *     Description:
 *         Set the temperature display setting
 *     Params:
 *         unsigned char tempDisplay - The temperature display setting
 *     Returns:
 *         void
 */
void ConfigSetTempDisplay(unsigned char tempDisplay)
{
    ConfigSetByteLowerNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY, tempDisplay);
}

/**
 * ConfigSetTempUnit()
 *     Description:
 *         Set the temperature unit setting
 *     Params:
 *         unsigned char tempUnit - The temperature unit
 *     Returns:
 *         void
 */
void ConfigSetTempUnit(unsigned char tempUnit)
{
    ConfigSetByteUpperNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY, tempUnit);
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
    ConfigSetByte(trap, count);
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
    ConfigSetByte(CONFIG_TRAP_LAST_ERR, trap);
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
    ConfigSetByte(CONFIG_UI_MODE_ADDRESS, uiMode);
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
    ConfigSetByteLowerNibble(CONFIG_VEHICLE_TYPE_ADDRESS, vehicleType);
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
        ConfigSetByte(vinAddress[i], vin[i]);
    }
}

void ConfigSetString(unsigned char address, char *str, uint8_t size) {
    while ((size>0)&&(str[0])!=0) {
        ConfigSetByte(address,str[0]);
        size--;
        str++;
        address++;
    };
    ConfigSetByte(address,0);
}

void ConfigGetString(unsigned char address, char *str, uint8_t size) {
    char b = ConfigGetByte(address);
    while ((size>0)&&(b!=0)) {
        str[0]=b;
        size--;
        str++;
        address++;
        b = ConfigGetByte(address);
    };
    str[0]=0;
}

void ConfigSetBytes(unsigned char address, const uint8_t *str, uint8_t size) {
    while (size>0) {
        ConfigSetByte(address,str[0]);
        size--;
        str++;
        address++;
    };
}

void ConfigGetBytes(unsigned char address, uint8_t *str, uint8_t size) {
    while (size>0) {
        uint8_t b = ConfigGetByte(address);
        str[0]=b;
        size--;
        str++;
        address++;
    };
}