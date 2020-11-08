/*
 * File:   local.h
 * Author: Aliaksei Prybytkin <alexeypribytkin@gmail.com>
 * Description:
 *     Contains functionality for string localization
 */
#ifndef LOCAL_H
#define LOCAL_H
#include <stdint.h>
#include "config.h"
#include "utils.h"

#define LOCAL_STRING_NOT_PLAYING 0
#define LOCAL_STRING_ABOUT 1
#define LOCAL_STRING_AUDIO 2
#define LOCAL_STRING_AUTOPLAY_OFF 3
#define LOCAL_STRING_AUTOPLAY_ON 4
#define LOCAL_STRING_BACK 5
#define LOCAL_STRING_BLINKERS 6
#define LOCAL_STRING_BLUETOOTH 7
#define LOCAL_STRING_BUILT 8
#define LOCAL_STRING_CALLING 9
#define LOCAL_STRING_CAR_E3X_E53 10
#define LOCAL_STRING_CAR_E46_Z4 11
#define LOCAL_STRING_CAR_UNSET 12
#define LOCAL_STRING_CLEAR_PAIRINGS 13
#define LOCAL_STRING_COMFORT 14
#define LOCAL_STRING_DASHBOARD 15
#define LOCAL_STRING_DEVICES 16
#define LOCAL_STRING_DSP_ANALOG 17
#define LOCAL_STRING_DSP_DIGITAL 18
#define LOCAL_STRING_FW 19
#define LOCAL_STRING_HANDSFREE_OFF 20
#define LOCAL_STRING_HANDSFREE_ON 21
#define LOCAL_STRING_LOCK_10KMH 22
#define LOCAL_STRING_LOCK_20KMH 23
#define LOCAL_STRING_LOCK_OFF 24
#define LOCAL_STRING_MAIN_MENU 25
#define LOCAL_STRING_MENU_DASHBOARD 26
#define LOCAL_STRING_MENU_MAIN 27
#define LOCAL_STRING_METADATA_CHUNK 28
#define LOCAL_STRING_METADATA_OFF 29
#define LOCAL_STRING_METADATA_PARTY 30
#define LOCAL_STRING_MIC_BIAS_OFF 31
#define LOCAL_STRING_MIC_BIAS_ON 32
#define LOCAL_STRING_MIC_GAIN 33
#define LOCAL_STRING_NO_DEVICE 34
#define LOCAL_STRING_PAIRING_OFF 35
#define LOCAL_STRING_PAIRING_ON 36
#define LOCAL_STRING_SN 37
#define LOCAL_STRING_SETTINGS 38
#define LOCAL_STRING_SETTINGS_ABOUT 39
#define LOCAL_STRING_SETTINGS_AUDIO 40
#define LOCAL_STRING_SETTINGS_CALLING 41
#define LOCAL_STRING_SETTINGS_COMFORT 42
#define LOCAL_STRING_SETTINGS_UI 43
#define LOCAL_STRING_TEMPS_COOLANT 44
#define LOCAL_STRING_TEMPS_OFF 45
#define LOCAL_STRING_UI 46
#define LOCAL_STRING_UNKNOWN_ALBUM 47
#define LOCAL_STRING_UNKNOWN_ARTIST 48
#define LOCAL_STRING_UNKNOWN_TITLE 49
#define LOCAL_STRING_UNLOCK_OFF 50
#define LOCAL_STRING_UNLOCK_POS_0 51
#define LOCAL_STRING_UNLOCK_POS_1 52
#define LOCAL_STRING_VOLUME_NEG_DB 53
#define LOCAL_STRING_VOLUME_POS_DB 54
#define LOCAL_STRING_VOLUME_24_DB 55
#define LOCAL_STRING_VOLUME_0_DB 56
#define LOCAL_STRING_LANGUAGE_ENGLISH 57
#define LOCAL_STRING_LANGUAGE_RUSSIAN 58

char * GetText(uint16_t);
#endif /* LOCAL_H */