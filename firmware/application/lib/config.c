/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Get & Set Configuration items on the EEPROM
 */
#include "config.h"

uint8_t CONFIG_SETTING_CACHE[CONFIG_SETTING_CACHE_SIZE] = {0};
uint8_t CONFIG_VALUE_CACHE[CONFIG_VALUE_CACHE_SIZE] = {0};

static int8_t CONFIG_TIMEZONE_OFFSETS[CONFIG_TIMEZONE_COUNT] = {
    0,
    -1 * (12 * 60 + 00) / 15, // -12:00
    -1 * (11 * 60 + 00) / 15, // -11:00
    -1 * (10 * 60 + 00) / 15, // -10:00
    -1 * (9 * 60 + 00) / 15, // -09:00
    -1 * (8 * 60 + 00) / 15, // -08:00
    -1 * (7 * 60 + 00) / 15,
    -1 * (6 * 60 + 00) / 15,
    -1 * (5 * 60 + 00) / 15,
    -1 * (4 * 60 + 00) / 15,
    -1 * (3 * 60 + 30) / 15,
    -1 * (3 * 60 + 0) / 15,
    -1 * (2 * 60 + 0) / 15,
    -1 * (1 * 60 + 0) / 15,
     0 * (0 * 60 + 0) / 15, // + 00:00 UTC
    +1 * (1 * 60 + 0) / 15,
    +1 * (2 * 60 + 0) / 15,
    +1 * (3 * 60 + 0) / 15,
    +1 * (3 * 60 + 30) / 15,
    +1 * (4 * 60 + 0) / 15,
    +1 * (4 * 60 + 30) / 15, // +04:30
    +1 * (5 * 60 + 0) / 15, // +05:00
    +1 * (5 * 60 + 30) / 15, // +05:30
    +1 * (6 * 60 + 0) / 15,
    +1 * (7 * 60 + 0) / 15,
    +1 * (8 * 60 + 0) / 15,
    +1 * (9 * 60 + 0) / 15,
    +1 * (9 * 60 + 30) / 15, // +09:30
    +1 * (10 * 60 + 0) / 15,
    +1 * (10 * 60 + 30) / 15, // +10:30
    +1 * (11 * 60 + 0) / 15,
    +1 * (12 * 60 + 0) / 15, // +12:00
};

/**
 * ConfigGetByte()
 *     Description:
 *         Pull a byte from the EEPROM. If that byte is 0xFF, assume it's 0x00
 *     Params:
 *         uint8_t address - The address to read from
 *     Returns:
 *         uint8_t
 */
static inline uint8_t ConfigGetByte(uint8_t address)
{
    uint8_t value = 0;
    if (address < CONFIG_SETTING_CACHE_SIZE) {
        value = CONFIG_SETTING_CACHE[address];
    }
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
 *         uint8_t address - The address to read from
 *         uint8_t value - Value to set
 */
void ConfigSetByte(uint8_t address, uint8_t value)
{
    if (address < CONFIG_SETTING_CACHE_SIZE) {
        CONFIG_SETTING_CACHE[address] = value;
    }
    EEPROMWriteByte(address, value);
}

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
 * ConfigGetBytes()
 *     Description:
 *         Get a byte into the EEPROM and update cache
 *     Params:
 *         uint8_t address - The address to read from
 *         uint8_t *data - The data pointer
 *         uint8_t size - The length of the data in the pointer
 */
void ConfigGetBytes(uint8_t address, uint8_t *data, uint8_t size)
{
    uint8_t i = 0;
    for (i = 0; i < size; i++) {
        data[i] = ConfigGetByte(address);
        address++;
    }
}

/**
 * ConfigGetByteLowerNibble()
 *     Description:
 *         Get the lower nibble of a given byte from the EEPROM
 *     Params:
 *         uint8_t byte - The byte to get
 *     Returns:
 *         uint8_t - The value
 */
uint8_t ConfigGetByteLowerNibble(uint8_t byte)
{
    uint8_t value = ConfigGetByte(byte);
    return value & 0x0F;
}

/**
 * ConfigGetByteUpperNibble()
 *     Description:
 *         Get the upper nibble of a given byte from the EEPROM
 *     Params:
 *         uint8_t byte - The byte to get
 *     Returns:
 *         uint8_t - The value
 */
uint8_t ConfigGetByteUpperNibble(uint8_t byte)
{
    uint8_t value = ConfigGetByte(byte);
    return (value & 0xF0) >> 4;
}

/**
 * ConfigGetBuildWeek()
 *     Description:
 *         Get the build week
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetBuildWeek()
{
    return ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_WEEK);
}

/**
 * ConfigGetBuildYear()
 *     Description:
 *         Get the build year
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetBuildYear()
{
    return ConfigGetByte(CONFIG_BUILD_DATE_ADDRESS_YEAR);
}

/**
 * ConfigGetComfortLock()
 *     Description:
 *         Get the Comfort Lock Setting
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetComfortLock()
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
 *         uint8_t
 */
uint8_t ConfigGetComfortUnlock()
{
    return ConfigGetByteLowerNibble(CONFIG_SETTING_COMFORT_LOCKS);
}

/**
 * ConfigGetDistUnit()
 *     Description:
 *         Return the distance units that the vehicle is configured for
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetDistUnit()
{
    return ConfigGetByteUpperNibble(CONFIG_SETTING_BMBT_DIST_UNIT_ADDRESS);
}

/**
 * ConfigGetFirmwareVersionMajor()
 *     Description:
 *         Get the major firmware version
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetFirmwareVersionMajor()
{
    return ConfigGetByte(CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS);
}

/**
 * ConfigGetFirmwareVersionMinor()
 *     Description:
 *         Get the minor firmware version
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetFirmwareVersionMinor()
{
    return ConfigGetByte(CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS);
}

/**
 * ConfigGetFirmwareVersionPatch()
 *     Description:
 *         Get the patch firmware version
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetFirmwareVersionPatch()
{
    return ConfigGetByte(CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS);
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
 *         uint8_t
 */
uint8_t ConfigGetIKEType()
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
 *         uint8_t
 */
uint8_t ConfigGetLightingFeaturesActive()
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
 *         uint8_t
 */
uint8_t ConfigGetLMVariant()
{
    return ConfigGetByte(CONFIG_LM_VARIANT_ADDRESS);
}

/**
 * ConfigGetLog()
 *     Description:
 *         Get the log level for different systems
 *     Params:
 *         uint8_t system - The system to get the log mode for
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetLog(uint8_t system)
{
    uint8_t currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
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
 *         uint8_t
 */
uint8_t ConfigGetNavType()
{
    uint8_t value = ConfigGetByte(CONFIG_NAV_TYPE_ADDRESS);
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
    uint8_t snMSB = EEPROMReadByte(CONFIG_SN_ADDRESS_MSB);
    uint8_t snLSB = EEPROMReadByte(CONFIG_SN_ADDRESS_LSB);
    return (snMSB << 8) + snLSB;
}

/**
 * ConfigGetSetting()
 *     Description:
 *         Get a given setting from the EEPROM
 *     Params:
 *         uint8_t setting - The setting to get
 *     Returns:
 *         uint8_t - The value
 */
uint8_t ConfigGetSetting(uint8_t setting)
{
    uint8_t value = 0x00;
    // Catch invalid setting addresses
    if (setting >= CONFIG_SETTING_START_ADDRESS &&
        setting <= CONFIG_SETTING_END_ADDRESS
    ) {
        value = ConfigGetByte(setting);
    }
    return value;
}

/**
 * ConfigGetString()
 *     Description:
 *         Pull a string of <size> from the given address
 *     Params:
 *         uint8_t setting - The setting to get
 *     Returns:
 *         uint8_t - The value
 */
void ConfigGetString(uint8_t address, char *string, uint8_t size)
{
    uint8_t i = 0;
    for (i = 0; i < size; i++) {
        string[i] = ConfigGetByte(address);
        address++;
    }
}

/**
 * ConfigGetTelephonyFeaturesActive()
 *     Description:
 *         Check if any telephone features are active and return a boolean
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetTelephonyFeaturesActive()
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
 *         uint8_t
 */
uint8_t ConfigGetTempDisplay()
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
 *         uint8_t
 */
uint8_t ConfigGetTempUnit()
{
    return ConfigGetByteUpperNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY);
}

/**
 * ConfigGetTimeSource()
 *     Description:
 *         Return the configured source of time
 *     Returns:
 *         uint8_t - CONFIG_SETTING_AUTO_TIME_PHONE | CONFIG_SETTING_AUTO_TIME_GPS | CONFIG_SETTING_OFF
 */
uint8_t ConfigGetTimeSource()
{
    return ConfigGetByte(CONFIG_SETTING_AUTO_TIME) & (CONFIG_SETTING_AUTO_TIME_PHONE | CONFIG_SETTING_AUTO_TIME_GPS);
}


/**
 * ConfigGetTimeDST()
 *     Description:
 *         Return the configured DST state
 *     Returns:
 *         uint8_t - CONFIG_SETTING_AUTO_TIME_DST | CONFIG_SETTING_OFF
 */
uint8_t ConfigGetTimeDST()
{
    return ConfigGetByte(CONFIG_SETTING_AUTO_TIME) & CONFIG_SETTING_AUTO_TIME_DST;
}

/**
 * ConfigGetTimeOffset()
 *     Description:
 *         Return the configured time offset in minutes
 *     Returns:
 *         int16_t - time offset in minutes
 */
int16_t ConfigGetTimeOffset()
{
    return CONFIG_TIMEZONE_OFFSETS[ConfigGetTimeOffsetIndex()] * 15;
}

/**
 * ConfigGetTimeOffsetIndex()
 *     Description:
 *         Return the configured time offset index
 *     Returns:
 *         int16_t - time offset as table index
 */
uint8_t ConfigGetTimeOffsetIndex()
{
    return (ConfigGetByte(CONFIG_SETTING_AUTO_TIME) & CONFIG_SETTING_AUTO_TIME_TZ) >> 3;
}

/**
 * ConfigGetTrapCount()
 *     Description:
 *         Get the number of times a trap has been triggered
 *     Params:
 *         uint8_t trap - The trap
 *     Returns:
 *         void
 */
uint8_t ConfigGetTrapCount(uint8_t trap)
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
uint8_t ConfigGetTrapLast()
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
 *         uint8_t
 */
uint8_t ConfigGetUIMode()
{
    return ConfigGetByte(CONFIG_UI_MODE_ADDRESS);
}

/**
 * ConfigGetVehicleType()
 *     Description:
 *         Get the vehicle type
 *     Params:
 *         None
 *     Returns:
 *         uint8_t
 */
uint8_t ConfigGetVehicleType()
{
    return ConfigGetByteLowerNibble(CONFIG_VEHICLE_TYPE_ADDRESS);
}

/**
 * ConfigGetValue()
 *     Description:
 *         Get a given value from the EEPROM
 *     Params:
 *         uint8_t value - The value to get
 *     Returns:
 *         uint8_t - The value
 */
uint8_t ConfigGetValue(uint8_t value)
{
    uint8_t data = 0x00;
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
 *         uint8_t *
 *     Returns:
 *         void
 */
void ConfigGetVehicleIdentity(uint8_t *vin)
{
    uint8_t vinAddress[] = CONFIG_VEHICLE_VIN;
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
    ConfigSetValue(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_MSB, failureCount >> 8);
    ConfigSetValue(CONFIG_INFO_BC127_BOOT_FAIL_COUNTER_LSB, failureCount & 0xFF);
}

/**
 * ConfigSetBootloaderMode()
 *     Description:
 *         Set the bootloader mode
 *     Params:
 *         uint8_t bootloaderMode - The Bootloader mode to set
 *     Returns:
 *         void
 */
void ConfigSetBootloaderMode(uint8_t bootloaderMode)
{
    ConfigSetByte(CONFIG_BOOTLOADER_MODE_ADDRESS, bootloaderMode);
}

/**
 * ConfigSetBytes()
 *     Description:
 *         Set a byte into the EEPROM and update cache
 *     Params:
 *         uint8_t address - The address to read from
 *         const uint8_t *data - The data pointer
 *         uint8_t size - The length of the data in the pointer
 */
void ConfigSetBytes(uint8_t address, const uint8_t *data, uint8_t size)
{
    uint8_t i = 0;
    for (i = 0; i < size; i++) {
        ConfigSetByte(address, data[i]);
        address++;
    }
}

/**
 * ConfigSetByteLowerNibble()
 *     Description:
 *         Set a given setting into the lower nibble of a byte in the EEPROM
 *     Params:
 *         uint8_t setting - The setting to set
 *         uint8_t value - The value to set
 *     Returns:
 *         void
 */
void ConfigSetByteLowerNibble(uint8_t setting, uint8_t value)
{
    uint8_t currentValue = ConfigGetByte(setting);
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
 *         uint8_t setting - The setting to set
 *         uint8_t value - The value to set
 *     Returns:
 *         void
 */
void ConfigSetByteUpperNibble(uint8_t setting, uint8_t value)
{
    uint8_t currentValue = ConfigGetByte(setting);
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
 *         uint8_t comfortLock - The comfort lock setting
 *     Returns:
 *         void
 */
void ConfigSetComfortLock(uint8_t comfortLock)
{
    ConfigSetByteUpperNibble(CONFIG_SETTING_COMFORT_LOCKS, comfortLock);
}

/**
 * ConfigSetComfortUnlock()
 *     Description:
 *         Set the comfort unlock setting
 *     Params:
 *         uint8_t comfortUnlock - The comfort unlock setting
 *     Returns:
 *         void
 */
void ConfigSetComfortUnlock(uint8_t comfortUnlock)
{
    ConfigSetByteLowerNibble(CONFIG_SETTING_COMFORT_LOCKS, comfortUnlock);
}

/**
 * ConfigSetDistUnit()
 *     Description:
 *         Set the temperature unit setting
 *     Params:
 *         uint8_t distUnit - The distance unit
 *     Returns:
 *         void
 */
void ConfigSetDistUnit(uint8_t distUnit)
{
    ConfigSetByteUpperNibble(CONFIG_SETTING_BMBT_DIST_UNIT, distUnit);
}

/**
 * ConfigSetFirmwareVersion()
 *     Description:
 *         Set the firmware version
 *     Params:
 *         uint8_t major - The major version
 *         uint8_t minor - The minor version
 *         uint8_t patch - The patch version
 *     Returns:
 *         void
 */
void ConfigSetFirmwareVersion(
    uint8_t major,
    uint8_t minor,
    uint8_t patch
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
 *         uint8_t ikeType - The IKE type
 *     Returns:
 *         void
 */
void ConfigSetIKEType(uint8_t ikeType)
{
    ConfigSetByteUpperNibble(CONFIG_VEHICLE_TYPE_ADDRESS, ikeType);
}

/***
 * ConfigSetLMVariant()
 *     Description:
 *         Set the Light Module variant
 *     Params:
 *         uint8_t version
 *     Returns:
 *         void
 */
void ConfigSetLMVariant(uint8_t variant)
{
    ConfigSetByte(CONFIG_LM_VARIANT_ADDRESS, variant);
}

/**
 * ConfigSetLog()
 *     Description:
 *         Set the log level for different systems
 *     Params:
 *         uint8_t system - The system to set the log mode for
 *         uint8_t mode - The mode
 *     Returns:
 *         void
 */
void ConfigSetLog(uint8_t system, uint8_t mode)
{
    uint8_t currentSetting = ConfigGetByte(CONFIG_SETTING_LOG_ADDRESS);
    uint8_t currentVal = (currentSetting >> system) & 1;
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
 *         uint8_t version
 *     Returns:
 *         void
 */
void ConfigSetNavType(uint8_t type)
{
    ConfigSetByte(CONFIG_NAV_TYPE_ADDRESS, type);
}

/**
 * ConfigSetSetting()
 *     Description:
 *         Set a given setting into the EEPROM
 *     Params:
 *         uint8_t setting - The setting to set
 *         uint8_t value - The value to set
 *     Returns:
 *         void
 */
void ConfigSetSetting(uint8_t setting, uint8_t value)
{
    // Catch invalid setting addresses
    if (setting >= CONFIG_SETTING_START_ADDRESS &&
        setting <= CONFIG_SETTING_END_ADDRESS
    ) {
        ConfigSetByte(setting, value);
    }
}

/**
 * ConfigSetString()
 *     Description:
 *         Write a string to a given address
 *     Params:
 *         uint8_t address - The start address
 *         char *string - The string to write to memory
 *         uint8_t size - The size of the string
 *     Returns:
 *         void
 */
void ConfigSetString(uint8_t address, char *string, uint8_t size)
{
    uint8_t i = 0;
    for (i = 0; i < size; i++) {
        ConfigSetByte(address, string[i]);
        address++;
    }
    ConfigSetByte(address, 0);
}

/**
 * ConfigSetTempDisplay()
 *     Description:
 *         Set the temperature display setting
 *     Params:
 *         uint8_t tempDisplay - The temperature display setting
 *     Returns:
 *         void
 */
void ConfigSetTempDisplay(uint8_t tempDisplay)
{
    ConfigSetByteLowerNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY, tempDisplay);
}

/**
 * ConfigSetTempUnit()
 *     Description:
 *         Set the temperature unit setting
 *     Params:
 *         uint8_t tempUnit - The temperature unit
 *     Returns:
 *         void
 */
void ConfigSetTempUnit(uint8_t tempUnit)
{
    ConfigSetByteUpperNibble(CONFIG_SETTING_BMBT_TEMP_DISPLAY, tempUnit);
}

/**
 * ConfigSetTimeDST()
 *     Description:
 *         Set the DST state
 *     Params:
 *         uint8_t dst - CONFIG_SETTING_TIME_DST | CONFIG_SETTING_OFF
 *     Returns:
 *         void
 */
void ConfigSetTimeDST(uint8_t dst)
{
    if (dst == 1) {
        dst = CONFIG_SETTING_AUTO_TIME_DST;
    }
    uint8_t val = (ConfigGetByte(CONFIG_SETTING_AUTO_TIME) & 0b11111011) | dst;
    ConfigSetByte(CONFIG_SETTING_AUTO_TIME, val);
}

/**
 * ConfigSetTimeOffset()
 *     Description:
 *         Set the timezone offset
 *     Params:
 *         int16_t offset - The offset in minutes
 *     Returns:
 *         void
 */
void ConfigSetTimeOffset(int16_t offset)
{
    uint8_t idx;
    int16_t calculatedOffset = offset / 15;
    for (idx = 1; idx < CONFIG_TIMEZONE_COUNT; idx++) {
        if (calculatedOffset == CONFIG_TIMEZONE_OFFSETS[idx]) {
            ConfigSetTimeOffsetIndex(idx);
        }
    }
}

/**
 * ConfigSetTimeOffsetIndex()
 *     Description:
 *         Set the timezone offset
 *     Params:
 *         uint8_t idx - Time offset table index
 *     Returns:
 *         void
 */
void ConfigSetTimeOffsetIndex(uint8_t idx)
{
    uint8_t val = ( ConfigGetByte(CONFIG_SETTING_AUTO_TIME) & 0b00000111 ) | (idx << 3);
    ConfigSetByte(CONFIG_SETTING_AUTO_TIME, val);
}

/**
 * ConfigSetTimeSource()
 *     Description:
 *         Set the time source
 *     Params:
 *         uint8_t source - CONFIG_SETTING_AUTO_TIME_PHONE | CONFIG_SETTING_AUTO_TIME_GPS | CONFIG_SETTING_OFF
 *     Returns:
 *         void
 */
void ConfigSetTimeSource(uint8_t source)
{
    uint8_t val = (ConfigGetByte(CONFIG_SETTING_AUTO_TIME) & 0b11111100) | source;
    ConfigSetByte(CONFIG_SETTING_AUTO_TIME, val);
}

/**
 * ConfigSetTrapCount()
 *     Description:
 *         Set the trap count for the given trap
 *     Params:
 *         uint8_t trap - The trap triggered
 *         uint8_t count - The number
 *     Returns:
 *         void
 */
void ConfigSetTrapCount(uint8_t trap, uint8_t count)
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
 *         uint8_t trap - The trap triggered
 *     Returns:
 *         void
 */
void ConfigSetTrapIncrement(uint8_t trap)
{
    uint8_t count = ConfigGetTrapCount(trap);
    ConfigSetTrapCount(trap, count + 1);
    ConfigSetTrapLast(trap);
}

/**
 * ConfigSetTrapLast()
 *     Description:
 *         Set the last trap that was raised
 *     Params:
 *         uint8_t trap - The trap triggered
 *     Returns:
 *         void
 */
void ConfigSetTrapLast(uint8_t trap)
{
    ConfigSetByte(CONFIG_TRAP_LAST_ERR, trap);
}

/**
 * ConfigSetUIMode()
 *     Description:
 *         Set the UI mode
 *     Params:
 *         uint8_t uiMode - The UI mode to set
 *     Returns:
 *         void
 */
void ConfigSetUIMode(uint8_t uiMode)
{
    ConfigSetByte(CONFIG_UI_MODE_ADDRESS, uiMode);
}

/**
 * ConfigSetValue()
 *     Description:
 *         Ssave the given config value at the given address
 *     Params:
 *         uint8_t address - The address to store the data at
 *     Returns:
 *         void
 */
void ConfigSetValue(uint8_t address, uint8_t value)
{
    // Catch invalid setting addresses
    if (address >= CONFIG_VALUE_START_ADDRESS &&
        address <= CONFIG_VALUE_END_ADDRESS
    ) {
        ConfigSetByte(address, value);
    }
}

/**
 * ConfigSetVehicleType()
 *     Description:
 *         Set the vehicle type
 *     Params:
 *         uint8_t vehicleType - The vehicle type
 *     Returns:
 *         void
 */
void ConfigSetVehicleType(uint8_t vehicleType)
{
    ConfigSetByteLowerNibble(CONFIG_VEHICLE_TYPE_ADDRESS, vehicleType);
}

/**
 * ConfigSetVehicleIdentity()
 *     Description:
 *         Set the vehicle VIN
 *     Params:
 *         uint8_t *vin - The array to populate
 *     Returns:
 *         void
 */
void ConfigSetVehicleIdentity(uint8_t *vin)
{
    uint8_t vinAddress[] = CONFIG_VEHICLE_VIN_ADDRESS;
    uint8_t i;
    for (i = 0; i < 5; i++) {
        ConfigSetByte(vinAddress[i], vin[i]);
    }
}
