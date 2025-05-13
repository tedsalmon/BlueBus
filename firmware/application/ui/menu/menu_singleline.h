/*
 * File: menu_singleline.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the shared single line Settings Menu
 */
#ifndef MENU_SINGLELINE_H
#define MENU_SINGLELINE_H

#include "../../lib/bt.h"
#include "../../lib/config.h"
#include "../../lib/ibus.h"
#include "../../lib/pcm51xx.h"
#include "../../lib/utils.h"

#define MENU_SINGLELINE_SETTING_IDX_METADATA_MODE 0
#define MENU_SINGLELINE_SETTING_IDX_AUTOPLAY 1
#define MENU_SINGLELINE_SETTING_IDX_AUDIO_DSP 2
#define MENU_SINGLELINE_SETTING_IDX_LOWER_VOL_REV 3
#define MENU_SINGLELINE_SETTING_IDX_AUDIO_DAC_GAIN 4
#define MENU_SINGLELINE_SETTING_IDX_TEL_HFP 5
#define MENU_SINGLELINE_SETTING_IDX_TEL_MIC_GAIN 6
#define MENU_SINGLELINE_SETTING_IDX_TEL_VOL_OFFSET 7
#define MENU_SINGLELINE_SETTING_IDX_TEL_TCU_MODE 8
#define MENU_SINGLELINE_SETTING_IDX_BLINKERS 9
#define MENU_SINGLELINE_SETTING_IDX_PARK_LIGHTS 10
#define MENU_SINGLELINE_SETTING_IDX_COMFORT_LOCKS 11
#define MENU_SINGLELINE_SETTING_IDX_COMFORT_UNLOCK 12
#define MENU_SINGLELINE_SETTING_IDX_ABOUT 13
#define MENU_SINGLELINE_SETTING_IDX_PAIRINGS 14

#define MENU_SINGLELINE_SETTING_MODE_SCROLL_SETTINGS 1
#define MENU_SINGLELINE_SETTING_MODE_SCROLL_VALUES 2

#define MENU_SINGLELINE_SETTING_METADATA_MODE_OFF 0x00
#define MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY 0x01
#define MENU_SINGLELINE_SETTING_METADATA_MODE_CHUNK 0x02
#define MENU_SINGLELINE_SETTING_METADATA_MODE_PARTY_SINGLE 0x03
#define MENU_SINGLELINE_SETTING_METADATA_MODE_STATIC 0x04

#define MENU_SINGLELINE_DISPLAY_UPDATE_MAIN 0
#define MENU_SINGLELINE_DISPLAY_UPDATE_TEMP 1

#define MENU_SINGLELINE_DIRECTION_FORWARD 0
#define MENU_SINGLELINE_DIRECTION_BACK 1

/*
 * MenuSingleLineContext_t
 */
typedef struct MenuSingleLineContext_t {
    IBus_t *ibus;
    BT_t *bt;
    void (*uiUpdateFunc)(void *, const char *, int8_t, uint8_t);
    void *uiContext;
    uint8_t settingIdx;
    uint8_t settingValue;
    uint8_t settingMode;
    uint8_t uiMode;
} MenuSingleLineContext_t;

MenuSingleLineContext_t MenuSingleLineInit(IBus_t *, BT_t*, void *, void *);
void MenuSingleLineMainDisplayText(MenuSingleLineContext_t *, const char *, int8_t);
void MenuSingleLineSetTempDisplayText(MenuSingleLineContext_t *, const char *, int8_t);
void MenuSingleLineSettings(MenuSingleLineContext_t *);
void MenuSingleLineSettingsEditSave(MenuSingleLineContext_t *);
void MenuSingleLineSettingsScroll(MenuSingleLineContext_t *, uint8_t);
void MenuSingleLineSettingsNextSetting(MenuSingleLineContext_t *, uint8_t);
void MenuSingleLineSettingsNextValue(MenuSingleLineContext_t *, uint8_t);
#endif /* MENU_SINGLELINE_H */
