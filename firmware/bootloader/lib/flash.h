/*
 * File: flash.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Read / Write Flash Memory
 */
#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>
#include <xc.h>

uint8_t FlashErasePage(uint32_t);
uint8_t FlashWriteDWORDAddress(uint32_t, uint32_t, uint32_t);

#endif /* FLASH_H */
