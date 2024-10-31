/*
 * File:   ibus.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#ifndef IBUS_H
#define IBUS_H
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "../mappings.h"
#include "char_queue.h"
#include "log.h"
#include "event.h"
#include "ibus.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"

// Devices
#define IBUS_DEVICE_GM 0x00 /* Body module */
#define IBUS_DEVICE_CDC 0x18 /* CD Changer */
#define IBUS_DEVICE_FUH 0x28 /* Radio controlled clock */
#define IBUS_DEVICE_CCM 0x30 /* Check control module */
#define IBUS_DEVICE_GT 0x3B /* Graphics driver (in navigation system) */
#define IBUS_DEVICE_DIA 0x3F /* Diagnostic */
#define IBUS_DEVICE_GTF 0x43 /* Graphics driver for rear screen (in navigation system) */
#define IBUS_DEVICE_EWS 0x44 /* EWS (Immobileiser) */
#define IBUS_DEVICE_CID 0x46 /* Central information display (flip-up LCD screen) */
#define IBUS_DEVICE_MFL 0x50 /* Multi function steering wheel */
#define IBUS_DEVICE_IHK 0x5B /* HVAC */
#define IBUS_DEVICE_PDC 0x60 /* Park Distance Control */
#define IBUS_DEVICE_RAD 0x68 /* Radio */
#define IBUS_DEVICE_DSP 0x6A /* DSP */
#define IBUS_DEVICE_SM0 0x72 /* Seat memory - 0 */
#define IBUS_DEVICE_SDRS 0x73 /* Sirius Radio */
#define IBUS_DEVICE_CDCD 0x76 /* CD changer, DIN size. */
#define IBUS_DEVICE_NAVE 0x7F /* Navigation (Europe) */
#define IBUS_DEVICE_IKE 0x80 /* Instrument cluster electronics */
#define IBUS_DEVICE_SES 0xB0 /* Speach recognition system */
#define IBUS_DEVICE_JNAV 0xBB /* Japanese Navigation */
#define IBUS_DEVICE_GLO 0xBF /* Global, broadcast address */
#define IBUS_DEVICE_MID 0xC0 /* Multi-info display */
#define IBUS_DEVICE_TEL 0xC8 /* Telephone */
#define IBUS_DEVICE_TCU 0xCA /* BMW Assist */
#define IBUS_DEVICE_LCM 0xD0 /* Light control module */
#define IBUS_DEVICE_IRIS 0xE0 /* Integrated radio information system */
#define IBUS_DEVICE_ANZV 0xE7 /* Front display */
#define IBUS_DEVICE_VM 0xED /* Video Module */
#define IBUS_DEVICE_BMBT 0xF0 /* On-board monitor */
#define IBUS_DEVICE_LOC 0xFF /* Local */

#define IBUS_DEVICE_BLUEBUS IBUS_DEVICE_CDC /* Reuse CDC Address as we know a CDC will never be present with the BlueBus */

// IBus Packet Indices
#define IBUS_PKT_SRC 0
#define IBUS_PKT_LEN 1
#define IBUS_PKT_DST 2
#define IBUS_PKT_CMD 3
#define IBUS_PKT_DB1 4
#define IBUS_PKT_DB2 5
#define IBUS_PKT_DB3 6

// IBus Commands
#define IBUS_COMMAND_CDC_REQUEST 0x38
#define IBUS_COMMAND_CDC_RESPONSE 0x39

// CDC Commands
#define IBUS_CDC_CMD_GET_STATUS 0x00
#define IBUS_CDC_CMD_STOP_PLAYING 0x01
#define IBUS_CDC_CMD_PAUSE_PLAYING 0x02
#define IBUS_CDC_CMD_START_PLAYING 0x03
#define IBUS_CDC_CMD_CHANGE_TRACK 0x0A
#define IBUS_CDC_CMD_SEEK 0x04
#define IBUS_CDC_CMD_CHANGE_TRACK_BLAUPUNKT 0x05
#define IBUS_CDC_CMD_CD_CHANGE 0x06
#define IBUS_CDC_CMD_SCAN 0x07
#define IBUS_CDC_CMD_RANDOM_MODE 0x08
// CDC Status
#define IBUS_CDC_STAT_STOP 0x00
#define IBUS_CDC_STAT_PAUSE 0x01
#define IBUS_CDC_STAT_PLAYING 0x02
#define IBUS_CDC_STAT_FAST_FWD 0x03
#define IBUS_CDC_STAT_FAST_REV 0x04
#define IBUS_CDC_STAT_END 0x07
#define IBUS_CDC_STAT_LOADING 0x08
// CDC Function
#define IBUS_CDC_FUNC_NOT_PLAYING 0x02
#define IBUS_CDC_FUNC_PLAYING 0x09
#define IBUS_CDC_FUNC_PAUSE 0x0C
#define IBUS_CDC_FUNC_SCAN_MODE 0x19
#define IBUS_CDC_FUNC_RANDOM_MODE 0x29
// CDC Disc Count
#define IBUS_CDC_DISC_LOADED_1 0x01
#define IBUS_CDC_DISC_LOADED_6 0x20
#define IBUS_CDC_DISC_LOADED_7 0x40
#define IBUS_CDC_DISC_LOADED_ALL 0x3F

// DSP
#define IBUS_DSP_CMD_CONFIG_SET 0x36
#define IBUS_DSP_CONFIG_SET_INPUT_RADIO 0xA1
#define IBUS_DSP_CONFIG_SET_INPUT_SPDIF 0xA0

// All buttons presses are triggered on the "Push" message
#define IBUS_DEVICE_BMBT_Button_Next 0x00
#define IBUS_DEVICE_BMBT_Button_Prev 0x10
#define IBUS_DEVICE_BMBT_Button_Mode 0x23
#define IBUS_DEVICE_BMBT_Button_PlayPause 0x14
#define IBUS_DEVICE_BMBT_Button_Knob 0x05
#define IBUS_DEVICE_BMBT_Button_Display 0x30
#define IBUS_DEVICE_BMBT_Button_Info 0x38
#define IBUS_DEVICE_BMBT_Button_SEL 0x0F
#define IBUS_DEVICE_BMBT_Button_Num1 0x11
#define IBUS_DEVICE_BMBT_Button_Num2 0x01
#define IBUS_DEVICE_BMBT_Button_Num3 0x12
#define IBUS_DEVICE_BMBT_Button_Num4 0x02
#define IBUS_DEVICE_BMBT_Button_Num5 0x13
#define IBUS_DEVICE_BMBT_Button_Num6 0x03
#define IBUS_DEVICE_BMBT_Button_TEL_Press 0x08
#define IBUS_DEVICE_BMBT_Button_TEL_Hold 0x48
#define IBUS_DEVICE_BMBT_Button_TEL_Release 0x88
#define IBUS_DEVICE_BMBT_BUTTON_PWR_PRESS 0x06
#define IBUS_DEVICE_BMBT_BUTTON_PWR_RELEASE 0x86
#define IBUS_CMD_BMBT_BUTTON0 0x47
#define IBUS_CMD_BMBT_BUTTON1 0x48

#define IBUS_CMD_DIA_JOB_REQUEST 0x0C
#define IBUS_CMD_DIA_DIAG_RESPONSE 0xA0

#define IBUS_CMD_EWS_IMMOBILISER_STATUS 0x74

#define IBUS_CMD_GM_DOORS_FLAPS_STATUS_RESP 0x7A
#define IBUS_CMD_ZKE3_GM4_JOB_CENTRAL_LOCK 0x0B
#define IBUS_CMD_ZKE3_GM4_JOB_LOCK_HIGH 0x40
#define IBUS_CMD_ZKE3_GM4_JOB_LOCK_LOW 0x41
#define IBUS_CMD_ZKE3_GM4_JOB_LOCK_ALL 0x88
#define IBUS_CMD_ZKE3_GM6_JOB_LOCK_ALL 0x90
#define IBUS_CMD_ZKE3_GM4_JOB_UNLOCK_HIGH 0x42
#define IBUS_CMD_ZKE3_GM4_JOB_UNLOCK_LOW 0x43
#define IBUS_CMD_ZKE5_JOB_CENTRAL_LOCK 0x03
#define IBUS_CMD_ZKE5_JOB_LOCK_ALL 0x34
#define IBUS_CMD_ZKE5_JOB_UNLOCK_LOW 0x37
#define IBUS_CMD_ZKE5_JOB_UNLOCK_ALL 0x45

#define IBUS_CMD_VOLUME_SET 0x32


#define IBUS_CMD_GT_WRITE_NO_CURSOR 0x21

#define IBUS_CMD_GT_CHANGE_UI_REQ 0x20
#define IBUS_CMD_GT_CHANGE_UI_RESP 0x21
#define IBUS_CMD_GT_MENU_BUFFER_STATUS 0x22
#define IBUS_CMD_GT_WRITE_TITLE 0x23
#define IBUS_CMD_GT_MENU_SELECT 0x31
#define IBUS_CMD_GT_DISPLAY_RADIO_MENU 0x37
#define IBUS_CMD_GT_SCREEN_MODE_SET 0x45
#define IBUS_CMD_GT_RAD_TV_STATUS 0x4E
#define IBUS_CMD_GT_MONITOR_CONTROL 0x4F
#define IBUS_CMD_GT_WRITE_INDEX 0x60
#define IBUS_CMD_GT_WRITE_INDEX_TMC 0x61
#define IBUS_CMD_GT_WRITE_ZONE 0x62
#define IBUS_CMD_GT_WRITE_STATIC 0x63
#define IBUS_CMD_GT_TELEMATICS_COORDINATES 0xA2
#define IBUS_CMD_GT_TELEMATICS_LOCATION 0xA4
#define IBUS_CMD_GT_WRITE_WITH_CURSOR 0xA5


#define IBUS_CMD_IKE_IGN_STATUS_REQ 0x10
#define IBUS_CMD_IKE_IGN_STATUS_RESP 0x11
#define IBUS_CMD_IKE_SENSOR_REQ 0x12
#define IBUS_CMD_IKE_SENSOR_RESP 0x13
#define IBUS_CMD_IKE_REQ_VEHICLE_TYPE 0x14
#define IBUS_CMD_IKE_RESP_VEHICLE_CONFIG 0x15
#define IBUS_CMD_IKE_SPEED_RPM_UPDATE 0x18
#define IBUS_CMD_IKE_TEMP_UPDATE 0x19
#define IBUS_CMD_IKE_OBC_TEXT 0x24
#define IBUS_CMD_IKE_SET_REQUEST 0x40
#define IBUS_CMD_IKE_SET_REQUEST_TIME 0x01
#define IBUS_CMD_IKE_SET_REQUEST_DATE 0x02
#define IBUS_CMD_IKE_WRITE_NUMERIC 0x44
#define IBUS_CMD_IKE_CCM_WRITE_TEXT 0x1A

#define IBUS_DATA_IKE_CCM_WRITE_CLEAR_TEXT 0x30
#define IBUS_DATA_IKE_NUMERIC_CLEAR 0x20
#define IBUS_DATA_IKE_NUMERIC_WRITE 0x23

#define IBUS_CMD_LCM_REQ_REDUNDANT_DATA 0x53
#define IBUS_CMD_LCM_RESP_REDUNDANT_DATA 0x54
#define IBUS_CMD_LCM_BULB_IND_REQ 0x5A

#define IBUS_CMD_MOD_STATUS_REQ 0x01
#define IBUS_CMD_MOD_STATUS_RESP 0x02

#define IBUS_CMD_PDC_STATUS 0x07
#define IBUS_CMD_PDC_SENSOR_REQUEST 0x1B
#define IBUS_CMD_PDC_SENSOR_RESPONSE 0xA0

#define IBUS_CMD_RAD_LED_TAPE_CTRL 0x4A

#define IBUS_CMD_RAD_SCREEN_MODE_UPDATE 0x46
#define IBUS_CMD_RAD_UPDATE_MAIN_AREA 0x23
#define IBUS_CMD_RAD_C43_SCREEN_UPDATE 0x21
#define IBUS_CMD_RAD_C43_SET_MENU_MODE 0xC0
#define IBUS_CMD_RAD_WRITE_MID_DISPLAY 0x23
#define IBUS_CMD_RAD_WRITE_MID_MENU 0x21

#define IBUS_CMD_VOL_CTRL 0x32

#define IBUS_GT_DETECT_ERROR 0
#define IBUS_GT_MKI 1
#define IBUS_GT_MKII 2
#define IBUS_GT_MKIII 3
// MVIII Nav systems got the new UI at version 40
#define IBUS_GT_MKIII_NEW_UI 4
#define IBUS_GT_MKIV 5
// MKIV Nav systems with version >= 40 support static screens
#define IBUS_GT_MKIV_STATIC 6
#define IBUS_GT_HW_ID_OFFSET 11
#define IBUS_GT_SW_ID_OFFSET 33
#define IBUS_GT_DI_ID_OFFSET 15
#define IBUS_GT_TONE_MENU_OFF 0x08
#define IBUS_GT_SEL_MENU_OFF 0x04
#define IBUS_RAD_HIDE_BODY 0x0C
#define IBUS_RAD_PRIORITY_GT 0x02
#define IBUS_RAD_PRIORITY_RAD 0x01
#define IBUS_GT_MONITOR_OFF 0x00
#define IBUS_GT_MONITOR_AT_KL_R 0x10
#define IBUS_DATA_GT_TELEMATICS_LOCALE 0x01
#define IBUS_DATA_GT_TELEMATICS_STREET 0x02
#define IBUS_DATA_GT_MKIII_MAX_IDX_LEN 14
#define IBUS_DATA_GT_MKIII_MAX_TITLE_LEN 16

#define IBUS_IGNITION_OFF 0x00
#define IBUS_IGNITION_KLR 0x01
#define IBUS_IGNITION_KL15 0x03
#define IBUS_IGNITION_KL50 0x07
// Make up an ignition status for when the ignition
// is off but the radio requests playback to begin
#define IBUS_IGNITION_KL99 0x08

#define IBUS_CMD_OBC_CONTROL 0x41

#define IBUS_IKE_OBC_PROPERTY_TEMPERATURE 0x03
#define IBUS_IKE_OBC_PROPERTY_REQUEST_TEXT 0x01

#define IBUS_LCM_LIGHT_STATUS_REQ 0x5A
#define IBUS_LCM_LIGHT_STATUS_RESP 0x5B
#define IBUS_LCM_DIMMER_STATUS 0x5C
#define IBUS_LCM_IO_STATUS 0x90

// Lamp Status (0x5B) L / R Lamp Byte = 0, Blink Byte = 2
#define IBUS_LM_LEFT_SIG_BIT 5
#define IBUS_LM_RIGHT_SIG_BIT 6
#define IBUS_LM_BLINK_SIG_BIT 2
#define IBUS_LM_SIG_BIT_PARKING 0
#define IBUS_LM_SIG_BIT_LOW_BEAM 1
#define IBUS_LM_SIG_BIT_HIGH_BEAM 2

// LM diagnostics activate (0x0C)
#define IBUS_LM_BLINKER_OFF 0
#define IBUS_LM_BLINKER_LEFT 1
#define IBUS_LM_BLINKER_RIGHT 2

#define IBUS_LM_BULB_OFF 0x00

// LME38 (Byte 0)
#define IBUS_LME38_BLINKER_OFF 0x00
#define IBUS_LME38_BLINKER_LEFT 0x01
#define IBUS_LME38_BLINKER_RIGHT 0x02
// Side Marker Left = Byte 2, Side Marker Right = Byte 3
#define IBUS_LME38_SIDE_MARKER_LEFT 0x02
#define IBUS_LME38_SIDE_MARKER_RIGHT 0x40

// LCM, LCM_A
// Different bytes! Update the blinker msg if alternating.
// Byte 0 (S2_BLK_L switch No.2 left turn / S2_BLK_R switch No.2 right turn)
// #define IBUS_LCM_BLINKER_LEFT 0x80
// #define IBUS_LCM_BLINKER_RIGHT 0x40
// Byte 1 (S2_BLK_L switch No.1 left turn / S2_BLK_R switch No.1 right turn)
#define IBUS_LCM_BLINKER_OFF 0x00
#define IBUS_LCM_BLINKER_LEFT 0x01
#define IBUS_LCM_BLINKER_RIGHT 0x0
// Side Marker Left = Byte 5, Side Marker Right = Byte 6
#define IBUS_LCM_SIDE_MARKER_LEFT 0x01
#define IBUS_LCM_SIDE_MARKER_RIGHT 0x20

// LCM_II, LCM_III, LCM_IV (Byte 2)
#define IBUS_LCM_II_BLINKER_OFF 0x00
#define IBUS_LCM_II_BLINKER_LEFT 0x80
#define IBUS_LCM_II_BLINKER_RIGHT 0x40

// LSZ, LSZ_2 (Headlight Status = Byte 2, Blinker Status = Byte 3)
#define IBUS_LSZ_HEADLIGHT_OFF 0xFF
#define IBUS_LSZ_BLINKER_LEFT 0x50
// Blinker Left = Byte 2, Blinker Right = Byte 3
#define IBUS_LSZ_BLINKER_RIGHT 0x80
#define IBUS_LSZ_BLINKER_OFF 0xFF
// Front Side Marker Left = Byte 5, Front Side Marker Right = Byte 4
#define IBUS_LSZ_SIDE_MARKER_LEFT 0x08
#define IBUS_LSZ_SIDE_MARKER_RIGHT 0x02

// Ident (0x00) parameter offsets
#define IBUS_LM_CI_ID_OFFSET 9
#define IBUS_LM_DI_ID_OFFSET 10
// Status (0x0B) parameter offsets
#define IBUS_LM_IO_LOAD_FRONT_OFFSET 11
#define IBUS_LM_IO_DIMMER_OFFSET 19
#define IBUS_LM_IO_LOAD_REAR_OFFSET 20
#define IBUS_LM_IO_PHOTO_OFFSET 22
// LME38 has unique mapping
#define IBUS_LME38_IO_DIMMER_OFFSET 22

// Light Module variants
#define IBUS_LM_LME38 1
#define IBUS_LM_LCM 2
#define IBUS_LM_LCM_A 3
#define IBUS_LM_LCM_II 4
#define IBUS_LM_LCM_III 5
#define IBUS_LM_LCM_IV 6
#define IBUS_LM_LSZ 7
#define IBUS_LM_LSZ_2 8

#define IBusMIDSymbolNext 0xC9
#define IBusMIDSymbolBack 0xCA

#define IBus_MID_MAX_CHARS 24
#define IBus_MID_TITLE_MAX_CHARS 11
#define IBus_MID_MENU_MAX_CHARS 4
#define IBus_MID_CMD_MODE 0x20
#define IBUS_MID_CMD_SET_MODE 0x27
#define IBUS_MID_MODE_REQUEST_TYPE_PHYSICAL 0x02
#define IBUS_MID_MODE_REQUEST_TYPE_SELF 0x00
#define IBus_MID_Button_Press 0x31
#define IBus_MID_BTN_TEL_RIGHT_RELEASE 0x4D
#define IBus_MID_BTN_TEL_LEFT_RELEASE 0x4C

#define IBUS_MID_UI_TEL_OPEN 0x8E
#define IBUS_MID_UI_TEL_CLOSE 0x8F
#define IBUS_MID_UI_RADIO_OPEN 0xB0

#define IBUS_PDC_DEFAULT_SENSOR_VALUE 0xFF

#define IBUS_TEL_CMD_LED_STATUS 0x2B
#define IBUS_TEL_CMD_STATUS 0x2C
#define IBUS_TEL_CMD_MAIN_MENU 0x21
#define IBUS_TEL_CMD_NUMBER 0x23
#define IBUS_TEL_STATUS_NONE 0x00
#define IBUS_TEL_STATUS_ACTIVE_POWER_HANDSFREE 0x10
#define IBUS_TEL_STATUS_ACTIVE_POWER_CALL_HANDSFREE 0x35
#define IBUS_TEL_LED_STATUS_RED 0x01
#define IBUS_TEL_LED_STATUS_RED_BLINKING 0x03
#define IBUS_TEL_LED_STATUS_GREEN 0x10
#define IBUS_TEL_SIG_EVEREST 0x38

#define IBUS_BLUEBUS_CMD_SET_STATUS 0xBB
#define IBUS_BLUEBUS_CMD_CARPLAY_MODE 0xBC

#define IBUS_BLUEBUS_SUBCMD_SET_STATUS_TEL 0x01

#define IBUS_C43_TITLE_MODE 0xC4

#define IBUS_RADIO_TYPE_C43 1
#define IBUS_RADIO_TYPE_BM53 2
#define IBUS_RADIO_TYPE_BM54 3
#define IBUS_RADIO_TYPE_BRCD 4
#define IBUS_RADIO_TYPE_BRTP 5
#define IBUS_RADIO_TYPE_BM24 6

#define IBUS_RAD_VOLUME_DOWN 0x00
#define IBUS_RAD_VOLUME_UP 0x01

#define IBUS_RAD_SPACE_CHAR_ALT 0x9D

#define IBUS_SENSOR_VALUE_COOLANT_TEMP 0x01
#define IBUS_SENSOR_VALUE_AMBIENT_TEMP 0x02
#define IBUS_SENSOR_VALUE_OIL_TEMP 0x03
#define IBUS_SENSOR_VALUE_TEMP_UNIT 0x04
#define IBUS_SENSOR_VALUE_GEAR_POS 0x05
#define IBUS_SENSOR_VALUE_AMBIENT_TEMP_CALCULATED 0x06

#define IBUS_SES_ZOOM_LEVELS 8

#define IBUS_MFL_CMD_BTN_PRESS 0x3B
#define IBUS_MFL_BTN_EVENT_NEXT_REL 0x21
#define IBUS_MFL_BTN_EVENT_PREV_REL 0x28
#define IBUS_MFL_BTN_EVENT_RT_PRESS 0x40
#define IBUS_MFL_BTN_EVENT_RT_REL 0x00
#define IBUS_MFL_BTN_EVENT_VOICE_PRESS 0x80
#define IBUS_MFL_BTN_EVENT_VOICE_HOLD 0x90
#define IBUS_MFL_BTN_EVENT_VOICE_REL 0xA0

#define IBUS_MFL_CMD_VOL_PRESS 0x32
#define IBUS_MFL_BTN_VOL_UP 0x11
#define IBUS_MFL_BTN_VOL_DOWN 0x10

#define IBUS_VEHICLE_TYPE_E38_E39_E52_E53 0x01
#define IBUS_VEHICLE_TYPE_E46 0x02
#define IBUS_VEHICLE_TYPE_E8X 0x03
#define IBUS_VEHICLE_TYPE_R50 0x04
#define IBUS_IKE_TYPE_LOW 0x00
#define IBUS_IKE_TYPE_HIGH 0x0F
#define IBUS_IKE_GEAR_NONE 0x00
#define IBUS_IKE_GEAR_PARK 0x0B
#define IBUS_IKE_GEAR_REVERSE 0x01
#define IBUS_IKE_GEAR_NEUTRAL 0x07
#define IBUS_IKE_GEAR_FIRST 0x10
#define IBUS_IKE_GEAR_SECOND 0x06
#define IBUS_IKE_GEAR_THIRD 0x0D
#define IBUS_IKE_GEAR_FOURTH 0x0C
#define IBUS_IKE_GEAR_FIFTH 0x0E
#define IBUS_IKE_GEAR_SIXTH 0x0F

#define IBUS_IKE_TEXT_TEMPERATURE 0x03

#define IBUS_GM_ZKE3_GM1 1
#define IBUS_GM_ZKE3_GM4 2
#define IBUS_GM_ZKE3_GM5 3
#define IBUS_GM_ZKE3_GM6 4
#define IBUS_GM_ZKE4 5
#define IBUS_GM_ZKE5 6
#define IBUS_GM_ZKE5_S12 7
#define IBUS_GM_ZKEBC1 8
#define IBUS_GM_ZKEBC1RD 9

#define IBUS_TCU_SINGLE_LINE_UI_MAX_LEN 11
#define IBUS_TELEMATICS_COORDS_LEN 18
#define IBUS_TELEMATICS_LOCATION_LEN 31

// Events
#define IBUS_EVENT_GTDIAIdentityResponse 32
#define IBUS_EVENT_CDPoll 33
#define IBUS_EVENT_CDStatusRequest 34
#define IBUS_EVENT_CDClearDisplay 35
#define IBUS_EVENT_IKEIgnitionStatus 36
#define IBUS_EVENT_BMBTButton 37
#define IBUS_EVENT_GTMenuSelect 38
#define IBUS_EVENT_ScreenModeUpdate 39
#define IBUS_EVENT_RAD_WRITE_DISPLAY 40
#define IBUS_EVENT_ScreenModeSet 41
#define IBUS_EVENT_RADDiagResponse 42
#define IBUS_EVENT_MFLButton 43
#define IBUS_EVENT_RADDisplayMenu 44
#define IBUS_EVENT_RADMIDDisplayText 45
#define IBUS_EVENT_RADMIDDisplayMenu 46
#define IBUS_EVENT_LCMLightStatus 47
#define IBUS_EVENT_LCMDimmerStatus 48
#define IBUS_EVENT_GTWriteResponse 49
#define IBUS_EVENT_MFLVolumeChange 50
#define IBUS_EVENT_MIDButtonPress 51
#define IBUS_EVENT_MIDModeChange 52
#define IBUS_EVENT_MODULE_STATUS_RESP 54
#define IBUS_EVENT_IKE_VEHICLE_CONFIG 55
#define IBUS_EVENT_LCMRedundantData 56
#define IBUS_EVENT_FirstMessageReceived 57
#define IBUS_EVENT_GTDIAOSIdentityResponse 58
#define IBUS_EVENT_IKESpeedRPMUpdate 59
#define IBUS_EVENT_ModuleStatusRequest 60
#define IBUS_EVENT_GTChangeUIRequest 61
#define IBUS_EVENT_DoorsFlapsStatusResponse 62
#define IBUS_EVENT_LCMDiagnosticsAcknowledge 63
#define IBUS_EVENT_DSPConfigSet 64
#define IBUS_EVENT_TELVolumeChange 65
#define IBUS_EVENT_RADVolumeChange 66
#define IBUS_EVENT_LMIdentResponse 67
#define IBUS_EVENT_TV_STATUS 68
#define IBUS_EVENT_PDC_STATUS 69
#define IBUS_EVENT_PDC_SENSOR_UPDATE 70
#define IBUS_EVENT_SENSOR_VALUE_UPDATE 71
#define IBUS_EVENT_SCREEN_BUFFER_FLUSH 72
#define IBUS_EVENT_GT_TELEMATICS_DATA 73
#define IBUS_EVENT_BLUEBUS_TEL_STATUS_UPDATE 74
#define IBUS_EVENT_VM_IDENT_RESP 75
#define IBUS_EVENT_GT_MENU_BUFFER_UPDATE 76
#define IBUS_EVENT_RAD_MESSAGE_RCV 77
#define IBUS_EVENT_MONITOR_STATUS 78
#define IBUS_EVENT_MONITOR_CONTROL 79

// Configuration and protocol definitions
#define IBUS_MAX_MSG_LENGTH 47 // Src Len Dest Cmd Data[42 Byte Max] XOR
#define IBUS_RAD_MAIN_AREA_WATERMARK 0x10
#define IBUS_RX_BUFFER_SIZE 255 // 8-bit Max
#define IBUS_TX_BUFFER_SIZE 16
#define IBUS_RX_BUFFER_TIMEOUT 70 // At 9600 baud, we transmit ~1.5 byte/ms
#define IBUS_TX_BUFFER_WAIT 7 // If we transmit faster, other modules may not hear us
#define IBUS_TX_TIMEOUT_OFF 0
#define IBUS_TX_TIMEOUT_ON 1
#define IBUS_TX_TIMEOUT_DATA_SENT 2
#define IBUS_TX_TIMEOUT_WAIT 250

/**
 * IBusModuleStatus_t
 *     Description:
 *         This object tracks the existence of certain modules on the bus based
 *         on traffic seen from them
 */
typedef struct IBusModuleStatus_t {
    uint8_t BMBT: 1;
    uint8_t DSP: 1;
    uint8_t GT: 1;
    uint8_t IKE: 1;
    uint8_t LCM: 1;
    uint8_t MID: 1;
    uint8_t NAV: 1;
    uint8_t RAD: 1;
    uint8_t IRIS: 1;
    uint8_t VM: 1;
    uint8_t PDC: 1;
} IBusModuleStatus_t;

/**
 * IBusPDCSensorStatus_t
 *     Description:
 *         This object tracks the PDC distances given by each sensor
 */
typedef struct IBusPDCSensorStatus_t {
    uint8_t frontLeft;
    uint8_t frontCenterLeft;
    uint8_t frontCenterRight;
    uint8_t frontRight;
    uint8_t rearLeft;
    uint8_t rearCenterLeft;
    uint8_t rearCenterRight;
    uint8_t rearRight;
} IBusPDCSensorStatus_t;

/**
 * IBus_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the I-Bus
 */
typedef struct IBus_t {
    UART_t uart;
    uint8_t rxBuffer[IBUS_RX_BUFFER_SIZE];
    uint8_t rxBufferIdx;
    uint8_t txBuffer[IBUS_TX_BUFFER_SIZE][IBUS_MAX_MSG_LENGTH];
    uint8_t txBufferReadbackIdx;
    uint8_t txBufferReadIdx;
    uint8_t txBufferWriteIdx;
    uint32_t rxLastStamp;
    uint32_t txLastStamp;
    signed char ambientTemperature;
    char ambientTemperatureCalculated[7];
    uint8_t coolantTemperature;
    uint8_t cdChangerFunction;
    uint8_t gearPosition: 4;
    uint8_t gtVersion;
    uint8_t ignitionStatus: 4;
    uint8_t lmDimmerVoltage;
    uint8_t lmLoadFrontVoltage;
    uint8_t lmLoadRearVoltage;
    uint8_t lmPhotoVoltage;
    uint8_t lmVariant;
    uint8_t oilTemperature;
    uint8_t vehicleType;
    IBusModuleStatus_t moduleStatus;
    IBusPDCSensorStatus_t pdcSensors;
    char telematicsLocale[IBUS_TELEMATICS_LOCATION_LEN];
    char telematicsStreet[IBUS_TELEMATICS_LOCATION_LEN];
    char telematicsLatitude[IBUS_TELEMATICS_COORDS_LEN];
    char telematicsLongtitude[IBUS_TELEMATICS_COORDS_LEN];
} IBus_t;

IBus_t IBusInit();
void IBusProcess(IBus_t *);
void IBusSendCommand(IBus_t *, const uint8_t, const uint8_t, const uint8_t *, const size_t);
void IBusSetInternalIgnitionStatus(IBus_t *, uint8_t);
uint8_t IBusGetLMCodingIndex(uint8_t *);
uint8_t IBusGetLMDiagnosticIndex(uint8_t *);
uint8_t IBusGetLMDimmerChecksum(uint8_t *);
uint8_t IBusGetLMVariant(uint8_t *);
uint8_t IBusGetNavDiagnosticIndex(uint8_t *);
uint8_t IBusGetNavHWVersion(uint8_t *);
uint8_t IBusGetNavSWVersion(uint8_t *);
uint8_t IBusGetNavType(uint8_t *);
uint8_t IBusGetVehicleType(uint8_t *);
uint8_t IBusGetConfigTemp(uint8_t *);
uint8_t IBusGetConfigDistance(uint8_t *);
uint8_t IBusGetConfigLanguage(uint8_t *);
void IBusCommandBlueBusSetStatus(IBus_t *, uint8_t, uint8_t);
void IBusCommandCDCAnnounce(IBus_t *);
void IBusCommandCDCStatus(IBus_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void IBusCommandDIAGetCodingData(IBus_t *, uint8_t, uint8_t, uint8_t);
void IBusCommandDIAGetIdentity(IBus_t *, uint8_t);
void IBusCommandDIAGetIOStatus(IBus_t *, uint8_t);
void IBusCommandDIAGetOSIdentity(IBus_t *, uint8_t);
void IBusCommandDIATerminateDiag(IBus_t *, uint8_t);
void IBusCommandDSPSetMode(IBus_t *, uint8_t);
void IBusCommandGetModuleStatus(IBus_t *, uint8_t, uint8_t);
void IBusCommandSetModuleStatus(IBus_t *, uint8_t, uint8_t, uint8_t);
void IBusCommandGMDoorCenterLockButton(IBus_t *);
void IBusCommandGMDoorUnlockHigh(IBus_t *);
void IBusCommandGMDoorUnlockLow(IBus_t *);
void IBusCommandGMDoorLockHigh(IBus_t *);
void IBusCommandGMDoorLockLow(IBus_t *);
void IBusCommandGMDoorUnlockAll(IBus_t *);
void IBusCommandGMDoorLockAll(IBus_t *);
void IBusCommandGTBMBTControl(IBus_t *, uint8_t);
void IBusCommandGTUpdate(IBus_t *, uint8_t);
void IBusCommandGTWriteBusinessNavTitle(IBus_t *, char *);
void IBusCommandGTWriteIndex(IBus_t *, uint8_t, char *);
void IBusCommandGTWriteIndexTMC(IBus_t *, uint8_t, char *);
void IBusCommandGTWriteIndexTitle(IBus_t *, char *);
void IBusCommandGTWriteIndexTitleNGUI(IBus_t *, char *);
void IBusCommandGTWriteIndexStatic(IBus_t *, uint8_t, char *);
void IBusCommandGTWriteTitleArea(IBus_t *, char *);
void IBusCommandGTWriteTitleIndex(IBus_t *, char *);
void IBusCommandGTWriteTitleC43(IBus_t *, char *);
void IBusCommandGTWriteZone(IBus_t *, uint8_t, char *);
void IBusCommandIKEGetIgnitionStatus(IBus_t *);
void IBusCommandIKEGetVehicleConfig(IBus_t *);
void IBusCommandIKEOBCControl(IBus_t *, uint8_t, uint8_t);
void IBusCommandIKESetIgnitionStatus(IBus_t *, uint8_t);
void IBusCommandIKESetTime(IBus_t *, uint8_t, uint8_t);
void IBusCommandIKESetDate(IBus_t *, uint8_t, uint8_t, uint8_t);
void IBusCommandIRISDisplayWrite(IBus_t *, char *);
void IBusCommandTELIKEDisplayWrite(IBus_t *, char *);
void IBusCommandTELIKEDisplayClear(IBus_t *);
void IBusCommandIKECheckControlDisplayWrite(IBus_t *, char *);
void IBusCommandIKECheckControlDisplayClear(IBus_t *);
void IBusCommandIKENumbericDisplayWrite(IBus_t *, uint8_t);
void IBusCommandIKENumbericDisplayClear(IBus_t *);
void IBusCommandLMActivateBulbs(IBus_t *, uint8_t, uint8_t);
void IBusCommandLMGetClusterIndicators(IBus_t *);
void IBusCommandLMGetRedundantData(IBus_t *);
void IBusCommandMIDButtonPress(IBus_t *, uint8_t, uint8_t);
void IBusCommandMIDDisplayRADTitleText(IBus_t *, char *);
void IBusCommandMIDDisplayText(IBus_t *, char *);
void IBusCommandMIDMenuWriteMany(IBus_t *, uint8_t, uint8_t *, uint8_t);
void IBusCommandMIDMenuWriteSingle(IBus_t *, uint8_t, char *);
void IBusCommandMIDSetMode(IBus_t *, uint8_t, uint8_t);
void IBusCommandPDCGetSensorStatus(IBus_t *);
void IBusCommandRADC43ScreenModeSet(IBus_t *, uint8_t);
void IBusCommandRADCDCRequest(IBus_t *, uint8_t);
void IBusCommandRADClearMenu(IBus_t *);
void IBusCommandRADDisableMenu(IBus_t *);
void IBusCommandRADEnableMenu(IBus_t *);
void IBusCommandRADExitMenu(IBus_t *);
void IBusCommandSESSetMapZoom(IBus_t *, uint8_t);
void IBusCommandSetVolume(IBus_t *, uint8_t, uint8_t, uint8_t);
void IBusCommandTELSetGTDisplayMenu(IBus_t *);
void IBusCommandTELSetGTDisplayNumber(IBus_t *, char *);
void IBusCommandTELSetLED(IBus_t *, uint8_t);
void IBusCommandTELStatus(IBus_t *, uint8_t);
void IBusCommandTELStatusText(IBus_t *, char *, uint8_t);
void IBusCommandCarplayDisplay(IBus_t *ibus, uint8_t enable);
#endif /* IBUS_H */
