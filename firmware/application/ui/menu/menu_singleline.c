/*
 * File: menu_singleline.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#include "menu_singleline.h"

static uint8_t SETTINGS_MENU[] = {
    MENU_SINGLELINE_SETTING_IDX_METADATA_MODE,
    MENU_SINGLELINE_SETTING_IDX_AUTOPLAY,
    MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP,
    MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV,
    MENU_SINGLELINE_SETTING_IDX_TEL_HFP,
    MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN,
    MENU_SINGLELINE_SETTING_IDX_TEL_VOL_OFFSET,
    MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE,
    MENU_SINGLELINE_SETTING_IDX_BLINKERS,
    MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS,
    MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS,
    MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK,
    MENU_SINGLELINE_SETTING_IDX_ABOUT,
    MENU_SINGLELINE_SETTING_IDX_PAIRINGS
};

static uint8_t SETTINGS_TO_CONFIG_MAP[] = {
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY,
    CONFIG_SETTING_DSP_INPUT_SRC,
    CONFIG_SETTING_VOLUME_LOWER_ON_REV,
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_MIC_GAIN,
    CONFIG_SETTING_TEL_VOL,
    CONFIG_SETTING_TEL_MODE,
    CONFIG_SETTING_COMFORT_BLINKERS,
    CONFIG_SETTING_COMFORT_PARKING_LAMPS
};

/**
 * MenuSingleLineInit()
 *     Description:
 *         Initialize the struct for the single line menu driver
 *     Params:
 *         IBus_t *ibus - Pointer to the IBus_t struct
 *         BT_t *bt - Pointer to the BT_t struct
 *         void *uiUpdateFunc - Void pointer to the UI update handler in the UI handler
 *         void *uiContext - Void pointer to the context of the UI handler
 *     Returns:
 *         MenuSingleLineContext_t
 */
MenuSingleLineContext_t MenuSingleLineInit(
    IBus_t *ibus,
    BT_t *bt,
    void *uiUpdateFunc,
    void *uiContext
) {
    MenuSingleLineContext_t Context;
    Context.ibus = ibus;
    Context.bt = bt;
    Context.uiUpdateFunc = uiUpdateFunc;
    Context.uiContext = uiContext;
    Context.settingIdx = MENU_SINGLELINE_SETTING_IDX_METADATA_MODE; 
    Context.settingValue = 0;
    Context.settingMode = MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS;
    Context.uiMode = ConfigGetUIMode();
    return Context;
}

/**
 * MenuSingleLineMainDisplayText()
 *     Description:
 *         Call the UIs main display function with the updated text
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *         char *str - The text to display
 *         int8_t timeout - The timeout value
 *     Returns:
 *         void
 */
void MenuSingleLineSetMainDisplayText(
    MenuSingleLineContext_t *context,
    const char *str,
    int8_t timeout
) {
    context->uiUpdateFunc(
        context->uiContext,
        str,
        timeout,
        MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
    );
}

/**
 * MenuSingleLineSetTempDisplayText()
 *     Description:
 *         Call the UIs temporary display function with the updated text
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *         char *str - The text to display
 *         int8_t timeout - The timeout value
 *     Returns:
 *         void
 */
void MenuSingleLineSetTempDisplayText(
    MenuSingleLineContext_t *context,
    const char *str,
    int8_t timeout
) {
    context->uiUpdateFunc(
        context->uiContext,
        str,
        timeout,
        MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
    );
}

/**
 * MenuSingleLineSettings()
 *     Description:
 *         Initialize the settings menu
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *     Returns:
 *         void
 */
void MenuSingleLineSettings(MenuSingleLineContext_t *context)
{
    unsigned char value = ConfigGetSetting(
        CONFIG_SETTING_METADATA_MODE
    );
    if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_OFF) {
        MenuSingleLineSetMainDisplayText(context, "Metadata: Off", 0);
    } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY) {
        MenuSingleLineSetMainDisplayText(context, "Metadata: Party", 0);
    } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
        MenuSingleLineSetMainDisplayText(context, "Metadata: Chunk", 0);
    }
    context->settingIdx = MENU_SINGLELINE_SETTING_IDX_METADATA_MODE;
    context->settingValue = value;
    context->settingMode = MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS;
}

/**
 * MenuSingleLineSettingsEditSave()
 *     Description:
 *         Handle the edit / save button input
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *     Returns:
 *         void
 */
void MenuSingleLineSettingsEditSave(MenuSingleLineContext_t *context)
{
    // Ignore the Edit / Save button for the about index
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_ABOUT ||
        (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP &&
        context->ibus->moduleStatus.DSP == 0)
    ) {
        return;
    }
    if (context->settingMode == MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS) {
        context->settingMode = MENU_SINGLELINE_SETTING_MODE_SCROLL_VALUES;
        // We should redisplay the existing text here for the non-MID UIs
        if (context->uiMode != CONFIG_UI_MID) {
            MenuSingleLineSettingsNextSetting(context, context->settingIdx);
        }
    } else if (context->settingMode == MENU_SINGLELINE_SETTING_MODE_SCROLL_VALUES) {
        context->settingMode = MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS;
        // Save Setting
        if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_PAIRINGS) {
            if (context->settingValue == CONFIG_SETTING_ON) {
                if (context->bt->type == BT_BTM_TYPE_BC127) {
                    BC127CommandUnpair(context->bt);
                } else {
                    BM83CommandRestore(context->bt);
                    ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x00);
                    ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE, 0x00);
                }
                MenuSingleLineSetTempDisplayText(context, "Unpaired", 1);
            }
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS) {
            ConfigSetComfortLock(context->settingValue);
            MenuSingleLineSetTempDisplayText(context, "Saved", 1);
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK) {
            ConfigSetComfortUnlock(context->settingValue);
            MenuSingleLineSetTempDisplayText(context, "Saved", 1);
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP) {
            ConfigSetSetting(CONFIG_SETTING_DSP_INPUT_SRC, context->settingValue);
            MenuSingleLineSetTempDisplayText(context, "Saved", 1);
            if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
            } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_ANALOG) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
            }
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN) {
            unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
            if (context->bt->type == BT_BTM_TYPE_BC127) {
                unsigned char micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
                unsigned char micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
                BC127CommandSetMicGain(
                    context->bt,
                    micGain,
                    micBias,
                    micPreamp
                );
            } else {
                uint8_t steps = 0;
                uint8_t direction = 0;
                if (context->settingValue > micGain) {
                    direction = 1;
                    steps = context->settingValue - micGain;
                } else {
                    steps = micGain - context->settingValue;
                }
                
                if (direction == 0) {
                    while (steps > 0) {
                        BM83CommandMicGainDown(context->bt);
                        steps--;
                    }
                } else {
                    while (steps > 0) {
                        BM83CommandMicGainUp(context->bt);
                        steps--;
                    }
                }
            }
            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, micGain);
        } else {
            ConfigSetSetting(
                SETTINGS_TO_CONFIG_MAP[context->settingIdx],
                context->settingValue
            );
            MenuSingleLineSetTempDisplayText(context, "Saved", 1);
            if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_HFP &&
                context->bt->activeDevice.deviceId != 0
            ) {
                if (context->settingValue == 0x00) {
                    if (context->bt->type == BT_BTM_TYPE_BC127) {
                        BC127CommandClose(context->bt, context->bt->activeDevice.hfpId);
                    } else {
                        BM83CommandDisconnect(context->bt, BM83_CMD_DISCONNECT_PARAM_HF);
                    }
                } else {
                    if (context->bt->type == BT_BTM_TYPE_BC127) {
                        BC127CommandProfileOpen(context->bt, "HFP");
                    } else {
                        BTPairedDevice_t *device = 0;
                        uint8_t i = 0;
                        for (i = 0; i < BT_MAC_ID_LEN; i++) {
                            BTPairedDevice_t *tmpDev = &context->bt->pairedDevices[i];
                            if (memcmp(context->bt->activeDevice.macId, tmpDev->macId, BT_MAC_ID_LEN) == 0) {
                                device = tmpDev;
                            }
                        }
                        if (device != 0) {
                            BM83CommandConnect(
                                context->bt, 
                                device, 
                                BM83_DATA_LINK_BACK_PROFILES_HF
                            );
                        }
                    }
                }
            }
        }
        MenuSingleLineSettingsNextSetting(context, context->settingIdx);
    }
}

/**
 * MenuSingleLineSettingsScroll()
 *     Description:
 *         Scroll the display in the given direction for the current setting
 *         mode
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *         uint8_t direction - Backwards or forwards (0x01 and 0x00 respectively)
 *     Returns:
 *         void
 */
void MenuSingleLineSettingsScroll(MenuSingleLineContext_t *context, uint8_t direction)
{
    if (context->settingMode == MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS) {
        uint8_t nextOption = 0;
        if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_METADATA_MODE && direction == 0x01) {
            nextOption = SETTINGS_MENU[MENU_SINGLELINE_SETTING_IDX_PAIRINGS];
        } else if(context->settingIdx == MENU_SINGLELINE_SETTING_IDX_PAIRINGS && direction == 0x00) {
            nextOption = SETTINGS_MENU[MENU_SINGLELINE_SETTING_IDX_METADATA_MODE];
        } else {
            if (direction == 0x00) {
                nextOption = SETTINGS_MENU[context->settingIdx + 1];
            } else {
                nextOption = SETTINGS_MENU[context->settingIdx - 1];
            }
        }
        // Hide TCU Mode option on HW Version 1. It is not necessary there.
        if (nextOption == MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE &&
            context->bt->type == BT_BTM_TYPE_BC127
        ) {
            if (direction == 0x00) {
                nextOption++;
            } else {
                nextOption--;
            }
        }
        MenuSingleLineSettingsNextSetting(context, nextOption);
    } else if (context->settingMode == MENU_SINGLELINE_SETTING_MODE_SCROLL_VALUES) {
        MenuSingleLineSettingsNextValue(context, direction);
    }
}

/**
 * MenuSingleLineSettingsNextSetting()
 *     Description:
 *         Display the next setting
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *         uint8_t nextMenu - The index of the next menu to display
 *     Returns:
 *         void
 */
void MenuSingleLineSettingsNextSetting(MenuSingleLineContext_t *context, uint8_t nextMenu)
{
    context->settingIdx = nextMenu;
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_METADATA_MODE) {
        unsigned char value = ConfigGetSetting(
            CONFIG_SETTING_METADATA_MODE
        );
        if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Metadata: Off", 0);
        } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY) {
            MenuSingleLineSetMainDisplayText(context, "Metadata: Party", 0);
        } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
            MenuSingleLineSetMainDisplayText(context, "Metadata: Chunk", 0);
        }
        context->settingValue = value;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_AUTOPLAY) {
        if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == 0x00) {
            MenuSingleLineSetMainDisplayText(context, "Autoplay: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Autoplay: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MENU_SINGLELINE_SETTING_IDX_AUTOPLAY;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP) {
        if (context->ibus->moduleStatus.DSP == 0) {
            MenuSingleLineSetMainDisplayText(context, "DSP: Not Equipped", 0);
        } else {
            context->settingValue = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
            if (context->ibus->moduleStatus.DSP == 0) {
                if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
                    MenuSingleLineSetMainDisplayText(context, "DSP: Digital", 0);
                } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_ANALOG) {
                    MenuSingleLineSetMainDisplayText(context, "DSP: Analog", 0);
                } else {
                    MenuSingleLineSetMainDisplayText(context, "DSP: Default", 0);
                }
            }
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV) {
        if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Lower Vol. On Reverse: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Lower Vol. On Reverse: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_HFP) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
            MenuSingleLineSetMainDisplayText(context, "Handsfree: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Handsfree: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN) {
        unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
        context->settingValue = micGain;
        char micGainText[16] = {0};
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (micGain > 21) {
                micGain = 0;
            }
            snprintf(micGainText, 15, "Mic Gain: %idB", (int8_t) BTBC127MicGainTable[micGain]);
        } else {
            if (micGain > 0x0F) {
                micGain = 0;
            }
            snprintf(micGainText, 15, "Mic Gain: %idB", (int8_t) BTBM83MicGainTable[micGain]);
        }
        MenuSingleLineSetMainDisplayText(
            context,
            micGainText,
            0
        );
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_VOL_OFFSET) {
        uint8_t telephoneVolume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (telephoneVolume > 0xF || telephoneVolume == 0) {
            telephoneVolume = 1;
        }
        char telephoneVolumeText[21] = {0};
        snprintf(telephoneVolumeText, 21, "Call Vol. Offset: %d", telephoneVolume);
        MenuSingleLineSetMainDisplayText(context, telephoneVolumeText, 0);
        context->settingValue = telephoneVolume;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE) {
        context->settingValue = ConfigGetSetting(CONFIG_SETTING_TEL_MODE);
        if (context->settingValue == CONFIG_SETTING_TEL_MODE_DEFAULT) {
            MenuSingleLineSetMainDisplayText(context, "Call Mode: Default (Rec.)", 0);
        } else {
            MenuSingleLineSetMainDisplayText(context, "Call Mode: TCU", 0);
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_BLINKERS) {
        unsigned char blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
        if (blinkCount > 8 || blinkCount == 0) {
            blinkCount = 1;
        }
        context->settingValue = blinkCount;
        char blinkerText[19] = {0};
        snprintf(blinkerText, 19, "Comfort Blinks: %d", context->settingValue);
        MenuSingleLineSetMainDisplayText(context, blinkerText, 0);
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS) {
        if (ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS) == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Parking Lamps: Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Parking Lamps: On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS) {
        context->settingValue = ConfigGetComfortLock();
        if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            MenuSingleLineSetMainDisplayText(context, "Comfort Lock: 10km/h", 0);
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_20KM) {
            MenuSingleLineSetMainDisplayText(context, "Comfort Lock: 20km/h", 0);
        } else {
            MenuSingleLineSetMainDisplayText(context, "Comfort Lock: Off", 0);
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK) {
        context->settingValue = ConfigGetComfortUnlock();
        if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            MenuSingleLineSetMainDisplayText(context, "Comfort Unlock: Pos 1", 0);
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_0) {
            MenuSingleLineSetMainDisplayText(context, "Comfort Unlock: Pos 0", 0);
        } else {
            MenuSingleLineSetMainDisplayText(context, "Comfort Unlock: Off", 0);
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_ABOUT) {
        char firmwareVersion[6] = {0};
        ConfigGetFirmwareVersionString(firmwareVersion);
        char aboutText[37] = {0};
        snprintf(
            aboutText,
            36,
            "FW: %s Serial: %d Built: %02d/%d",
            firmwareVersion,
            ConfigGetSerialNumber(),
            ConfigGetBuildWeek(),
            ConfigGetBuildYear()
        );
        MenuSingleLineSetMainDisplayText(context, aboutText, 0);
        context->settingValue = CONFIG_SETTING_OFF;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_PAIRINGS) {
        MenuSingleLineSetMainDisplayText(context, "Clear Pairings", 0);
        context->settingValue = CONFIG_SETTING_OFF;
    }
}

/**
 * MenuSingleLineSetttingsNextValue()
 *     Description:
 *         Display the next value for the current setting
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *         uint8_t direction - The direction that we pressed the button
 *     Returns:
 *         void
 */
void MenuSingleLineSettingsNextValue(MenuSingleLineContext_t *context, uint8_t direction)
{
    // Select different configuration options
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_METADATA_MODE) {
        if (context->settingValue == MENU_SINGLELINE_SETTING_METADATA_MODE_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Party", 0);
            context->settingValue = MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY;
        } else if (context->settingValue == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY) {
            MenuSingleLineSetMainDisplayText(context, "Chunk", 0);
            context->settingValue = MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK;
        } else if (context->settingValue == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
            MenuSingleLineSetMainDisplayText(context, "Off", 0);
            context->settingValue = MENU_SINGLELINE_SETTING_METADATA_MODE_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUTOPLAY) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP &&
        context->ibus->moduleStatus.DSP == 1
    ) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Digital", 0);
            context->settingValue = CONFIG_SETTING_DSP_INPUT_SPDIF;
        } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
            MenuSingleLineSetMainDisplayText(context, "Analog", 0);
            context->settingValue = CONFIG_SETTING_DSP_INPUT_ANALOG;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Default", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV ||
        context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_HFP ||
        context->settingIdx == MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS
    ) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "On", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN) {
        if (direction == 0x01) {
            if (context->settingValue > 0) {
                context->settingValue--;
            }
        } else {
            context->settingValue++;
        }
        char micGainText[6] = {0};
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (context->settingValue > 21) {
                context->settingValue = 0;
            }
            snprintf(micGainText, 5, "%idB", (int8_t) BTBC127MicGainTable[context->settingValue]);
        } else {
            if (context->settingValue > 0x0F) {
                context->settingValue = 0;
            }
            snprintf(micGainText, 5, "%idB", (int8_t) BTBM83MicGainTable[context->settingValue]);
        }
        MenuSingleLineSetMainDisplayText(
            context,
            micGainText,
            0
        );
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_VOL_OFFSET) {
        if (direction == 0x01) {
            if (context->settingValue > 0) {
                context->settingValue--;
            }
        } else {
            context->settingValue++;
        }
        if (context->settingValue > 0xF) {
            context->settingValue = 1;
        }
        char telephoneVolumeText[3] = {0};
        snprintf(telephoneVolumeText, 3, "%d", context->settingValue);
        MenuSingleLineSetMainDisplayText(context, telephoneVolumeText, 0);
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE) {
        if (context->settingValue == CONFIG_SETTING_TEL_MODE_DEFAULT) {
            MenuSingleLineSetMainDisplayText(context, "TCU", 0);
            context->settingValue = CONFIG_SETTING_TEL_MODE_TCU;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Default (Recommended)", 0);
            context->settingValue = CONFIG_SETTING_TEL_MODE_DEFAULT;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_BLINKERS) {
        if (direction == 0x00) {
            context->settingValue++;
        } else {
            context->settingValue--;
        }
        if (context->settingValue > 8 || context->settingValue == 0) {
            context->settingValue = 1;
        }
        char blinkerText[2] = {0};
        snprintf(blinkerText, 2, "%d", context->settingValue);
        MenuSingleLineSetMainDisplayText(context, blinkerText, 0);
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "10km/h", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_10KM;
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            MenuSingleLineSetMainDisplayText(context, "20km/h", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_20KM;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Pos 1", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_1;
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            MenuSingleLineSetMainDisplayText(context, "Pos 0", 0);
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_0;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Off", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_PAIRINGS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetMainDisplayText(context, "Press Save", 0);
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MenuSingleLineSetMainDisplayText(context, "Clear Pairings", 0);
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
}