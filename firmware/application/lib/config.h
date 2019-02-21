/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#ifndef CONFIG_H
#define CONFIG_H
#include "eeprom.h"

#define CONFIG_CACHE_VALUES 5

#define CONFIG_SN_ADDRESS {0x00, 0x01}
#define CONFIG_VERSION_ADDRESS {0x02, 0x03}
#define CONFIG_BOOTLOADER_MODE_ADDRESS 0x04
#define CONFIG_DEVICE_LOG_SETTINGS_ADDRESS 0x05
#define CONFIG_UI_MODE_ADDRESS 0x06

#define CONFIG_DEVICE_LOG_BT 1
#define CONFIG_DEVICE_LOG_IBUS 2
#define CONFIG_DEVICE_LOG_SYSTEM 3
#define CONFIG_DEVICE_LOG_UI 4

unsigned char ConfigGetLog(unsigned char);
unsigned char ConfigGetUIMode();
void ConfigSetBootloaderMode(unsigned char);
void ConfigSetLog(unsigned char, unsigned char);
void ConfigSetUIMode(unsigned char);
#endif /* CONFIG_H */
