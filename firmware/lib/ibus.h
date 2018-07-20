/*
 * File:   ibus.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     This implements the I-Bus
 */
#ifndef IBUS_H
#define IBUS_H
#define IBUS_RX_BUFFER_SIZE 256
#define IBUS_TX_BUFFER_SIZE 16
#define IBUS_MAX_MSG_LENGTH 37 // Src Len Dest Cmd Data[32 Byte Max] XOR
#define IBUS_RX_BUFFER_TIMEOUT 50 // At 9600 baud, we transmit ~1 byte/ms
#define IBUS_TX_BUFFER_WAIT 100
#include <stdint.h>
#include <string.h>
#include "../io_mappings.h"
#include "char_queue.h"
#include "debug.h"
#include "event.h"
#include "ibus.h"
#include "timer.h"
#include "uart.h"

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

const static unsigned char IBusAction_CD_ACTION_NONE = 0x00;
const static unsigned char IBusAction_CD_ACTION_START_PLAYBACK = 0x02;
const static unsigned char IBusAction_CD_KEEPALIVE = 0x01;
const static unsigned char IBusAction_CD_STATUS_REQ = 0x38;
const static unsigned char IBusAction_CD_STATUS_REP = 0x39;

const static unsigned char IBusAction_CD_STATUS_REQ_PING = 0x00;
const static unsigned char IBusAction_CD_STATUS_REQ_STOP = 0x01;
const static unsigned char IBusAction_CD_STATUS_REQ_PLAY = 0x01;

const static unsigned char IBusAction_CD53_SEEK = 0x0A;
const static unsigned char IBusAction_CD53_CD_SEL = 0x06;

const static uint8_t IBusEvent_Startup = 33;
const static uint8_t IBusEvent_CDKeepAlive = 34;
const static uint8_t IBusEvent_CDStatusRequest = 35;

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
} IBus_t;
IBus_t IBusInit();
void IBusProcess(IBus_t *);
void IBusSendCommand(IBus_t *, const unsigned char, const unsigned char, const unsigned char *, size_t);
void IBusStartup();
void IBusCommandDisplayText(IBus_t *, char *);
void IBusCommandDisplayTextClear(IBus_t *);
void IBusCommandSendCdChangeAnnounce(IBus_t *);
void IBusCommandSendCdChangerKeepAlive(IBus_t *);
void IBusCommandSendCdChangerStatus(IBus_t *, unsigned char *,  unsigned char *);
void IBusHandleIKEMessage(IBus_t *, unsigned char *);
void IBusHandleRadioMessage(IBus_t *, unsigned char *);
uint8_t IBusValidateChecksum(unsigned char *);
#endif /* IBUS_H */
