/*
 * File:   flash.c
 * Author: tsalmon
 *
 * Created on November 13, 2018, 5:28 PM
 */

#include "flash.h"

uint8_t FlashWriteAddress(uint32_t address, uint32_t data)
{
    // Initialize NVMCON for writing
    NVMCON = 0x4001;
    // Set the TBLPAG to the base address of write latches
    TBLPAG = 0xFA;
    NVMADRU = address >> 16;
    NVMADR = address & 0xFFFF;
    //perform TBLWT instructions to write necessary number of latches
    // Write to address low word
    __builtin_tblwtl(0, data & 0x0000ffff);            
    // Write to upper byte
    __builtin_tblwth(0, (data & 0xffff0000) >> 16);    

    __builtin_disi(6);
    __builtin_write_NVM();
    while (NVMCONbits.WR);
    NVMCONbits.WREN = 0;
    return NVMCONbits.WRERR == 0;
}
