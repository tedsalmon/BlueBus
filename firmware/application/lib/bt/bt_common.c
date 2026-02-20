/*
 * File:   bt_common.c
 * Author: tsalmon
 *
 * Created on December 1, 2021, 9:12 PM
 */
#include "bt_common.h"
#include <stdio.h>


/**
 * BTClearActiveDevice()
 *     Description:
 *        Clear the active device
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BTClearActiveDevice(BT_t *bt)
{
    bt->activeDevice = BTConnectionInit();
}

/**
 * BTClearMetadata()
 *     Description:
 *        (Re)Initialize the metadata fields to blank
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BTClearMetadata(BT_t *bt)
{
    memset(bt->title, 0, BT_METADATA_FIELD_SIZE);
    memset(bt->artist, 0, BT_METADATA_FIELD_SIZE);
    memset(bt->album, 0, BT_METADATA_FIELD_SIZE);
}

/**
 * BTClearPairedDevices()
 *     Description:
 *        Clear the paired devices list
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BTClearPairedDevices(BT_t *bt)
{
    uint8_t idx;
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
        memset(&bt->pairedDevices[idx], 0, sizeof(bt->pairedDevices[idx]));
    }
    bt->pairedDevicesCount = 0;
}

/**
 * BTPairedDevicesFind()
 *     Description:
 *        Clear the paired devices list
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
uint8_t BTPairedDevicesFind(BT_t *bt, uint8_t *macId)
{
    uint8_t idx;
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
        if (memcmp(macId, &bt->pairedDevices[idx].macId, BT_DEVICE_MAC_ID_LEN) == 0) {
            return idx;
        }
    }
    return 0;
}

/**
 * BTConnectionInit()
 *     Description:
 *         Returns a fresh BTConnection_t object to the caller
 *     Params:
 *         None
 *     Returns:
 *         BTConnection_t
 */
BTConnection_t BTConnectionInit()
{
    BTConnection_t conn;
    memset(&conn, 0, sizeof(BTConnection_t));
    conn.status = BT_DEVICE_STATUS_DISCONNECTED;
    // Assume the volume is halfway
    conn.a2dpVolume = 64;
    return conn;
}


/**
 * BTPairedDeviceClearRecords()
 *     Description:
 *         Clear all paired device data from EEPROM
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void BTPairedDeviceClearRecords(void)
{
    uint16_t currentAddr = CONFIG_BT_DEVICE_EEPROM_BASE;
    uint16_t endAddr = currentAddr + (BT_DEVICE_RECORD_LEN * BT_MAX_PAIRINGS);
    while (currentAddr < endAddr) {
        EEPROMWriteByte(currentAddr, 0x00);
        currentAddr++;
    }
    LogDebug(LOG_SOURCE_BT, "BT: Cleared Pairings from EEPROM");
}

/**
 * BTPairedDeviceInit()
 *     Description:
 *         Initialize a pairing profile if one does not exist
 *     Params:
 *         BT_t *bt
 *         char *macId
 *         uint8_t deviceNumber - The PDL index
 *     Returns:
 *         uint8_t The Pairing index
 */
void BTPairedDeviceInit(
    BT_t *bt,
    uint8_t *macId,
    uint8_t deviceNumber
) {
    uint8_t deviceExists = 0;
    uint8_t idx;
    for (idx = 0; idx < bt->pairedDevicesCount; idx++) {
        BTPairedDevice_t *btDevice = &bt->pairedDevices[idx];
        if (memcmp(macId, btDevice->macId, BT_DEVICE_MAC_ID_LEN) == 0) {
            deviceExists = 1;
        }
    }
    // Create a connection for this device since one does not exist
    if (deviceExists == 0) {
        BTPairedDevice_t pairedDevice;
        memcpy(pairedDevice.macId, macId, BT_DEVICE_MAC_ID_LEN);
        memset(pairedDevice.deviceName, 0, BT_DEVICE_NAME_LEN);
        if (deviceNumber <= BT_MAX_PAIRINGS) {
            pairedDevice.number = deviceNumber;
        }
        BTPairedDeviceLoadRecord(&pairedDevice, bt->pairedDevicesCount);
        bt->pairedDevices[bt->pairedDevicesCount] = pairedDevice;
        bt->pairedDevicesCount++;
        LogDebug(
            LOG_SOURCE_BT,
            "BT: Pairing[%d]: %02X%02X%02X%02X%02X%02X",
            deviceNumber,
            macId[0], macId[1], macId[2], macId[3], macId[4], macId[5]
        );
    }
    EventTriggerCallback(BT_EVENT_DEVICE_FOUND, (uint8_t *) macId);
}

/**
 * BTPairedDeviceLoadRecord()
 *     Description:
 *
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BTPairedDeviceLoadRecord(BTPairedDevice_t *btDevice, uint8_t devIdx)
{
    uint32_t baseAddr = CONFIG_BT_DEVICE_EEPROM_BASE + (devIdx * BT_DEVICE_RECORD_LEN);
    uint8_t storedMacId[BT_DEVICE_MAC_ID_LEN] = {0};
    uint8_t i;
    for (i = 0; i < BT_DEVICE_MAC_ID_LEN; i++) {
        storedMacId[i] = EEPROMReadByte(baseAddr + i);
    }
    if (memcmp(storedMacId, btDevice->macId, BT_DEVICE_MAC_ID_LEN) == 0) {
        // Read device name from EEPROM
        char storedName[BT_DEVICE_NAME_LEN] = {0};
        for (i = 0; i < BT_DEVICE_NAME_LEN - 1; i++) {
            uint8_t data = EEPROMReadByte(baseAddr + BT_DEVICE_MAC_ID_LEN + i);
            if (data == 0xFF) {
                data = 0x00;
            }
            storedName[i] = data;
            if (storedName[i] == '\0') {
                break;
            }
        }
        storedName[BT_DEVICE_NAME_LEN - 1] = '\0';
        if (storedName[0] != 0x00) {
            UtilsStrncpy(btDevice->deviceName, storedName, BT_DEVICE_NAME_LEN);
        } else {
            memset(btDevice->deviceName, 0x00, BT_DEVICE_NAME_LEN);
            snprintf(
                btDevice->deviceName,
                13,
                "%02X%02X%02X%02X%02X%02X",
                storedMacId[0],
                storedMacId[1],
                storedMacId[2],
                storedMacId[3],
                storedMacId[4],
                storedMacId[5]
            );
        }
    } else {
        LogWarning(
            "BT: Device Mismatch[%d]: %02X%02X%02X%02X%02X%02X",
            devIdx,
            storedMacId[0],
            storedMacId[1],
            storedMacId[2],
            storedMacId[3],
            storedMacId[4],
            storedMacId[5]
        );
        // Change Pairing Record
        BTPairedDeviceSave(btDevice->macId, "", devIdx);
    }
}

/**
 * BTPairedDeviceSave()
 *     Description:
 *         Save a paired device MAC ID and name to EEPROM
 *     Params:
 *         uint8_t *macId - The MAC ID of the device (6 bytes)
 *         char *deviceName - The friendly name of the device
 *         uint8_t devIdx - The index of the device (0 for the first pairing)
 *     Returns:
 *         void
 */
void BTPairedDeviceSave(uint8_t *macId, char *deviceName, uint8_t devIdx)
{
    if (devIdx >= BT_MAX_PAIRINGS) {
        LogDebug(LOG_SOURCE_BT, "BT: Invalid device number %d", devIdx);
        return;
    }
    uint32_t baseAddr = CONFIG_BT_DEVICE_EEPROM_BASE + (devIdx * BT_DEVICE_RECORD_LEN);
    uint8_t subAddr = 0;
    while (subAddr < BT_DEVICE_RECORD_LEN) {
        EEPROMWriteByte(baseAddr + subAddr, 0x00);
        subAddr++;
    }
    uint8_t i = 0;
    for (i = 0; i < BT_DEVICE_MAC_ID_LEN; i++) {
        EEPROMWriteByte(baseAddr + i, macId[i]);
    }
    for (i = 0; i < BT_DEVICE_NAME_LEN; i++) {
        if (deviceName[i] == '\0') {
            break;
        }
        EEPROMWriteByte(baseAddr + BT_DEVICE_MAC_ID_LEN + i, deviceName[i]);
    }
    EEPROMWriteByte(baseAddr + BT_DEVICE_MAC_ID_LEN + BT_DEVICE_NAME_LEN - 1, '\0');
    LogDebug(LOG_SOURCE_BT, "BT: Saved[%d]: %s", devIdx, deviceName);
}

/**
 * BTPBAPBCDToPhoneNumber()
 *     Description:
 *         Convert a BCD-encoded phone number to ASCII string
 *     Params:
 *         const uint8_t *bcd - The BCD encoded number (7 bytes)
 *         char *ascii - Output buffer for ASCII string
 *         uint8_t maxLen - Maximum output length (including null terminator)
 *     Returns:
 *         void
 */
void BTPBAPBCDToPhoneNumber(const uint8_t *bcd, char *ascii, uint8_t maxLen)
{
    uint8_t i;
    uint8_t pos = 0;
    for (i = 0; i < 7 && pos < maxLen - 1; i++) {
        uint8_t highNibble = (bcd[i] >> 4) & 0x0F;
        uint8_t lowNibble = bcd[i] & 0x0F;
        // Process high nibble first (most significant)
        if (highNibble != BT_PBAP_BCD_UNUSED) {
            if (highNibble <= 9) {
                ascii[pos++] = '0' + highNibble;
            } else if (highNibble == BT_PBAP_BCD_STAR) {
                ascii[pos++] = '*';
            } else if (highNibble == BT_PBAP_BCD_HASH) {
                ascii[pos++] = '#';
            } else if (highNibble == BT_PBAP_BCD_PLUS) {
                ascii[pos++] = '+';
            }
        }
        // Process low nibble
        if (lowNibble != BT_PBAP_BCD_UNUSED && pos < maxLen - 1) {
            if (lowNibble <= 9) {
                ascii[pos++] = '0' + lowNibble;
            } else if (lowNibble == BT_PBAP_BCD_STAR) {
                ascii[pos++] = '*';
            } else if (lowNibble == BT_PBAP_BCD_HASH) {
                ascii[pos++] = '#';
            } else if (lowNibble == BT_PBAP_BCD_PLUS) {
                ascii[pos++] = '+';
            }
        }
    }
    ascii[pos] = '\0';
}

/**
 * BTPBAPPhoneNumberToBCD()
 *     Description:
 *         Convert an ASCII phone number to BCD encoding
 *     Params:
 *         const char *ascii - The ASCII phone number string
 *         uint8_t *bcd - Output buffer for BCD (7 bytes, cleared first)
 *     Returns:
 *         uint8_t - Number of digits encoded
 */
uint8_t BTPBAPPhoneNumberToBCD(const char *ascii, uint8_t *bcd)
{
    uint8_t i;
    uint8_t digitCount = 0;
    uint8_t bytePos = 0;
    uint8_t nibbleHigh = 1;
    // Initialize BCD buffer with unused markers
    for (i = 0; i < 7; i++) {
        bcd[i] = 0xFF;
    }
    for (i = 0; ascii[i] != '\0' && digitCount < 14; i++) {
        uint8_t nibble = BT_PBAP_BCD_UNUSED;
        char c = ascii[i];
        if (c >= '0' && c <= '9') {
            nibble = c - '0';
        } else if (c == '*') {
            nibble = BT_PBAP_BCD_STAR;
        } else if (c == '#') {
            nibble = BT_PBAP_BCD_HASH;
        } else if (c == '+') {
            nibble = BT_PBAP_BCD_PLUS;
        } else {
            // Skip non-digit characters (spaces, dashes, etc.)
            continue;
        }
        if (nibbleHigh) {
            bcd[bytePos] = (nibble << 4) | BT_PBAP_BCD_UNUSED;
            nibbleHigh = 0;
        } else {
            bcd[bytePos] = (bcd[bytePos] & 0xF0) | nibble;
            nibbleHigh = 1;
            bytePos++;
        }
        digitCount++;
    }
    return digitCount;
}

/**
 * BTParseVCard()
 *     Description:
 *         Process a single line from vCard data
 *     Params:
 *         BT_t *bt - A pointer to the module object
 *     Returns:
 *         void
 */
void BTParseVCard(BT_t *bt)
{
    char *line = bt->pbap.parser.buffer;
    uint8_t len = bt->pbap.parser.bufferIdx;
    if (memcmp(line, "BEGIN:VCARD", 11) == 0) {
        bt->pbap.parser.inVCard = 1;
        bt->pbap.contactIdx = bt->pbap.contactCount;
        if (bt->pbap.contactIdx < BT_PBAP_MAX_CONTACTS) {
            memset(&bt->pbap.contacts[bt->pbap.contactIdx], 0, sizeof(BTPBAPContact_t));
        }
        return;
    }
    if (memcmp(line, "END:VCARD", 8) == 0) {
        if (
            bt->pbap.parser.inVCard &&
            bt->pbap.contactIdx < BT_PBAP_MAX_CONTACTS
        ) {
            bt->pbap.contactCount++;
        }
        bt->pbap.parser.inVCard = 0;
        return;
    }
    // Exit if we are not within a vCard
    if (!bt->pbap.parser.inVCard) {
        return;
    }
    if (bt->pbap.contactIdx >= BT_PBAP_MAX_CONTACTS) {
        return;
    }
    if (memcmp(line, "FN:", 3) == 0) {
        uint8_t nameLen = len - 3;
        if (nameLen >= BT_PBAP_CONTACT_NAME_LEN) {
            nameLen = BT_PBAP_CONTACT_NAME_LEN - 1;
        }
        BTPBAPContact_t *contact = &bt->pbap.contacts[bt->pbap.contactIdx];
        memcpy(contact->name, line + 3, nameLen);
        contact->name[nameLen] = '\0';
        return;
    }
    if (
        memcmp(line, "TEL:", 4) == 0 ||
        memcmp(line, "TEL;", 4) == 0
    ) {
        BTPBAPContact_t *contact = &bt->pbap.contacts[bt->pbap.contactIdx];
        BTPBAPContactTelephone_t *tel = &contact->numbers[contact->numberCount];
        // Parse type
        uint8_t telType = BT_PBAP_TEL_TYPE_UNKNOWN;
        uint8_t numberStart = (uint8_t) UtilsCharIndex(line, ':');
        if (numberStart == 0) {
            return;
        }
        // We will reuse this counter
        uint8_t i;
        // Check for type parameter
        if (line[3] == ';' && numberStart > 3) {
            int8_t typeDeclarationEnd = UtilsCharIndex(line, '=');
            // 4 is right after the ';'
            uint8_t typeStrIdx = 4;
            if (typeDeclarationEnd > 0) {
                typeStrIdx = typeDeclarationEnd + 1;
            }
            uint8_t typeLen = numberStart - typeStrIdx;
            char telTypeStr[typeLen + 1];
            memset(telTypeStr, 0, typeLen + 1);
            for (i = 0; i < typeLen; i++) {
                telTypeStr[i] = line[i + typeStrIdx];
            }
            if (
                UtilsStricmp(telTypeStr, "CELL") == 0 ||
                UtilsStricmp(telTypeStr, "MOBILE") == 0
            ) {
                telType = BT_PBAP_TEL_TYPE_CELL;
            } else if (UtilsStricmp(telTypeStr, "HOME") == 0) {
                telType = BT_PBAP_TEL_TYPE_HOME;
            } else if (UtilsStricmp(telTypeStr, "WORK") == 0) {
                telType = BT_PBAP_TEL_TYPE_WORK;
            }
        }
        numberStart++;
        uint8_t numberLen = len - numberStart;
        char number[numberLen];
        memset(&number, 0, numberLen);
        for (i = 0; i < numberLen; i++) {
            number[i] = line[i + numberStart];
        }
        tel->type = telType;
        BTPBAPPhoneNumberToBCD(number, tel->number);
        contact->numberCount++;
    }
}

