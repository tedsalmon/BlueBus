/*
 * File: upgrade.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement Upgrade Tasks
 */
#include "upgrade.h"

/**
 * UpgradeProcess()
 *     Description:
 *         Run through the applicable upgrades
 *     Params:
 *         BC127_t *bt - The bt object
 *         IBus_t *ibus - The ibus object
 *     Returns:
 *         uint8_t - Only returns to stop processing when versions match
 */
uint8_t UpgradeProcess(BC127_t *bt, IBus_t *ibus)
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
        BC127CommandSetAudio(bt, 0, 1);
        BC127CommandSetAudioDigital(
            bt,
            BC127_AUDIO_SPDIF,
            "44100",
            "0",
            "0"
        );
        BC127CommandSetBtState(bt, 2, 2);
        BC127CommandSetCodec(bt, 1, "OFF");
        BC127CommandSetMetadata(bt, 1);
        BC127CommandSetModuleName(bt, "BlueBus");
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
        ConfigSetSetting(CONFIG_SETTING_DAC_VOL, 0x44);
        PCM51XXSetVolume(0x44);
        ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_ON);
        ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_ON);
        // Set the Mic Gain to -17.5dB by default
        ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x03);
        LogRaw("Device Provisioned\r\n");
    }
    // Changes in version 1.1.1
    if (UpgradeVersionCompare(curMajor, curMinor, curPatch, 1, 1, 1) == 1) {
        BC127CommandSetAudioAnalog(bt, 3, 15, 1, "OFF");
        BC127CommandSetProfiles(bt, 1, 1, 1, 1);
        BC127SendCommand(bt, "SET HFP_CONFIG=OFF ON ON OFF ON OFF");
        BC127CommandWrite(bt);
        // Set the Mic Gain to -17.5dB by default
        ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x03);
        LogRaw("Ran Upgrade 1.1.1\r\n");
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

