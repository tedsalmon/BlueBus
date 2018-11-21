/*
 * File: protocol.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a simple message protocol for the bootloader
 */
#include "protocol.h"

/**
 * ProtocolFlashWrite()
 *     Description:
 *         Take a Flash Write packet and write it out to NVM
 *     Params:
 *         UART_t *uart - The UART struct to use for communication
 *         ProtocolPacket_t *packet - The data packet structure
 *     Returns:
 *         void
 */
void ProtocolFlashErase()
{
    uint32_t address = 0x00000000;
    while (address <= BOOTLOADER_APPLICATION_END) {
         if (address < BOOTLOADER_BOOTLOADER_START ||
             address >= BOOTLOADER_APPLICATION_START
        ) {
            FlashErasePage(address);
        }
        address += _FLASH_ROW * 2;
    }
    // Reinitialize the reset vector
    uint32_t bootloaderStart = 0x00040000 + BOOTLOADER_BOOTLOADER_START;
    FlashWriteDWORDAddress(0x00000000, bootloaderStart, 0x00000000);
}

/**
 * ProtocolFlashWrite()
 *     Description:
 *         Take a Flash Write packet and write it out to NVM
 *     Params:
 *         UART_t *uart - The UART struct to use for communication
 *         ProtocolPacket_t *packet - The data packet structure
 *     Returns:
 *         void
 */
void ProtocolFlashWrite(UART_t *uart, ProtocolPacket_t *packet)
{
    uint32_t address = (
        ((uint32_t) 0 << 24) +
        ((uint32_t)packet->data[0] << 16) +
        ((uint32_t)packet->data[1] << 8) +
        ((uint32_t)packet->data[2])
    );
    if (address == 0x00000000) {
        ProtocolFlashErase();
    }
    uint8_t index = 3;
    uint8_t flashRes = 1;
    while (index < packet->dataSize && flashRes == 1) {
        // Do not allow the RESET or Bootloader to be overwritten
        if ((address >= BOOTLOADER_BOOTLOADER_START &&
            address < BOOTLOADER_APPLICATION_START) ||
            address < 0x04
        ) {
            // Skip the current dword, since it overwrites protected memory
            address += 0x2;
            index += 3;
        } else {
            uint32_t data = (
                ((uint32_t)0 << 24) + // "Phantom" Byte
                ((uint32_t)packet->data[index] << 16) +
                ((uint32_t)packet->data[index + 1] << 8) + 
                ((uint32_t)packet->data[index + 2])
            );
            uint32_t data2 = (
                ((uint32_t)0 << 24) + // "Phantom" Byte
                ((uint32_t)packet->data[index + 3] << 16) +
                ((uint32_t)packet->data[index + 4] << 8) + 
                ((uint32_t)packet->data[index + 5])
            );
            // We write two WORDs at a time, so jump the necessary
            // number of indices and addresses
            flashRes = FlashWriteDWORDAddress(address, data, data2);
            index += 6;
            address += 0x04;
        }
    }
    if (flashRes == 1) {
        ProtocolSendPacket(uart, PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK, 0, 0);
    } else {
        ProtocolSendPacket(uart, PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR, 0, 0);
    }
}

/**
 * ProtocolProcessPacket()
 *     Description:
 *         Attempt to create and verify a packet. It will return a bad
 *         error status if the data in the UART buffer does not align with
 *         the protocol.
 *     Params:
 *         UART_t *uart - The UART struct to use for communication
 *     Returns:
 *         ProtocolPacket_t
 */
ProtocolPacket_t ProtocolProcessPacket(UART_t *uart)
{
    struct ProtocolPacket_t packet;
    packet.status = PROTOCOL_PACKET_STATUS_INCOMPLETE;
    if (uart->rxQueueSize >= 2) {
        if (uart->rxQueueSize == (uint8_t) UARTGetOffsetByte(uart, 1)) {
            packet.command = UARTGetNextByte(uart);
            packet.dataSize = (uint8_t) UARTGetNextByte(uart) - PROTOCOL_CONTROL_PACKET_SIZE;
            uint8_t i;
            for (i = 0; i < packet.dataSize; i++) {
                if (uart->rxQueueSize > 0) {
                    packet.data[i] = UARTGetNextByte(uart);
                } else {
                    packet.data[i] = 0x00;
                }
            }
            unsigned char validation = UARTGetNextByte(uart);
            packet.status = ProtocolValidatePacket(&packet, validation);
        }
    }
    return packet;
}

/**
 * ProtocolSendPacket()
 *     Description:
 *         Generated a packet and send if over UART
 *     Params:
 *         UART_t *uart - The UART struct to use for communication
 *         unsigned char command - The protocol command
 *         unsigned char *data - The data to send in the packet. Must be at least
 *             one byte per protocol
 *         uint8_t dataSize - The number of bytes in the data pointer
 *     Returns:
 *         void
 */
void ProtocolSendPacket(
    UART_t *uart, 
    unsigned char command, 
    unsigned char *data,
    uint8_t dataSize
) {
    uint8_t nullData = 0;
    if (dataSize == 0) {
        dataSize = 1;
        nullData = 1;
    }
    uint8_t crc = 0;
    uint8_t length = dataSize + PROTOCOL_DATA_INDEX_BEGIN;
    unsigned char packet[length];

    crc = crc ^ (uint8_t) command;
    crc = crc ^ (uint8_t) length;
    packet[0] = command;
    packet[1] = length;
    if (nullData == 1) {
        // The protocol requires at least one data byte, so throw in a NULL
        packet[2] = 0x00;
    } else {
        uint8_t i;
        for (i = 2; i < length; i++) {
            packet[i] = data[i - 2];
            crc = crc ^ (uint8_t) data[i - 2];
        }
    }
    packet[length - 1] = (unsigned char) crc;
    UARTSendData(uart, packet, length);
}

/**
 * ProtocolSendStringPacket()
 *     Description:
 *         Send a given string in a packet with the given command.
 *     Params:
 *         UART_t *uart - The UART struct to use for communication
 *         unsigned char command - The protocol command
 *         char *string - The string to send
 *     Returns:
 *         void
 */
void ProtocolSendStringPacket(UART_t *uart, unsigned char command, char *string)
{
    uint8_t len = 0;
    while (string[len] != 0) {
        len++;
    }
    len++;
    ProtocolSendPacket(uart, command, (unsigned char *) string, len);
}

/**
 * ProtocolValidatePacket()
 *     Description:
 *         Use the given checksum to validate the data in a given packet
 *     Params:
 *         ProtocolPacket_t *packet - The data packet structure
 *         unsigned char validation - the XOR checksum of the data in packet
 *     Returns:
 *         uint8_t - If the packet is valid or not
 */
uint8_t ProtocolValidatePacket(ProtocolPacket_t *packet, unsigned char validation)
{
    uint8_t chk = packet->command;
    chk = chk ^ (packet->dataSize + PROTOCOL_CONTROL_PACKET_SIZE);
    uint8_t msgSize = packet->dataSize;
    uint8_t idx;
    for (idx = 0; idx < msgSize; idx++) {
        chk = chk ^ packet->data[idx];
    }
    chk = chk ^ validation;
    if (chk == 0) {
        return PROTOCOL_PACKET_STATUS_OK;
    } else {
        return PROTOCOL_PACKET_STATUS_BAD;
    }
}
