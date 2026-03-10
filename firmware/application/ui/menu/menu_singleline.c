/*
 * File: menu_singleline.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the MID UI Mode handler
 */
#include "menu_singleline.h"
#include "../../lib/bt/bt_bc127.h"
#include "../../lib/bt/bt_bm83.h"
#include "../../lib/config.h"
#include "../../lib/event.h"
#include <stdio.h>
#include <string.h>

static uint8_t SETTINGS_MENU[] = {
    MENU_SINGLELINE_SETTING_IDX_METADATA_MODE,
    MENU_SINGLELINE_SETTING_IDX_AUTOPLAY,
    MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP,
    MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV,
    MENU_SINGLELINE_SETTING_IDX_AUDIO_DAC_GAIN,
    MENU_SINGLELINE_SETTING_IDX_TEL_HFP,
    MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN,
    MENU_SINGLELINE_SETTING_IDX_TEL_VOL_OFFSET,
    MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE,
    MENU_SINGLELINE_SETTING_IDX_BLINKERS,
    MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS,
    MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS,
    MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK,
    MENU_SINGLELINE_SETTING_IDX_VISUAL_PDC,
    MENU_SINGLELINE_SETTING_IDX_ABOUT,
    MENU_SINGLELINE_SETTING_IDX_PAIRINGS
};

static uint8_t SETTINGS_TO_CONFIG_MAP[] = {
    CONFIG_SETTING_METADATA_MODE,
    CONFIG_SETTING_AUTOPLAY,
    CONFIG_SETTING_DSP_INPUT_SRC,
    CONFIG_SETTING_VOLUME_LOWER_ON_REV,
    CONFIG_SETTING_DAC_AUDIO_VOL,
    CONFIG_SETTING_HFP,
    CONFIG_SETTING_MIC_GAIN,
    CONFIG_SETTING_TEL_VOL,
    CONFIG_SETTING_TEL_MODE,
    CONFIG_SETTING_COMFORT_BLINKERS,
    CONFIG_SETTING_COMFORT_PARKING_LAMPS,
    CONFIG_SETTING_COMFORT_LOCKS,
    CONFIG_SETTING_COMFORT_UNLOCK,
    CONFIG_SETTING_VISUAL_PDC
};

/**
 * MenuSingleLineInit()
 *     Description:
 *         Initialize the struct for the single line menu driver
 *     Params:
 *         IBus_t *ibus - Pointer to the IBus_t struct
 *         BT_t *bt - Pointer to the BT_t struct
 *     Returns:
 *         void
 */
void MenuSingleLineInit(
    MenuSingleLineContext_t *context,
    IBus_t *ibus,
    BT_t *bt
) {
    context->ibus = ibus;
    context->bt = bt;
    context->activeView = MENU_SINGLELINE_VIEW_METADATA;
    context->settingIdx = MENU_SINGLELINE_SETTING_IDX_METADATA_MODE;
    context->settingValue = 0;
    context->btDeviceIndex = 0;
    context->settingMode = MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS;
    context->uiMode = ConfigGetUIMode();
    context->vehicleSpeed = 0;
    // Event Registrations
    EventRegisterCallback(
        IBUS_EVENT_SENSOR_VALUE_UPDATE,
        &MenuSingleLineIBusSensorValueUpdate,
        context
    );
    EventRegisterCallback(
        IBUS_EVENT_IKE_SPEED_RPM_UPDATE,
        &MenuSingleLineIBusSpeedUpdate,
        context
    );
}

/**
 * MenuSingleLineDestroy()
 *     Description:
 *         Unregister all event handlers, scheduled tasks and clear the context
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void MenuSingleLineDestory()
{
    EventUnregisterCallback(
        IBUS_EVENT_SENSOR_VALUE_UPDATE,
        &MenuSingleLineIBusSensorValueUpdate
    );
    EventUnregisterCallback(
        IBUS_EVENT_IKE_SPEED_RPM_UPDATE,
        &MenuSingleLineIBusSpeedUpdate
    );
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
void MenuSingleLineSetDisplayText(
    MenuSingleLineContext_t *context,
    const char *str,
    int8_t timeout,
    uint8_t type
) {
    MenuSingleLineDisplayUpdate_t update;
    update.timeout = timeout;
    update.type = type;
    uint8_t textLen = strlen(str);
    if (textLen >= UTILS_DISPLAY_TEXT_SIZE) {
        textLen = UTILS_DISPLAY_TEXT_SIZE;
    } else {
        textLen += 1;
    }
    UtilsStrncpy(update.text, str, textLen);
    EventTriggerCallback(
        UI_EVENT_MAIN_DISPLAY_UPDATE,
        (uint8_t *) &update
    );
}

/**
 * MenuSingleLineIBusSensorValueUpdate()
 *     Description:
 *         Handle sensor value update events for OBC display
 *     Params:
 *         void *ctx - Pointer to the context
 *         uint8_t *type - Pointer to the sensor value type
 *     Returns:
 *         void
 */
void MenuSingleLineIBusSensorValueUpdate(void *ctx, uint8_t *type)
{
    MenuSingleLineContext_t *context = (MenuSingleLineContext_t *) ctx;
    uint8_t updateType = *type;
    if (context->activeView != MENU_SINGLELINE_VIEW_OBC) {
        return;
    }
    if (
        updateType == IBUS_SENSOR_VALUE_COOLANT_TEMP ||
        updateType == IBUS_SENSOR_VALUE_OIL_TEMP
    ) {
        MenuSingleLineOBC(context);
    }
}

/**
 * MenuSingleLineIBusSpeedUpdate()
 *     Description:
 *         Handle speed update events for OBC display
 *     Params:
 *         void *ctx - Pointer to the context
 *         uint8_t *pkt - Pointer to the IBus packet
 *     Returns:
 *         void
 */
void MenuSingleLineIBusSpeedUpdate(void *ctx, uint8_t *pkt)
{
    MenuSingleLineContext_t *context = (MenuSingleLineContext_t *) ctx;
    if (context->activeView != MENU_SINGLELINE_VIEW_OBC) {
        return;
    }
    uint16_t speed = pkt[IBUS_PKT_DB1] * 2;
    if (context->vehicleSpeed != speed) {
        context->vehicleSpeed = speed;
        MenuSingleLineOBC(context);
    }
}

/**
 * MenuSingleLineOBC()
 *     Description:
 *         Update the OBC display with current sensor values
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *     Returns:
 *         void
 */
void MenuSingleLineOBC(MenuSingleLineContext_t *context)
{
    if (context->activeView != MENU_SINGLELINE_VIEW_OBC) {
        return;
    }

    uint8_t coolant = context->ibus->coolantTemperature;
    uint8_t oil = context->ibus->oilTemperature;

    // Convert to Fahrenheit if configured
    if (ConfigGetTempUnit() == CONFIG_SETTING_TEMP_FAHRENHEIT) {
        coolant = (coolant * 9 / 5) + 32;
        if (oil != 0) {
            oil = (oil * 9 / 5) + 32;
        }
    }

    char text[25] = {0};
    if (context->uiMode == CONFIG_UI_MID) {
        // MID: 24 chars max
        if (oil != 0) {
            snprintf(text, 24, "C:%d O:%d S:%u", coolant, oil, context->vehicleSpeed);
        } else {
            snprintf(text, 24, "Coolant:%d Speed:%u", coolant, context->vehicleSpeed);
        }
    } else {
        snprintf(text, 12, "C:%d S:%u", coolant, context->vehicleSpeed);
    }
    MenuSingleLineSetDisplayText(
        context,
        text,
        0,
        MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
    );
}

/**
 * MenuSingleLineSetUIView()
 *     Description:
 *         Set the active UI view / mode
 *     Params:
 *         MenuSingleLineContext_t *context - Pointer to the context
 *         uint8_t view - The UI mode
 *     Returns:
 *         void
 */
void MenuSingleLineSetUIView(MenuSingleLineContext_t *context, uint8_t view)
{
    context->activeView = view;

    switch (context->activeView) {
        case MENU_SINGLELINE_VIEW_SETTINGS:
            MenuSingleLineSettings(context);
            break;
        case MENU_SINGLELINE_VIEW_OBC:
            MenuSingleLineOBC(context);
            break;
        case MENU_SINGLELINE_VIEW_DEVICES:
            MenuSingleLineDevices(context, MENU_SINGLELINE_DIRECTION_FORWARD);
            break;
    }
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
    uint8_t value = ConfigGetSetting(
        CONFIG_SETTING_METADATA_MODE
    );
    if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_OFF) {
        MenuSingleLineSetDisplayText(
            context,
            "Metadata: Off",
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY) {
        MenuSingleLineSetDisplayText(
            context,
            "Metadata: Party",
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
        MenuSingleLineSetDisplayText(
            context,
            "Metadata: Chunk",
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
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
                    BTPairedDeviceClearRecords();
                    ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x00);
                    ConfigSetSetting(CONFIG_SETTING_LAST_CONNECTED_DEVICE, 0x00);
                }
                MenuSingleLineSetDisplayText(
                    context,
                    "Unpaired",
                    1,
                    MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
                );
            }
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS) {
            ConfigSetComfortLock(context->settingValue);
            MenuSingleLineSetDisplayText(
                context,
                "Saved",
                1,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK) {
            ConfigSetComfortUnlock(context->settingValue);
            MenuSingleLineSetDisplayText(
                context,
                "Saved",
                1,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP) {
            ConfigSetSetting(CONFIG_SETTING_DSP_INPUT_SRC, context->settingValue);
            MenuSingleLineSetDisplayText(
                context,
                "Saved",
                1,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
            if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_SPDIF);
            } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_ANALOG) {
                IBusCommandDSPSetMode(context->ibus, IBUS_DSP_CONFIG_SET_INPUT_RADIO);
            }
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DAC_GAIN) {
            // Limit to minimum of 24 (+12dB)
            if (context->settingValue < 24) {
                context->settingValue = 24;
            }
            ConfigSetSetting(CONFIG_SETTING_DAC_AUDIO_VOL, context->settingValue);
            // Apply the volume setting to the PCM51XX DAC
            PCM51XXSetVolume(context->settingValue);
            MenuSingleLineSetDisplayText(
                context,
                "Saved",
                1,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
        } else if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN) {
            MenuSingleLineSetDisplayText(
                context,
                "Saved",
                1,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
            uint8_t micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
            if (context->bt->type == BT_BTM_TYPE_BC127) {
                uint8_t micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
                uint8_t micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
                BC127CommandSetMicGain(
                    context->bt,
                    micGain,
                    micBias,
                    micPreamp
                );
            } else {
                int8_t offset = micGain - context->settingValue;
                while (offset < 0) {
                    BM83CommandMicGainUp(context->bt);
                    offset++;
                }
                while (offset > 0) {
                    BM83CommandMicGainDown(context->bt);
                    offset--;
                }
            }
            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, context->settingValue);
        } else {
            ConfigSetSetting(
                SETTINGS_TO_CONFIG_MAP[context->settingIdx],
                context->settingValue
            );
            MenuSingleLineSetDisplayText(
                context,
                "Saved",
                1,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
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
                    BTPairedDevice_t *device = &context->bt->pairedDevices[
                        context->bt->activeDevice.deviceIndex
                    ];
                    if (context->bt->type == BT_BTM_TYPE_BC127) {
                        BC127CommandProfileOpen(context->bt, device, "HFP");
                    } else {
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
        uint8_t value = ConfigGetSetting(
            CONFIG_SETTING_METADATA_MODE
        );
        if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Metadata: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY) {
            MenuSingleLineSetDisplayText(
                context,
                "Metadata: Party",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (value == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
            MenuSingleLineSetDisplayText(
                context,
                "Metadata: Chunk",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        }
        context->settingValue = value;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_AUTOPLAY) {
        if (ConfigGetSetting(CONFIG_SETTING_AUTOPLAY) == 0x00) {
            MenuSingleLineSetDisplayText(
                context,
                "Autoplay: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Autoplay: On",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        }
        context->settingIdx = MENU_SINGLELINE_SETTING_IDX_AUTOPLAY;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP) {
        if (context->ibus->moduleStatus.DSP == 0) {
            MenuSingleLineSetDisplayText(
                context,
                "DSP: Not Equipped",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else {
            context->settingValue = ConfigGetSetting(CONFIG_SETTING_DSP_INPUT_SRC);
            if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
                MenuSingleLineSetDisplayText(
                    context,
                    "DSP: Digital",
                    0,
                    MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
                );
            } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_ANALOG) {
                MenuSingleLineSetDisplayText(
                    context,
                    "DSP: Analog",
                    0,
                    MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
                );
            } else {
                MenuSingleLineSetDisplayText(
                    context,
                    "DSP: Default",
                    0,
                    MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
                );
            }
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV) {
        if (ConfigGetSetting(CONFIG_SETTING_VOLUME_LOWER_ON_REV) == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Lower Vol. On Reverse: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Lower Vol. On Reverse: On",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_AUDIO_DAC_GAIN) {
        uint8_t currentVolume = ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL);
        // Limit to minimum of 24 (+12dB)
        if (currentVolume < 24) {
            currentVolume = 24;
        }
        context->settingValue = currentVolume;
        char volText[19] = {0};
        if (currentVolume > 0x30) {
            uint8_t gain = (currentVolume - 0x30) / 2;
            snprintf(volText, 18, "DAC Volume: -%ddB", gain);
        } else if (currentVolume == 0x30) {
            snprintf(volText, 18, "DAC Volume: 0dB");
        } else {
            uint8_t gain = (0x30 - currentVolume) / 2;
            snprintf(volText, 18, "DAC Volume: +%ddB", gain);
        }
        MenuSingleLineSetDisplayText(
            context,
            volText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_HFP) {
        if (ConfigGetSetting(CONFIG_SETTING_HFP) == 0x00) {
            MenuSingleLineSetDisplayText(
                context,
                "Handsfree: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Handsfree: On",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN) {
        uint8_t micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
        context->settingValue = micGain;
        char micGainText[17] = {0};
        if (context->bt->type == BT_BTM_TYPE_BC127) {
            if (micGain > 21) {
                micGain = 0;
            }
            snprintf(micGainText, 16, "Mic Gain: %idB", (int8_t) BTBC127MicGainTable[micGain]);
        } else {
            if (micGain > 0x0F) {
                micGain = 0;
            }
            snprintf(micGainText, 16, "Mic Gain: %idB", (int8_t) BTBM83MicGainTable[micGain]);
        }
        MenuSingleLineSetDisplayText(
            context,
            micGainText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_VOL_OFFSET) {
        int8_t telephoneVolume = ConfigGetSetting(CONFIG_SETTING_TEL_VOL);
        if (telephoneVolume > CONFIG_SETTING_TEL_VOL_OFFSET_MAX) {
            telephoneVolume = CONFIG_SETTING_TEL_VOL_OFFSET_MAX;
        }
        char telephoneVolumeText[21] = {0};
        snprintf(telephoneVolumeText, 21, "Call Vol. Offset: %+d", telephoneVolume);
        MenuSingleLineSetDisplayText(
            context,
            telephoneVolumeText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
        context->settingValue = telephoneVolume;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE) {
        context->settingValue = ConfigGetSetting(CONFIG_SETTING_TEL_MODE);
        if (context->settingValue == CONFIG_SETTING_TEL_MODE_TCU) {
            MenuSingleLineSetDisplayText(
                context,
                "Call Mode: TCU",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (context->settingValue == CONFIG_SETTING_TEL_MODE_NO_MUTE) {
            MenuSingleLineSetDisplayText(
                context,
                "Call Mode: No Mute",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (context->settingValue == CONFIG_SETTING_TEL_MODE_ANALOG) {
            MenuSingleLineSetDisplayText(
                context,
                "Call Mode: Analog",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Call Mode: Default (Rec.)",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_BLINKERS) {
        uint8_t blinkCount = ConfigGetSetting(CONFIG_SETTING_COMFORT_BLINKERS);
        if (blinkCount > 8 || blinkCount == 0) {
            blinkCount = 1;
        }
        context->settingValue = blinkCount;
        char blinkerText[19] = {0};
        snprintf(blinkerText, 19, "Comfort Blinks: %d", context->settingValue);
        MenuSingleLineSetDisplayText(
            context,
            blinkerText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS) {
        if (ConfigGetSetting(CONFIG_SETTING_COMFORT_PARKING_LAMPS) == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Parking Lamps: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Parking Lamps: On",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS) {
        context->settingValue = ConfigGetComfortLock();
        if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            MenuSingleLineSetDisplayText(
                context,
                "Comfort Lock: 10km/h",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_20KM) {
            MenuSingleLineSetDisplayText(
                context,
                "Comfort Lock: 20km/h",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Comfort Lock: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK) {
        context->settingValue = ConfigGetComfortUnlock();
        if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            MenuSingleLineSetDisplayText(
                context,
                "Comfort Unlock: Pos 1",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_0) {
            MenuSingleLineSetDisplayText(
                context,
                "Comfort Unlock: Pos 0",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Comfort Unlock: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        }
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_VISUAL_PDC) {
        context->settingValue = ConfigGetSetting(CONFIG_SETTING_VISUAL_PDC);
        if (context->settingValue == CONFIG_SETTING_PDC_CLUSTER) {
            MenuSingleLineSetDisplayText(
                context,
                "PDC: Cluster",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (context->settingValue == CONFIG_SETTING_PDC_RADIO) {
            MenuSingleLineSetDisplayText(
                context,
                "PDC: Radio",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else if (context->settingValue == CONFIG_SETTING_PDC_BOTH) {
            MenuSingleLineSetDisplayText(
                context,
                "PDC: Both",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "PDC: Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
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
        MenuSingleLineSetDisplayText(
            context,
            aboutText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
        context->settingValue = CONFIG_SETTING_OFF;
    }
    if (nextMenu == MENU_SINGLELINE_SETTING_IDX_PAIRINGS) {
        MenuSingleLineSetDisplayText(
            context,
            "Clear Pairings",
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
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
            MenuSingleLineSetDisplayText(
                context,
                "Party",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY;
        } else if (context->settingValue == MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY) {
            MenuSingleLineSetDisplayText(
                context,
                "Chunk",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK;
        } else if (context->settingValue == MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK) {
            MenuSingleLineSetDisplayText(
                context,
                "Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = MENU_SINGLELINE_SETTING_METADATA_MODE_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUTOPLAY) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "On",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP &&
        context->ibus->moduleStatus.DSP == 1
    ) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Digital",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_DSP_INPUT_SPDIF;
        } else if (context->settingValue == CONFIG_SETTING_DSP_INPUT_SPDIF) {
            MenuSingleLineSetDisplayText(
                context,
                "Analog",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_DSP_INPUT_ANALOG;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Default",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV ||
        context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_HFP ||
        context->settingIdx == MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS
    ) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "On",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
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
        MenuSingleLineSetDisplayText(
            context,
            micGainText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
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
        MenuSingleLineSetDisplayText(
            context,
            telephoneVolumeText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_AUDIO_DAC_GAIN) {
        uint8_t currentVolume = context->settingValue;
        if (direction == MENU_SINGLELINE_DIRECTION_BACK) {
            if (currentVolume >= 2) {
                currentVolume = currentVolume - 2;
            } else {
                currentVolume = 96;
            }
            // Limit to minimum of 24 (+12dB)
            if (currentVolume < 24) {
                currentVolume = 24;
            }
        } else {
            currentVolume = currentVolume + 2;
            if (currentVolume > 96) {
                currentVolume = 24;  // Wrap around to +12dB instead of 0
            }
        }
        context->settingValue = currentVolume;
        char volText[18] = {0};
        if (currentVolume > 0x30) {
            uint8_t gain = (currentVolume - 0x30) / 2;
            snprintf(volText, 17, "DAC Volume: -%ddB", gain);
        } else if (currentVolume == 0x30) {
            snprintf(volText, 17, "DAC Volume: 0dB");
        } else {
            uint8_t gain = (0x30 - currentVolume) / 2;
            snprintf(volText, 17, "DAC Volume: +%ddB", gain);
        }
        MenuSingleLineSetDisplayText(
            context,
            volText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE) {
        if (context->settingValue == CONFIG_SETTING_TEL_MODE_DEFAULT) {
            MenuSingleLineSetDisplayText(
                context,
                "TCU",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_TEL_MODE_TCU;
        } else if (context->settingValue == CONFIG_SETTING_TEL_MODE_TCU) {
            MenuSingleLineSetDisplayText(
                context,
                "No Mute",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_TEL_MODE_NO_MUTE;
        } else if (context->settingValue == CONFIG_SETTING_TEL_MODE_NO_MUTE) {
            MenuSingleLineSetDisplayText(
                context,
                "Analog",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_TEL_MODE_ANALOG;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Default (Recommended)",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
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
        MenuSingleLineSetDisplayText(
            context,
            blinkerText,
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "10km/h",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_10KM;
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_LOCK_10KM) {
            MenuSingleLineSetDisplayText(
                context,
                "20km/h",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_COMFORT_LOCK_20KM;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Pos 1",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_1;
        } else if (context->settingValue == CONFIG_SETTING_COMFORT_UNLOCK_POS_1) {
            MenuSingleLineSetDisplayText(
                context,
                "Pos 0",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_COMFORT_UNLOCK_POS_0;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_VISUAL_PDC) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Cluster",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_PDC_CLUSTER;
        } else if (context->settingValue == CONFIG_SETTING_PDC_CLUSTER) {
            MenuSingleLineSetDisplayText(
                context,
                "Radio",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_PDC_RADIO;
        } else if (context->settingValue == CONFIG_SETTING_PDC_RADIO) {
            MenuSingleLineSetDisplayText(
                context,
                "Both",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_PDC_BOTH;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Off",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
    if (context->settingIdx == MENU_SINGLELINE_SETTING_IDX_PAIRINGS) {
        if (context->settingValue == CONFIG_SETTING_OFF) {
            MenuSingleLineSetDisplayText(
                context,
                "Press Save",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_ON;
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Clear Pairings",
                0,
                MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
            );
            context->settingValue = CONFIG_SETTING_OFF;
        }
    }
}

void MenuSingleLineDevices(
    MenuSingleLineContext_t *context,
    uint8_t direction
) {
    if (context->bt->pairedDevicesCount == 0) {
        MenuSingleLineSetDisplayText(
            context,
            "No Paired Devices",
            0,
            MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
        );
        return;
    }
    if (context->bt->pairedDevicesCount == 1) {
        context->btDeviceIndex = 0;
    } else {
        if (direction == MENU_SINGLELINE_DIRECTION_FORWARD) {
            if (context->btDeviceIndex >= context->bt->pairedDevicesCount) {
                context->btDeviceIndex = 0;
            } else {
                context->btDeviceIndex++;
            }
        } else {
            if (context->btDeviceIndex == 0) {
                context->btDeviceIndex = context->bt->pairedDevicesCount - 1;
            } else {
                context->btDeviceIndex--;
            }
        }
    }
    BTPairedDevice_t *dev = &context->bt->pairedDevices[context->btDeviceIndex];
    char text[BT_DEVICE_NAME_LEN + 3] = {0};
    UtilsStrncpy(text, dev->deviceName, BT_DEVICE_NAME_LEN);
    // Add a space and asterisks to the end of the device name
    // if it's the currently selected device
    if (context->btDeviceIndex == context->bt->activeDevice.deviceIndex) {
        uint8_t startIdx = strlen(text);
        text[startIdx++] = 0x20;
        text[startIdx++] = 0x2A;
    }
    MenuSingleLineSetDisplayText(
        context,
        text,
        0,
        MENU_SINGLELINE_DISPLAY_UPDATE_MAIN
    );
}

void MenuSingleLineDevicesConnect(MenuSingleLineContext_t *context) {
    if (context->bt->pairedDevicesCount > 0) {
        // Connect to device
        if (context->btDeviceIndex != context->bt->activeDevice.deviceIndex) {
            // Trigger device selection event
            EventTriggerCallback(
                UI_EVENT_INITIATE_CONNECTION,
                (uint8_t *)&context->btDeviceIndex
            );
            MenuSingleLineSetDisplayText(
                context,
                "Connecting",
                8,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
        } else {
            MenuSingleLineSetDisplayText(
                context,
                "Connected",
                4,
                MENU_SINGLELINE_DISPLAY_UPDATE_TEMP
            );
        }
    }

}

