/*
 * File: protocol.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a simple data protocol for the bootloader
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define _ADDED_C_LIB 1
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../mappings.h"
#include "eeprom.h"
#include "flash.h"
#include "uart.h"
#define PROTOCOL_MAX_DATA_SIZE 255
#define PROTOCOL_CONTROL_PACKET_SIZE 3
#define PROTOCOL_DATA_INDEX_BEGIN 2
#define PROTOCOL_PACKET_STATUS_BAD 0
#define PROTOCOL_PACKET_STATUS_INCOMPLETE 1
#define PROTOCOL_PACKET_STATUS_OK 3
#define PROTOCOL_PACKET_WAIT_MS 50
/**
 * Commands that the bootloader recognizes
 */
#define PROTOCOL_CMD_PLATFORM_REQUEST 0x00
#define PROTOCOL_CMD_PLATFORM_RESPONSE 0x01
#define PROTOCOL_CMD_ERASE_FLASH_REQUEST 0x02
#define PROTOCOL_CMD_ERASE_FLASH_RESPONSE 0x03
#define PROTOCOL_CMD_WRITE_DATA_REQUEST 0x04
#define PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK 0x05
#define PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR 0x06
#define PROTOCOL_CMD_BC127_MODE_REQUEST 0x07
#define PROTOCOL_CMD_BC127_MODE_RESPONSE 0x08
#define PROTOCOL_CMD_START_APP_REQUEST 0x09
#define PROTOCOL_CMD_START_APP_RESPONSE 0x0A
#define PROTOCOL_CMD_FIRMWARE_VERSION_REQUEST 0x0B
#define PROTOCOL_CMD_FIRMWARE_VERSION_RESPONSE 0x0C
#define PROTOCOL_CMD_READ_SN_REQUEST 0x0D
#define PROTOCOL_CMD_READ_SN_RESPONSE 0x0E
#define PROTOCOL_CMD_WRITE_SN_REQUEST 0x0F
#define PROTOCOL_CMD_WRITE_SN_RESPONSE_OK 0x10
#define PROTOCOL_CMD_WRITE_SN_RESPONSE_ERR 0x11
#define PROTOCOL_CMD_READ_BUILD_DATE_REQUEST 0x12
#define PROTOCOL_CMD_READ_BUILD_DATE_RESPONSE 0x13
#define PROTOCOL_CMD_WRITE_BUILD_DATE_REQUEST 0x14
#define PROTOCOL_CMD_WRITE_BUILD_DATE_RESPONSE_OK 0x15
#define PROTOCOL_CMD_WRITE_BUILD_DATE_RESPONSE_ERR 0x16
#define PROTOCOL_ERR_PACKET_TIMEOUT 0xFE
#define PROTOCOL_BAD_PACKET_RESPONSE 0xFF

/**
 * Packet Description:
 * <Command Byte> <Length of entire message> <data (up to 128 bytes)> <XOR>
 * At least _one_ data byte is required, though it can be any value
 *
 * Integrity is verified with the XOR byte at the end of the packet
 */

/**
 * ProtocolPacket_t
 *     Description:
 *         This object defines the structure of a protocol packet
 */
typedef struct ProtocolPacket_t {
    uint8_t status;
    unsigned char command;
    uint8_t dataSize;
    unsigned char data[PROTOCOL_MAX_DATA_SIZE];
} ProtocolPacket_t;
void ProtocolBC127Mode();
void ProtocolFlashErase();
uint8_t ProtocolFlashWrite(ProtocolPacket_t *);
void ProtocolProcessMessage(UART_t *, uint8_t *);
ProtocolPacket_t ProtocolProcessPacket(UART_t *);
void ProtocolSendPacket(UART_t *, unsigned char, unsigned char *, uint8_t);
void ProtocolSendStringPacket(UART_t *, unsigned char, char *);
uint8_t ProtocolValidatePacket(ProtocolPacket_t *, unsigned char);
void ProtocolWriteSerialNumber(UART_t *, ProtocolPacket_t *);
void ProtocolWriteSerialNumber(UART_t *, ProtocolPacket_t *);
void ProtocolWriteBuildDate(UART_t *, ProtocolPacket_t *);
#endif /* PROTOCOL_H */
