/*
 * File:   ibus.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#ifndef IBUS_H
#define IBUS_H
#define IBUS_GT_MKI 1
#define IBUS_GT_MKII 2
#define IBUS_GT_MKIII 2
#define IBUS_GT_MKIV 4
#define IBUS_GT_HW_ID_OFFSET 11
#define IBUS_GT_SW_ID_OFFSET 33
#define IBUS_IGNITION_OFF 0
#define IBUS_IGNITION_ON 1
#define IBUS_MAX_MSG_LENGTH 47 // Src Len Dest Cmd Data[42 Byte Max] XOR
#define IBUS_RAD_MAIN_AREA_WATERMARK 0x10
#define IBUS_RX_BUFFER_SIZE 256
#define IBUS_TX_BUFFER_SIZE 32
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

const static unsigned char IBusDevice_GM = 0x00; /* Body module */
const static unsigned char IBusDevice_CDC = 0x18; /* CD Changer */
const static unsigned char IBusDevice_FUH = 0x28; /* Radio controlled clock */
const static unsigned char IBusDevice_CCM = 0x30; /* Check control module */
const static unsigned char IBusDevice_GT = 0x3B; /* Graphics driver (in navigation system) */
const static unsigned char IBusDevice_DIA = 0x3F; /* Diagnostic */
const static unsigned char IBusDevice_GTF = 0x43; /* Graphics driver for rear screen (in navigation system) */
const static unsigned char IBusDevice_EWS = 0x44; /* EWS (Immobileiser) */
const static unsigned char IBusDevice_CID = 0x46; /* Central information display (flip-up LCD screen) */
const static unsigned char IBusDevice_MFL = 0x50; /* Multi function steering wheel */
const static unsigned char IBusDevice_IHK = 0x5B; /* Integrated heating and air conditioning */
const static unsigned char IBusDevice_RAD = 0x68; /* Radio */
const static unsigned char IBusDevice_DSP = 0x6A; /* Digital signal processing audio amplifier */
const static unsigned char IBusDevice_SM0 = 0x72; /* Seat memory */
const static unsigned char IBusDevice_SDRS = 0x73; /* Sirius Radio */
const static unsigned char IBusDevice_CDCD = 0x76; /* CD changer, DIN size. */
const static unsigned char IBusDevice_NAVE = 0x7F; /* Navigation (Europe) */
const static unsigned char IBusDevice_IKE = 0x80; /* Instrument cluster electronics */
const static unsigned char IBusDevice_GLO = 0xBF; /* Global, broadcast address */
const static unsigned char IBusDevice_MID = 0xC0; /* Multi-info display */
const static unsigned char IBusDevice_TEL = 0xC8; /* Telephone */
const static unsigned char IBusDevice_TCU = 0xCA; /* BMW Assist */
const static unsigned char IBusDevice_LCM = 0xD0; /* Light control module */
const static unsigned char IBusDevice_GTHL = 0xDA; /* unknown */
const static unsigned char IBusDevice_IRIS = 0xE0; /* Integrated radio information system */
const static unsigned char IBusDevice_ANZV = 0xE7; /* Front display */
const static unsigned char IBusDevice_BMBT = 0xF0; /* On-board monitor operating part */
const static unsigned char IBusDevice_LOC = 0xFF; /* Local */

// All buttons presses are triggered on the "Push" message
const static unsigned char IBusDevice_BMBT_Button_Next = 0x00;
const static unsigned char IBusDevice_BMBT_Button_Prev = 0x10;
const static unsigned char IBusDevice_BMBT_Button_PlayPause = 0x14;
const static unsigned char IBusDevice_BMBT_Button_Knob = 0x05;
const static unsigned char IBusDevice_BMBT_Button_Display = 0x30;

const static unsigned char IBusAction_BMBT_BUTTON = 0x48;

const static unsigned char IBusAction_CD_ACTION_NONE = 0x00;
const static unsigned char IBusAction_CD_ACTION_START_PLAYBACK = 0x02;
const static unsigned char IBusAction_CD_KEEPALIVE = 0x01;
const static unsigned char IBusAction_CD_STATUS_REQ = 0x38;
const static unsigned char IBusAction_CD_STATUS_REP = 0x39;

const static unsigned char IBusAction_CD_STATUS_REQ_PING = 0x00;
const static unsigned char IBusAction_CD_STATUS_REQ_STOP = 0x01;
const static unsigned char IBusAction_CD_STATUS_REQ_PLAY = 0x02;

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
    unsigned char gtHardwareVersion;
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
#endif /* IBUS_H */
