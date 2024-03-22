/*
 * File:   eeprom.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     EEPROM mechanisms
 */
#include "eeprom.h"

// These values constitute the SOA mode for each SPI module
static const uint8_t SPI_SDO_MODES[] = {7, 10, 23};

// These values constitute the SCK mode for each SPI module
static const uint8_t SPI_SCK_MODES[] = {8, 11, 24};

/**
 * EEPROMInit()
 *     Description:
 *         Initialize the EEPROM on SPI1 with the predefined values
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void EEPROMInit()
{
    uint8_t spiModuleIndex = EEPROM_SPI_MODULE - 1;
    EEPROM_CS_IO_MODE = 0;
    EEPROM_CS_PIN = 1;
    // Disable the Module & associated IRQs
    SetSPIIE(spiModuleIndex, 0);
    SetSPITXIE(spiModuleIndex, 0);
    SetSPIRXIE(spiModuleIndex, 0);
    SPI1CON1L = 0;
    SPI1STATLbits.SPIRBF = 0;
    // Unlock the programmable pin register
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    // Data Input
    _SDI1R = EEPROM_SDI_RPIN;
    // Set the SCK Output
    UtilsSetRPORMode(EEPROM_SCK_RPIN, SPI_SCK_MODES[spiModuleIndex]);
    // Set the SDO Output
    UtilsSetRPORMode(EEPROM_SDO_RPIN, SPI_SDO_MODES[spiModuleIndex]);
    // Lock the programmable pin register
    __builtin_write_OSCCONL(OSCCON & 0x40);
    SPI1BRGL = EEPROM_BRG;
    SPI1STATLbits.SPIROV = 0;
    // Enable Module | Set CKE to active -> idle | Master Enable
    SPI1CON1L = 0b1000000100100000;
}

/**
 * EEPROMSend()
 *     Description:
 *         Write to the SPI buffer and return the received value
 *     Params:
 *         char data - The data to transfer to the EEPROM
 *     Returns:
 *         unsigned char - The 8-bit byte returned from the EEPROM
 */
static unsigned char EEPROMSend(char data)
{
    SPI1BUFL = data;
    while (!SPI1STATLbits.SPIRBF);
    return SPI1BUFL;
}

/**
 * EEPROMEnableWrite()
 *     Description:
 *         Perform the necessary actions to set up the EEPROM for writing
 *     Params:
 *         void
 *     Returns:
 *         void
 */
static void EEPROMEnableWrite()
{
    // Wait until EEPROM is not busy
    EEPROMIsReady();
    EEPROM_CS_PIN = 0;
    EEPROMSend(EEPROM_COMMAND_WREN);
    EEPROM_CS_PIN = 1;
}

/**
 * EEPROMErase()
 *     Description:
 *         Erase the entire contents of the EEPROM
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void EEPROMErase()
{
    EEPROMEnableWrite();
    EEPROM_CS_PIN = 0;
    EEPROMSend(EEPROM_COMMAND_CE);
    EEPROM_CS_PIN = 1;
}

/**
 * EEPROMIsReady()
 *     Description:
 *         Check with the EEPROM to see if it's ready to be written to. If it
 *         is not, this function blocks until it is ready (status 0x00).
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void EEPROMIsReady()
{
    char status = EEPROM_STATUS_BUSY;
    while (status & EEPROM_STATUS_BUSY) {
        EEPROM_CS_PIN = 0;
        EEPROMSend(EEPROM_COMMAND_RDSR);
        status = EEPROMSend(EEPROM_COMMAND_GET);
        EEPROM_CS_PIN = 1;
    }
}

/**
 * EEPROMReadByte()
 *     Description:
 *         Read a byte from the EEPROM at the given address and return it
 *     Params:
 *         uint32_t - The memory address of the byte to retrieve
 *     Returns:
 *         unsigned char - The byte at the given address
 */
unsigned char EEPROMReadByte(uint32_t address)
{
    EEPROMIsReady();
    EEPROM_CS_PIN = 0;
    EEPROMSend(EEPROM_COMMAND_READ);
    // The HW1 boards use a 1024kB EEPROM while the HW2 boards use a
    // 128kB EEPROM. This means that we need not send as many address bytes
    if (UtilsGetBoardVersion() == BOARD_VERSION_ONE) {
        EEPROMSend(address >> 16 & 0xFF);
    }
    EEPROMSend(address >> 8 & 0xFF);
    EEPROMSend(address & 0xFF);
    // Cast return of EEPROM send to an 8-bit byte, since the returned register
    // is always 16 bits
    unsigned char data = (unsigned char)((uint8_t )EEPROMSend(EEPROM_COMMAND_GET));
    EEPROM_CS_PIN = 1;
    return data;
}

/**
 * EEPROMWriteByte()
 *     Description:
 *         Check with the EEPROM to see if it's ready to be written to. If it
 *         is not, this function blocks until it is ready (status 0x00).
 *     Params:
 *         uint32_t address - The memory address of the byte to retrieve
 *         unsigned char data - The 8-bit byte to write
 *     Returns:
 *         void
 */
void EEPROMWriteByte(uint32_t address, unsigned char data)
{
    EEPROMEnableWrite();
    EEPROM_CS_PIN = 0;
    EEPROMSend(EEPROM_COMMAND_WRITE);
    // The HW1 boards use a 1024kB EEPROM while the HW2 boards use a
    // 128kB EEPROM. This means that we need not send as many address bytes
    if (UtilsGetBoardVersion() == BOARD_VERSION_ONE) {
        EEPROMSend(address >> 16 & 0xFF);
    }
    EEPROMSend(address >> 8 & 0xFF);
    EEPROMSend(address & 0xFF);
    EEPROMSend(data);
    EEPROM_CS_PIN = 1;
}
