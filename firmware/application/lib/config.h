/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#ifndef CONFIG_H
#define CONFIG_H
#include "eeprom.h"

#define CONFIG_CACHE_VALUES 32
/* EEPROM 0x00 - 0x04: Reserved for the BlueBus */
#define CONFIG_SN_ADDRESS {0x00, 0x01}
#define CONFIG_VERSION_ADDRESS {0x02, 0x03}
#define CONFIG_BOOTLOADER_MODE_ADDRESS 0x04
/* EEPROM 0x05 - 0x09: IBus / Vehicle Data */
#define CONFIG_UI_MODE_ADDRESS 0x05
#define CONFIG_NAV_TYPE_ADDRESS 0x06
#define CONFIG_VEHICLE_TYPE_ADDRESS 0x07
/* EEPROM 0x0A - 0x14: User Configurable Settings */
#define CONFIG_SETTING_LOG_ADDRESS 0x0A
#define CONFIG_SETTING_HFP_ADDRESS 0x0B
#define CONFIG_SETTING_METADATA_MODE_ADDRESS 0x0C
#define CONFIG_SETTING_AUTOPLAY_ADDRESS 0x0D
#define CONFIG_SETTING_BMBT_DEFAULT_MENU_ADDRESS 0x0E
#define CONFIG_SETTING_OT_BLINKERS_ADDRESS 0x0F

#define CONFIG_DEVICE_LOG_BT 1
#define CONFIG_DEVICE_LOG_IBUS 2
#define CONFIG_DEVICE_LOG_SYSTEM 3
#define CONFIG_DEVICE_LOG_UI 4

#define CONFIG_SETTING_OFF 0x00
#define CONFIG_SETTING_ON 0x01
#define CONFIG_SETTING_LOG CONFIG_SETTING_LOG_ADDRESS
#define CONFIG_SETTING_HFP CONFIG_SETTING_HFP_ADDRESS
#define CONFIG_SETTING_METADATA_MODE CONFIG_SETTING_METADATA_MODE_ADDRESS
#define CONFIG_SETTING_AUTOPLAY CONFIG_SETTING_AUTOPLAY_ADDRESS
#define CONFIG_SETTING_BMBT_DEFAULT_MENU CONFIG_SETTING_BMBT_DEFAULT_MENU_ADDRESS
#define CONFIG_SETTING_OT_BLINKERS CONFIG_SETTING_OT_BLINKERS_ADDRESS

unsigned char ConfigGetLog(unsigned char);
unsigned char ConfigGetNavType();
unsigned char ConfigGetSetting(unsigned char);
unsigned char ConfigGetUIMode();
unsigned char ConfigGetVehicleType();
void ConfigSetBootloaderMode(unsigned char);
void ConfigSetLog(unsigned char, unsigned char);
void ConfigSetSetting(unsigned char, unsigned char);
void ConfigSetNavType(unsigned char);
void ConfigSetUIMode(unsigned char);
void ConfigSetVehicleType(unsigned char);
#endif /* CONFIG_H */
