/*
 * File:   config.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description: 
 *     Get & Set Configuration items on the EEPROM
 */
#ifndef CONFIG_H
#define CONFIG_H
#include "eeprom.h"
#include "ibus.h"

#define CONFIG_SN_ADDRESS {0x00, 0x01}
#define CONFIG_VERSION_ADDRESS {0x02, 0x03}
#define CONFIG_BOOTLOADER_MODE_ADDRESS 0x04
#define CONFIG_DEVICE_DEBUG_SETTINGS_ADDRESS 0x05
#define CONFIG_UI_MODE_ADDRESS 0x06

unsigned char ConfigGetUIMode();
void ConfigSetBootloaderMode(unsigned char);
void ConfigSetUIMode(unsigned char);
#endif /* CONFIG_H */
