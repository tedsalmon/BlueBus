/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#ifndef CONFIG_H
#define CONFIG_H
#include "eeprom.h"

#define CONFIG_CACHE_VALUES 96
/* EEPROM 0x00 - 0x07: Reserved for the BlueBus */
#define CONFIG_SN_ADDRESS_MSB 0x00
#define CONFIG_SN_ADDRESS_LSB 0x01
#define CONFIG_FIRMWARE_VERSION_MAJOR_ADDRESS 0x02
#define CONFIG_FIRMWARE_VERSION_MINOR_ADDRESS 0x03
#define CONFIG_FIRMWARE_VERSION_PATCH_ADDRESS 0x04
#define CONFIG_BUILD_DATE_ADDRESS_WEEK 0x05
#define CONFIG_BUILD_DATE_ADDRESS_YEAR 0x06
#define CONFIG_BOOTLOADER_MODE_ADDRESS 0x07
/* EEPROM 0x08 - 0x0E: Reserved for Trap counters */
#define CONFIG_TRAP_OSC 0x08
#define CONFIG_TRAP_ADDR 0x09
#define CONFIG_TRAP_STACK 0x0A
#define CONFIG_TRAP_MATH 0x0B
#define CONFIG_TRAP_NVM 0x0C
#define CONFIG_TRAP_GEN 0x0D
#define CONFIG_TRAP_LAST_ERR 0x0E
/* EEPROM 0x0F - 0x19: IBus / Vehicle Data */
#define CONFIG_UI_MODE_ADDRESS 0x0F
#define CONFIG_NAV_TYPE_ADDRESS 0x10
#define CONFIG_VEHICLE_TYPE_ADDRESS 0x11
#define CONFIG_VEHICLE_VIN_ADDRESS {0x12, 0x13, 0x14, 0x15, 0x16}
/* EEPROM 0x1A - 0x50: User Configurable Settings */
#define CONFIG_SETTING_LOG_ADDRESS 0x1A
#define CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS 0x1B
/* Config 0x1C - 0x24: UI Settings */
#define CONFIG_SETTING_METADATA_MODE_ADDRESS 0x1C
#define CONFIG_SETTING_BMBT_DEFAULT_MENU_ADDRESS 0x1D
#define CONFIG_SETTING_BMBT_DASHBOARD_OBC_ADDRESS 0x1E
#define CONFIG_SETTING_BMBT_TEMP_HEADERS_ADDRESS 0x1F
/* Config 0x25 - 0x35: Comfort Settings */
#define CONFIG_SETTING_COMFORT_BLINKERS_ADDRESS 0x25
#define CONFIG_SETTING_COMFORT_LOCKS_ADDRESS 0x26
#define CONFIG_SETTING_COMFORT_MIRRORS_ADDRESS 0x27
#define CONFIG_SETTING_COMFORT_WELCOME_LIGHTS_ADDRESS 0x28
#define CONFIG_SETTING_COMFORT_HOME_LIGHTS_ADDRESS 0x29
#define CONFIG_SETTING_COMFORT_RUNNING_LIGHTS_ADDRESS 0x2A
#define CONFIG_SETTING_COMFORT_DATE_TIME_ADDRESS 0x2B
/* Config 0x36 - 0x3E: Telephony Settings */
#define CONFIG_SETTING_HFP_ADDRESS 0x36
#define CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL_ADDRESS 0x37
#define CONFIG_SETTING_MIC_GAIN_ADDRESS 0x38
#define CONFIG_SETTING_MIC_BIAS_ADDRESS 0x39
#define CONFIG_SETTING_TEL_VOL_ADDRESS 0x3A
#define CONFIG_SETTING_MIC_PREAMP_ADDRESS 0x3B
/* Config 0x40 - 0x50: Audio Settings */
#define CONFIG_SETTING_AUTOPLAY_ADDRESS 0x40
#define CONFIG_SETTING_DAC_AUDIO_VOL_ADDRESS 0x41
#define CONFIG_SETTING_USE_SPDIF_INPUT_ADDRESS 0x42

#define CONFIG_DEVICE_LOG_BT 2
#define CONFIG_DEVICE_LOG_IBUS 3
#define CONFIG_DEVICE_LOG_SYSTEM 4
#define CONFIG_DEVICE_LOG_UI 5

#define CONFIG_SETTING_OFF 0x00
#define CONFIG_SETTING_ON 0x01
#define CONFIG_SETTING_ENABLED 0x00
#define CONFIG_SETTING_DISABLED 0xFF
#define CONFIG_SETTING_COMFORT_LOCK_10KM 0x01
#define CONFIG_SETTING_COMFORT_LOCK_20KM 0x02
#define CONFIG_SETTING_COMFORT_UNLOCK_POS_1 0x01
#define CONFIG_SETTING_COMFORT_UNLOCK_POS_0 0x02
/* EEPROM 0x1A - 0x50: User Configurable Settings */
#define CONFIG_SETTING_LOG CONFIG_SETTING_LOG_ADDRESS
#define CONFIG_SETTING_POWEROFF_TIMEOUT CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS
/* Config 0x1C - 0x24: UI Settings */
#define CONFIG_SETTING_METADATA_MODE CONFIG_SETTING_METADATA_MODE_ADDRESS
#define CONFIG_SETTING_BMBT_DEFAULT_MENU CONFIG_SETTING_BMBT_DEFAULT_MENU_ADDRESS
#define CONFIG_SETTING_BMBT_DASHBOARD_OBC CONFIG_SETTING_BMBT_DASHBOARD_OBC_ADDRESS
#define CONFIG_SETTING_BMBT_TEMP_HEADERS CONFIG_SETTING_BMBT_TEMP_HEADERS_ADDRESS
/* Config 0x25 - 0x35: Comfort Settings */
#define CONFIG_SETTING_COMFORT_BLINKERS CONFIG_SETTING_COMFORT_BLINKERS_ADDRESS
#define CONFIG_SETTING_COMFORT_LOCKS CONFIG_SETTING_COMFORT_LOCKS_ADDRESS
#define CONFIG_SETTING_COMFORT_MIRRORS CONFIG_SETTING_COMFORT_MIRRORS_ADDRESS
#define CONFIG_SETTING_COMFORT_WELCOME_LIGHTS CONFIG_SETTING_COMFORT_WELCOME_LIGHTS_ADDRESS
#define CONFIG_SETTING_COMFORT_HOME_LIGHTS CONFIG_SETTING_COMFORT_HOME_LIGHTS_ADDRESS
#define CONFIG_SETTING_COMFORT_RUNNING_LIGHTS CONFIG_SETTING_COMFORT_RUNNING_LIGHTS_ADDRESS
#define CONFIG_SETTING_COMFORT_DATE_TIME CONFIG_SETTING_COMFORT_DATE_TIME_ADDRESS
/* Config 0x36 - 0x3E: Telephony Settings */
#define CONFIG_SETTING_HFP CONFIG_SETTING_HFP_ADDRESS
#define CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL_ADDRESS
#define CONFIG_SETTING_MIC_GAIN CONFIG_SETTING_MIC_GAIN_ADDRESS
#define CONFIG_SETTING_MIC_BIAS CONFIG_SETTING_MIC_BIAS_ADDRESS
#define CONFIG_SETTING_TEL_VOL CONFIG_SETTING_TEL_VOL_ADDRESS
#define CONFIG_SETTING_MIC_PREAMP CONFIG_SETTING_MIC_PREAMP_ADDRESS
/* Config 0x40 - 0x50: Audio Settings */
#define CONFIG_SETTING_AUTOPLAY CONFIG_SETTING_AUTOPLAY_ADDRESS
#define CONFIG_SETTING_DAC_AUDIO_VOL CONFIG_SETTING_DAC_AUDIO_VOL_ADDRESS
#define CONFIG_SETTING_DAC_TEL_VOL CONFIG_SETTING_DAC_TEL_VOL_ADDRESS
#define CONFIG_SETTING_USE_SPDIF_INPUT CONFIG_SETTING_USE_SPDIF_INPUT_ADDRESS
/* Data Boundary Helpers */
#define CONFIG_SETTING_START_ADDRESS 0x1A
#define CONFIG_SETTING_END_ADDRESS 0x50


unsigned char ConfigGetByte(unsigned char);
unsigned char ConfigGetBuildWeek();
unsigned char ConfigGetBuildYear();
unsigned char ConfigGetComfortLock();
unsigned char ConfigGetComfortUnlock();
unsigned char ConfigGetFirmwareVersionMajor();
unsigned char ConfigGetFirmwareVersionMinor();
unsigned char ConfigGetFirmwareVersionPatch();
void ConfigGetFirmwareVersionString(char *);
unsigned char ConfigGetIKEType();
unsigned char ConfigGetLog(unsigned char);
unsigned char ConfigGetNavType();
unsigned char ConfigGetPoweroffTimeoutDisabled();
uint16_t ConfigGetSerialNumber();
unsigned char ConfigGetSetting(unsigned char);
unsigned char ConfigGetTrapCount(unsigned char);
unsigned char ConfigGetTrapLast();
unsigned char ConfigGetUIMode();
unsigned char ConfigGetVehicleType();
void ConfigGetVehicleIdentity(unsigned char *);
void ConfigSetBootloaderMode(unsigned char);
void ConfigSetComfortLock(unsigned char);
void ConfigSetComfortUnlock(unsigned char);
void ConfigSetFirmwareVersion(unsigned char, unsigned char, unsigned char);
void ConfigSetIKEType(unsigned char);
void ConfigSetLog(unsigned char, unsigned char);
void ConfigSetSetting(unsigned char, unsigned char);
void ConfigSetNavType(unsigned char);
void ConfigSetPoweroffTimeoutDisabled(unsigned char);
void ConfigSetTrapCount(unsigned char, unsigned char);
void ConfigSetTrapIncrement(unsigned char);
void ConfigSetTrapLast(unsigned char);
void ConfigSetUIMode(unsigned char);
void ConfigSetVehicleType(unsigned char);
void ConfigSetVehicleIdentity(unsigned char *);
#endif /* CONFIG_H */
