/*
 * File: protocol.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a simple message protocol for the bootloader
 */
#include "protocol.h"

void ProtocolFlashWrite(UART_t *uart, ProtocolPacket_t *packet)
{
    uint32_t address = (
        ((uint64_t)0x00 << 32) |
        ((uint32_t)packet->data[0] << 24) |
        ((uint32_t)packet->data[0] << 16) |
        packet->data[1]
    );
    if (address > BOOTLOADER_APPLICATION_START) {
        uint8_t dataLength = packet->dataSize - 7;
        uint8_t index = 2;
        while (index < dataLength) {
            uint32_t data = (
                ((uint64_t)0x00 << 32) | // "Phantom" Byte
                ((uint32_t)packet->data[index] << 24) |
                ((uint32_t)packet->data[index + 1] << 16) |
                packet->data[index + 2]
            );
            uint8_t flashRes = FlashWriteAddress(address, data);
            if (!flashRes) {
                // Abort the write
                ProtocolSendPacket(uart, PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR, 0x00, 1);
                index = dataLength + 1;
            } else {
                address = address + 2;
            }
        }
        ProtocolSendPacket(uart, PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK, 0x00, 1);
    } else {
        ProtocolSendPacket(uart, PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR, 0x00, 1);
    }
}

ProtocolPacket_t ProtocolProcessPacket(UART_t *uart)
{
    struct ProtocolPacket_t packet;
    packet.status = PROTOCOL_PACKET_STATUS_INCOMPLETE;
    if (uart->rxQueueSize >= 2) {
        if (uart->rxQueueSize == (uint8_t) UARTGetOffsetByte(uart, 1)) {
            packet.command = UARTGetNextByte(uart);
            packet.dataSize = (uint8_t) UARTGetNextByte(uart);
            uint8_t i;
            for (i = 0; i < packet.dataSize - 1; i++) {
                if (uart->rxQueueSize > 0) {
                    packet.data[i] = UARTGetNextByte(uart);
                } else {
                    packet.data[i] = 0x00;
                }
            }
            packet.status = ProtocolValidatePacket(&packet);
        }
    }
    return packet;
}

void ProtocolSendPacket(
    UART_t *uart, 
        unsigned char command, 
        unsigned char *data, 
        uint8_t dataSize
) {
    uint8_t crc = 0;
    uint8_t length = dataSize + 3;
    unsigned char packet[length];

    crc = crc ^ (uint8_t) command;
    crc = crc ^ (uint8_t) length;
    packet[0] = command;
    packet[1] = length;
    uint8_t i;
    for (i = 2; i < length; i++) {
        packet[i] = data[i - 2];
        crc = crc ^ (uint8_t) data[i - 2];
    }
    packet[length - 1] = (unsigned char) crc;
    UARTSendData(uart, packet, length);
}

void ProtocolSendStringPacket(UART_t *uart, unsigned char command, char *string)
{
    uint8_t len = 0;
    while (string[len] != 0) {
        len++;
    }
    len++;
    ProtocolSendPacket(uart, command, (unsigned char *) string, len);
}

uint8_t ProtocolValidatePacket(ProtocolPacket_t *packet)
{
    uint8_t chk = 0;
    chk = chk ^ packet->command;
    chk = chk ^ packet->dataSize;
    uint8_t msgSize = packet->dataSize - 2;
    uint8_t idx;
    for (idx = 0; idx < msgSize; idx++) {
        chk = chk ^ packet->data[idx];
    }
    if (chk == 0) {
        return PROTOCOL_PACKET_STATUS_OK;
    } else {
        return PROTOCOL_PACKET_STATUS_BAD;
    }
}
