/*
 * File:   locale.h
 * Author: Aliaksei Prybytkin <alexeypribytkin@gmail.com>
 * Description:
 *     Contains functionality for string localization
 */
#ifndef LOCALE_H
#define LOCALE_H
#include <stdint.h>
#include "config.h"
#include "utils.h"

#define LOCALE_STRING_NOT_PLAYING 0
#define LOCALE_STRING_ABOUT 1
#define LOCALE_STRING_AUDIO 2
#define LOCALE_STRING_AUTOPLAY_OFF 3
#define LOCALE_STRING_AUTOPLAY_ON 4
#define LOCALE_STRING_BACK 5
#define LOCALE_STRING_BLINKERS 6
#define LOCALE_STRING_BLUETOOTH 7
#define LOCALE_STRING_BUILT 8
#define LOCALE_STRING_CALLING 9
#define LOCALE_STRING_CAR_E3X_E53 10
#define LOCALE_STRING_CAR_E46_Z4 11
#define LOCALE_STRING_CAR_UNSET 12
#define LOCALE_STRING_CLEAR_PAIRINGS 13
#define LOCALE_STRING_COMFORT 14
#define LOCALE_STRING_DASHBOARD 15
#define LOCALE_STRING_DEVICES 16
#define LOCALE_STRING_DSP_ANALOG 17
#define LOCALE_STRING_DSP_DIGITAL 18
#define LOCALE_STRING_DSP_DEFAULT 66
#define LOCALE_STRING_FW 19
#define LOCALE_STRING_HANDSFREE_OFF 20
#define LOCALE_STRING_HANDSFREE_ON 21
#define LOCALE_STRING_LOCK_10KMH 22
#define LOCALE_STRING_LOCK_20KMH 23
#define LOCALE_STRING_LOCK_OFF 24
#define LOCALE_STRING_MAIN_MENU 25
#define LOCALE_STRING_MENU_DASHBOARD 26
#define LOCALE_STRING_MENU_MAIN 27
#define LOCALE_STRING_METADATA_CHUNK 28
#define LOCALE_STRING_METADATA_OFF 29
#define LOCALE_STRING_METADATA_PARTY 30
#define LOCALE_STRING_MIC_BIAS_OFF 31
#define LOCALE_STRING_MIC_BIAS_ON 32
#define LOCALE_STRING_MIC_GAIN 33
#define LOCALE_STRING_NO_DEVICE 34
#define LOCALE_STRING_PAIRING_OFF 35
#define LOCALE_STRING_PAIRING_ON 36
#define LOCALE_STRING_SN 37
#define LOCALE_STRING_SETTINGS 38
#define LOCALE_STRING_SETTINGS_ABOUT 39
#define LOCALE_STRING_SETTINGS_AUDIO 40
#define LOCALE_STRING_SETTINGS_CALLING 41
#define LOCALE_STRING_SETTINGS_COMFORT 42
#define LOCALE_STRING_SETTINGS_UI 43
#define LOCALE_STRING_TEMPS_AMBIENT 64
#define LOCALE_STRING_TEMPS_OIL 65
#define LOCALE_STRING_TEMPS_COOLANT 44
#define LOCALE_STRING_TEMPS_OFF 45
#define LOCALE_STRING_UI 46
#define LOCALE_STRING_UNKNOWN_ALBUM 47
#define LOCALE_STRING_UNKNOWN_ARTIST 48
#define LOCALE_STRING_UNKNOWN_TITLE 49
#define LOCALE_STRING_UNLOCK_OFF 50
#define LOCALE_STRING_UNLOCK_POS_0 51
#define LOCALE_STRING_UNLOCK_POS_1 52
#define LOCALE_STRING_VOLUME_NEG_DB 53
#define LOCALE_STRING_VOLUME_POS_DB 54
#define LOCALE_STRING_VOLUME_24_DB 55
#define LOCALE_STRING_VOLUME_0_DB 56
#define LOCALE_STRING_LANG 57
#define LOCALE_STRING_PARK_LAMPS_ON 58
#define LOCALE_STRING_PARK_LAMPS_OFF 59
#define LOCALE_STRING_MANAGE_VOL_ON 60
#define LOCALE_STRING_MANAGE_VOL_OFF 61
#define LOCALE_STRING_REV_VOL_LOW_ON 62
#define LOCALE_STRING_REV_VOL_LOW_OFF 63
#define LOCALE_STRING_DASH_OBC_ON 67
#define LOCALE_STRING_DASH_OBC_OFF 68

char *LocaleGetText(uint16_t);
#endif /* LOCALE_H */
