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
/* EEPROM 0x05 - 0x0B: Reserved for Trap counters */
#define CONFIG_TRAP_OSC 0x05
#define CONFIG_TRAP_ADDR 0x06
#define CONFIG_TRAP_STACK 0x07
#define CONFIG_TRAP_MATH 0x08
#define CONFIG_TRAP_NVM 0x09
#define CONFIG_TRAP_GEN 0x0A
#define CONFIG_TRAP_LAST_ERR 0x0B
/* EEPROM 0x0C - 0x0E: IBus / Vehicle Data */
#define CONFIG_UI_MODE_ADDRESS 0x0C
#define CONFIG_NAV_TYPE_ADDRESS 0x0D
#define CONFIG_VEHICLE_TYPE_ADDRESS 0x0E
/* EEPROM 0x0F - 0x19: User Configurable Settings */
#define CONFIG_SETTING_LOG_ADDRESS 0x0F
#define CONFIG_SETTING_HFP_ADDRESS 0x10
#define CONFIG_SETTING_METADATA_MODE_ADDRESS 0x11
#define CONFIG_SETTING_AUTOPLAY_ADDRESS 0x12
#define CONFIG_SETTING_BMBT_DEFAULT_MENU_ADDRESS 0x13
#define CONFIG_SETTING_OT_BLINKERS_ADDRESS 0x14
#define CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS 0x15
#define CONFIG_SETTING_TCU_MODE_ADDRESS 0x16
#define CONFIG_SETTING_MIC_GAIN_ADDRESS 0x17
#define CONFIG_SETTING_DAC_VOL_ADDRESS 0x18

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
#define CONFIG_SETTING_POWEROFF_TIMEOUT CONFIG_SETTING_POWEROFF_TIMEOUT_ADDRESS
#define CONFIG_SETTING_TCU_MODE CONFIG_SETTING_TCU_MODE_ADDRESS
#define CONFIG_SETTING_MIC_GAIN CONFIG_SETTING_MIC_GAIN_ADDRESS
#define CONFIG_SETTING_DAC_VOL CONFIG_SETTING_DAC_VOL_ADDRESS

unsigned char ConfigGetByte(unsigned char);
unsigned char ConfigGetLog(unsigned char);
unsigned char ConfigGetNavType();
unsigned char ConfigGetPoweroffTimeout();
unsigned char ConfigGetSetting(unsigned char);
unsigned char ConfigGetTrapCount(unsigned char);
unsigned char ConfigGetTrapLast();
unsigned char ConfigGetUIMode();
unsigned char ConfigGetVehicleType();
void ConfigSetBootloaderMode(unsigned char);
void ConfigSetLog(unsigned char, unsigned char);
void ConfigSetSetting(unsigned char, unsigned char);
void ConfigSetNavType(unsigned char);
void ConfigSetPoweroffTimeout(unsigned char);
void ConfigSetTrapCount(unsigned char, unsigned char);
void ConfigSetTrapIncrement(unsigned char);
void ConfigSetTrapLast(unsigned char);
void ConfigSetUIMode(unsigned char);
void ConfigSetVehicleType(unsigned char);
#endif /* CONFIG_H */
