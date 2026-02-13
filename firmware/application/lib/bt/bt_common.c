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
