/*
 * File:   ibus.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#ifndef IBUS_H
#define IBUS_H
#define IBUS_CDC_GET_STATUS 0x00
#define IBUS_CDC_PLAYING 0x09
#define IBUS_CDC_NOT_PLAYING 0x02
#define IBUS_CDC_CHANGE_TRACK 0x0A
#define IBUS_CDC_STOP_PLAYING 0x01
#define IBUS_CDC_START_PLAYING 0x02
#define IBUS_CDC_START_PLAYING_CD53 0x03
#define IBUS_CDC_SCAN_FORWARD 0x03
#define IBUS_CDC_SCAN_BACKWARDS 0x04
#define IBUS_CDC_SONG_END 0x05
#define IBUS_CDC_CD_CHANGE 0x06
/* Sending this to the BM53 prevents it from clearing the GT as often */
#define IBUS_CDC_BM53_START_PLAYING 0x01
#define IBUS_CDC_BM53_PLAYING 0x0C
// Commands
#define IBUS_COMMAND_CDC_ALIVE 0x01
#define IBUS_COMMAND_CDC_GET_STATUS 0x38
#define IBUS_COMMAND_CDC_SET_STATUS 0x39
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
#define IBUS_DEVICE_RAD 0x68 /* Radio */
#define IBUS_DEVICE_DSP 0x6A /* DSP */
#define IBUS_DEVICE_SM0 0x72 /* Seat memory - 0 */
#define IBUS_DEVICE_SDRS 0x73 /* Sirius Radio */
#define IBUS_DEVICE_CDCD 0x76 /* CD changer, DIN size. */
#define IBUS_DEVICE_NAVE 0x7F /* Navigation (Europe) */
#define IBUS_DEVICE_IKE 0x80 /* Instrument cluster electronics */
#define IBUS_DEVICE_GLO 0xBF /* Global, broadcast address */
#define IBUS_DEVICE_MID 0xC0 /* Multi-info display */
#define IBUS_DEVICE_TEL 0xC8 /* Telephone */
#define IBUS_DEVICE_TCU 0xCA /* BMW Assist */
#define IBUS_DEVICE_LCM 0xD0 /* Light control module */
#define IBUS_DEVICE_IRIS 0xE0 /* Integrated radio information system */
#define IBUS_DEVICE_ANZV 0xE7 /* Front display */
#define IBUS_DEVICE_BMBT 0xF0 /* On-board monitor */
#define IBUS_DEVICE_LOC 0xFF /* Local */

#define IBUS_GT_MKI 1
#define IBUS_GT_MKII 2
#define IBUS_GT_MKIII 2
#define IBUS_GT_MKIV 4
#define IBUS_GT_HW_ID_OFFSET 11
#define IBUS_GT_SW_ID_OFFSET 33
#define IBUS_IGNITION_OFF 0
#define IBUS_IGNITION_ON 1
// Configuration and protocol definitions
#define IBUS_MAX_MSG_LENGTH 47 // Src Len Dest Cmd Data[42 Byte Max] XOR
#define IBUS_RAD_MAIN_AREA_WATERMARK 0x10
#define IBUS_RX_BUFFER_SIZE 256
#define IBUS_TX_BUFFER_SIZE 48
#define IBUS_RX_BUFFER_TIMEOUT 70 // At 9600 baud, we transmit ~1.5 byte/ms
#define IBUS_TX_BUFFER_WAIT 25 // If we transmit faster, other modules may not hear us
#include <stdint.h>
#include <string.h>
#include "../io_mappings.h"
#include "char_queue.h"
#include "debug.h"
#include "event.h"
#include "ibus.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"


// All buttons presses are triggered on the "Push" message
const static unsigned char IBUS_DEVICE_BMBT_Button_Next = 0x00;
const static unsigned char IBUS_DEVICE_BMBT_Button_Prev = 0x10;
const static unsigned char IBUS_DEVICE_BMBT_Button_PlayPause = 0x14;
const static unsigned char IBUS_DEVICE_BMBT_Button_Knob = 0x05;
const static unsigned char IBUS_DEVICE_BMBT_Button_Display = 0x30;

const static unsigned char IBusAction_BMBT_BUTTON = 0x48;

const static unsigned char IBusAction_CD53_SEEK = 0x0A;
const static unsigned char IBusAction_CD53_CD_SEL = 0x06;

const static unsigned char IBusAction_DIAG_DATA = 0xA0;

const static unsigned char IBusAction_GT_MENU_SELECT = 0x31;
const static unsigned char IBusAction_GT_WRITE_MK4 = 0x21;
const static unsigned char IBusAction_GT_WRITE_TITLE = 0x23;
// Newer GTs use a different action to write to fields
const static unsigned char IBusAction_GT_WRITE_MK2 = 0xA5;
const static unsigned char IBusAction_GT_WRITE_INDEX = 0x60;
const static unsigned char IBusAction_GT_WRITE_ZONE = 0x62;
const static unsigned char IBusAction_GT_WRITE_STATIC = 0x63;

const static unsigned char IBusAction_IGN_STATUS_REQ = 0x11;

const static unsigned char IBusAction_RAD_SCREEN_MODE_UPDATE = 0x46;
const static unsigned char IBusAction_RAD_UPDATE_MAIN_AREA = 0x23;

const static uint8_t IBusEvent_Startup = 33;
const static uint8_t IBusEvent_CDKeepAlive = 34;
const static uint8_t IBusEvent_CDStatusRequest = 35;
const static uint8_t IBusEvent_CDClearDisplay = 36;
const static uint8_t IBusEvent_IgnitionStatus = 37;
const static uint8_t IBusEvent_GTDiagResponse = 38;
const static uint8_t IBusEvent_BMBTButton = 39;
const static uint8_t IBusEvent_GTMenuSelect = 40;
const static uint8_t IBusEvent_ScreenModeUpdate = 41;
const static uint8_t IBusEvent_RADUpdateMainArea = 42;

const static char IBusMIDSymbolNext = 0xC9;
const static char IBusMIDSymbolBack = 0xCA;

/**
 * IBus_t
 *     Description:
 *         This object defines helper functionality to allow us to interact
 *         with the I-Bus
 */
typedef struct IBus_t {
    UART_t uart;
    unsigned char rxBuffer[IBUS_RX_BUFFER_SIZE];
    uint8_t rxBufferIdx;
    uint32_t rxLastStamp;
    unsigned char txBuffer[IBUS_TX_BUFFER_SIZE][IBUS_MAX_MSG_LENGTH];
    uint8_t txBufferReadIdx;
    uint8_t txBufferWriteIdx;
    uint32_t txLastStamp;
    unsigned char cdChangerStatus;
    unsigned char ignitionStatus;
    unsigned char gtCanDisplayStatic;
} IBus_t;
IBus_t IBusInit();
void IBusProcess(IBus_t *);
void IBusStartup();
void IBusCommandCDCAnnounce(IBus_t *);
void IBusCommandCDCKeepAlive(IBus_t *);
void IBusCommandCDCStatus(IBus_t *, unsigned char,  unsigned char);
void IBusCommandGTGetDiagnostics(IBus_t *);
void IBusCommandGTUpdate(IBus_t *, unsigned char);
void IBusCommandGTWriteIndexMk2(IBus_t *, uint8_t, char *);
void IBusCommandGTWriteIndexMk4(IBus_t *, uint8_t, char *);
void IBusCommandGTWriteIndex(IBus_t *, uint8_t, char *, unsigned char, unsigned char);
void IBusCommandGTWriteIndexStatic(IBus_t *ibus, uint8_t, char *);
void IBusCommandGTWriteTitle(IBus_t *ibus, char *);
void IBusCommandGTWriteZone(IBus_t *ibus, uint8_t, char *);
void IBusCommandIKEGetIgnition(IBus_t *);
void IBusCommandMIDText(IBus_t *, char *);
void IBusCommandMIDTextClear(IBus_t *);
void IBusCommandRADDisableMenu(IBus_t *);
void IBusCommandRADEnableMenu(IBus_t *);
void IBusCommandRADUpdateMenu(IBus_t *);
#endif /* IBUS_H */
