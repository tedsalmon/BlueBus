/*
 * File: upgrade.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement Upgrade Tasks
 */
#include "upgrade.h"
#include "lib/config.h"

/**
 * UpgradeProcess()
 *     Description:
 *         Run through the applicable upgrades
 *     Params:
 *         BT_t *bt - The bt object
 *         IBus_t *ibus - The ibus object
 *     Returns:
 *         uint8_t - Only returns to stop processing when versions match
 */
uint8_t UpgradeProcess(BT_t *bt, IBus_t *ibus)
{
    unsigned char curMajor = ConfigGetFirmwareVersionMajor();
    unsigned char curMinor = ConfigGetFirmwareVersionMinor();
    unsigned char curPatch = ConfigGetFirmwareVersionPatch();
    if (FIRMWARE_VERSION_MAJOR == curMajor &&
        FIRMWARE_VERSION_MINOR == curMinor &&
        FIRMWARE_VERSION_PATCH == curPatch
    ) {
        return 0;
    }
    // Initial settings burn
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 0, 0, 1) == 1) {
        // Reset the UI
        ConfigSetUIMode(0x00);
        ConfigSetNavType(0x00);
        // Reset the VIN
        unsigned char vin[] = {0x00, 0x00, 0x00, 0x00, 0x00};
        ConfigSetVehicleIdentity(vin);
        // Reset all settings
        uint8_t idx = CONFIG_SETTING_START_ADDRESS;
        while (idx <= 0x50) {
            ConfigSetSetting(idx, 0x00);
            idx++;
        }
        // Settings
        // -10dB Gain for the DAC
        ConfigSetSetting(CONFIG_SETTING_DAC_AUDIO_VOL, 0x44);
        PCM51XXSetVolume(0x44);
        ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_ON);
        ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_ON);
        // Set the Mic Gain to -17.5dB by default
        ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x03);
        LogRaw("Device Provisioned\r\n");
    }
    // Changes in version 1.1.1
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 1) == 1) {
        // Set the Mic Gain to -23dB by default
        ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x01);
        LogRaw("Ran Upgrade 1.1.1\r\n");
    }
    // Changes in version 1.1.8
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 8) == 1) {
        // -10dB Gain for the DAC in Telephone Mode
        ConfigSetSetting(CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL, 0x44);
        LogRaw("Ran Upgrade 1.1.8\r\n");
    }
    // Changes in version 1.1.9
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 9) == 1) {
        // Max out the A2DP and HFP volumes by default
        BC127CommandSetBtVolConfig(bt, 15, 100, 10, 1);
        // Set a starting value for the telephony volume value
        ConfigSetSetting(CONFIG_SETTING_TEL_VOL, 0x00);
        LogRaw("Ran Upgrade 1.1.9\r\n");
    }
    // Changes in version 1.1.10
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 10) == 1) {
        // Migrate settings to new addresses by shifting every value up by one
        unsigned char startAddress = 0x20;
        while (startAddress > 0x1C) {
            ConfigSetSetting(startAddress, ConfigGetSetting(startAddress - 1));
            startAddress--;
        }
        // Set new `0x1C` to OFF
        ConfigSetSetting(CONFIG_SETTING_IGN_ALWAYS_ON, CONFIG_SETTING_OFF);
        LogRaw("Ran Upgrade 1.1.10\r\n");
    }
    // Changes in version 1.1.15
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 15) == 1) {
        // Enable cVc by default AGAIN to fix the units where
        // "restore" wiped out cVc
        BC127SendCommand(bt, "SET HFP_CONFIG=ON ON ON ON ON OFF");
    }
    // Changes in version 1.1.17
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 17) == 1) {
        // Set new `0x21` to English language by default
        ConfigSetSetting(CONFIG_SETTING_LANGUAGE, CONFIG_SETTING_LANGUAGE_ENGLISH);
        // Set BC127 failure counters
        ConfigSetBC127BootFailures(0);
        ConfigSetSetting(CONFIG_SETTING_AUTO_POWEROFF, CONFIG_SETTING_ON);
        ConfigSetSetting(CONFIG_SETTING_IGN_ALWAYS_ON, ConfigGetSetting(0x1C));
        // Reset to logging to all off
        ConfigSetSetting(CONFIG_SETTING_LOG, 0x01);
        LogRaw("Ran Upgrade 1.1.17\r\n");
    }
    // Changes in version 1.1.18
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 18) == 1) {
        ConfigSetSetting(CONFIG_SETTING_MANAGE_VOLUME, CONFIG_SETTING_ON);
        ConfigSetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV, CONFIG_SETTING_ON);
        BC127CommandSetCOD(bt, 300420);
        LogRaw("Ran Upgrade 1.1.18\r\n");
    }
    // Changes in version 1.2.0
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 2, 0) == 1) {
        if (bt->type == BT_BTM_TYPE_BM83) {
            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x09);
        }
        LogRaw("Ran Upgrade 1.2.0\r\n");
    }
    // Changes in version 1.2.1
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 2, 1) == 1) {
        if (bt->type == BT_BTM_TYPE_BM83) {
            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x00);
            ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE, 0x00);
        }
        LogRaw("Ran Upgrade 1.2.1\r\n");
    }
    // Changes in version 1.3.2
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 3, 2) == 1) {
        ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE_MAC,0x00);
        LogRaw("Ran Upgrade 1.3.2\r\n");
    }
    // Changes in version 1.3.5
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 3, 5) == 1) {
        ConfigSetSetting(CONFIG_SETTING_LM_IO_POLL_ENABLED, CONFIG_SETTING_ON);
        LogRaw("Ran Upgrade 1.3.5\r\n");
    }
    // Changes in version 1.4.0
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 4, 0) == 1) {
        ConfigSetSetting(CONFIG_SETTING_COMFORT_AUTOZOOM, CONFIG_SETTING_OFF);
        ConfigSetSetting(CONFIG_SETTING_COMFORT_PDC, CONFIG_SETTING_OFF);
        LogRaw("Ran Upgrade 1.4.0\r\n");
    }
    ConfigSetFirmwareVersion(
        FIRMWARE_VERSION_MAJOR,
        FIRMWARE_VERSION_MINOR,
        FIRMWARE_VERSION_PATCH
    );
    return 1;
}

/**
 * UpgradeVersionCompare()
 *     Description:
 *         Returns a boolean result indicating if the new firmware version
 *         is newer than the currently installed version
 *     Params:
 *         unsigned char curMajor - The current major version installed
 *         unsigned char curMinor - The current minor version installed
 *         unsigned char curPatch - The current patch version installed
 *         unsigned char newMajor - The new major version
 *         unsigned char newMinor - The new minor version
 *         unsigned char newPatch - The new patch version
 *     Returns:
 *         uint8_t
 */
uint8_t UpgradeVersionCompare(
    unsigned char curMajor,
    unsigned char curMinor,
    unsigned char curPatch,
    unsigned char newMajor,
    unsigned char newMinor,
    unsigned char newPatch
) {
    uint8_t minorIsNewer = UPGRADE_MINOR_IS_NEWER;
    // If the major version is newer, nothing else matters
    if (curMajor < newMajor) {
        return 1;
    }
    if (curMinor == newMinor) {
        minorIsNewer = UPGRADE_MINOR_IS_SAME;
    }
    if (curMinor < newMinor) {
        minorIsNewer = UPGRADE_MINOR_IS_OLDER;
    }
    if (curPatch < newPatch && minorIsNewer == UPGRADE_MINOR_IS_SAME) {
        return 1;
    }
    if (minorIsNewer == UPGRADE_MINOR_IS_OLDER) {
        return 1;
    }
    return 0;
}

