/*
 * File: flash.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Read / Write Flash Memory
 */
#include "flash.h"

/**
 * FlashWriteDWORDAddress()
 *     Description:
 *         Write a DWORD to flash at the given address
 *     Params:
 *         uint32_t address - The beginning address to write to
 *         uint32_t data - The first WORD to write
 *         uint32_t data2 - The second WORD to write
 *     Returns:
 *         uint8_t - If an error occurred while writing flash
 */
uint8_t FlashWriteDWORDAddress(uint32_t address, uint32_t data, uint32_t data2)
{
    // Initialize NVMCON for writing
    NVMCON = 0x4001;
    // Set the TBLPAG to the base address of write latches
    TBLPAG = 0xFA;
    NVMADRU = address >> 16;
    NVMADR = address & 0xFFFF;

    // Write the lower byte
    __builtin_tblwtl(0, data & 0x0000ffff);
    // Write the upper byte
    __builtin_tblwth(0, (data & 0xffff0000) >> 16);

    // Write the lower byte
    __builtin_tblwtl(2, data2 & 0x0000ffff);
    // Write the upper byte
    __builtin_tblwth(2, (data2 & 0xffff0000) >> 16);

    __builtin_disi(6);
    __builtin_write_NVM();
    while (NVMCONbits.WR);
    NVMCONbits.WREN = 0;
    return NVMCONbits.WRERR == 0;
}
