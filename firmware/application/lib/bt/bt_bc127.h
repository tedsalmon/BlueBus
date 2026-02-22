/*
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Sierra Wireless BC127 Bluetooth UART API
 */
#ifndef BC127_H
#define BC127_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../../mappings.h"
#include "../log.h"
#include "../event.h"
#include "../timer.h"
#include "../uart.h"
#include "../utils.h"
#include "bt_common.h"

#define BC127_AUDIO_SPDIF "2"
#define BC127_CLOSE_ALL 255
#define BC127_DEVICE_NAME_OFFSET 19
#define BC127_METADATA_TITLE_OFFSET 22
#define BC127_METADATA_ARTIST_OFFSET 23
#define BC127_METADATA_ALBUM_OFFSET 22
#define BC127_MSG_END_CHAR 0x0D
#define BC127_MSG_DELIMETER 0x20
#define BC127_SHORT_NAME_MAX_LEN 8
#define BC127_PROFILE_COUNT 9
#define BC127_RX_QUEUE_TIMEOUT 750
#define BC127_LINK_A2DP 0
#define BC127_LINK_AVRCP 1
#define BC127_LINK_HFP 3
#define BC127_LINK_BLE 4
#define BC127_LINK_MAP 8

extern int8_t BTBC127MicGainTable[];

void BTClearMetadata(BT_t *);
void BC127ClearPairedDevices(BT_t *);
void BC127ClearPairingErrors(BT_t *);
void BC127CommandAT(BT_t *, char *);
void BC127CommandATSet(BT_t *, char *, char *);
void BC127CommandBackward(BT_t *);
void BC127CommandBackwardSeekPress(BT_t *);
void BC127CommandBackwardSeekRelease(BT_t *);
void BC127CommandBtState(BT_t *, uint8_t, uint8_t);
void BC127CommandCallAnswer(BT_t *);
void BC127CommandCallEnd(BT_t *);
void BC127CommandCallReject(BT_t *);
void BC127CommandClose(BT_t *, uint8_t);
void BC127CommandCVC(BT_t *, char *, uint8_t, uint8_t);
void BC127CommandCVCParams(BT_t *, char *);
void BC127CommandForward(BT_t *);
void BC127CommandForwardSeekPress(BT_t *);
void BC127CommandForwardSeekRelease(BT_t *);
void BC127CommandGetDeviceName(BT_t *, BTPairedDevice_t *);
void BC127CommandGetMetadata(BT_t *);
void BC127CommandLicense(BT_t *, char *, char *);
void BC127CommandList(BT_t *);
void BC127CommandPause(BT_t *);
void BC127CommandPlay(BT_t *);
void BC127CommandProfileClose(BT_t *, uint8_t);
void BC127CommandProfileOpen(BT_t *, BTPairedDevice_t *, char *);
void BC127CommandReset(BT_t *);
void BC127CommandSetAudio(BT_t *, uint8_t, uint8_t);
void BC127CommandSetAudioAnalog(BT_t *, uint8_t, uint8_t, uint8_t, char *);
void BC127CommandSetAudioDigital(BT_t *, char *,char *, char *, char *);
void BC127CommandSetAutoConnect(BT_t *, uint8_t);
void BC127CommandSetBtState(BT_t *, uint8_t, uint8_t);
void BC127CommandSetBtVolConfig(BT_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void BC127CommandSetCOD(BT_t *, uint32_t);
void BC127CommandSetCodec(BT_t *, uint8_t, char *);
void BC127CommandSetMetadata(BT_t *, uint8_t);
void BC127CommandSetMicGain(BT_t *, unsigned char, unsigned char, unsigned char);
void BC127CommandSetModuleName(BT_t *, char *);
void BC127CommandSetPin(BT_t *, char *);
void BC127CommandSetProfiles(BT_t *, uint8_t, uint8_t, uint8_t, uint8_t);
void BC127CommandSetUART(BT_t *, uint32_t, char *, uint8_t);
void BC127CommandStatus(BT_t *);
void BC127CommandStatusAVRCP(BT_t *);
void BC127CommandToggleVR(BT_t *);
void BC127CommandTone(BT_t *, char *);
void BC127CommandUnpair(BT_t *);
void BC127CommandVersion(BT_t *);
void BC127CommandVolume(BT_t *, uint8_t, char *);
void BC127CommandWrite(BT_t *);
uint8_t BC127GetDeviceId(char *);
void BC127ProcessEventA2DPStreamSuspend(BT_t *, char **);
void BC127ProcessEventAbsVol(BT_t *, char **);
void BC127ProcessEventAT(BT_t *, char **, uint8_t);
void BC127ProcessEventAVRCPMedia(BT_t *, char **, char *);
void BC127ProcessEventAVRCPPlay(BT_t *, char **);
void BC127ProcessEventAVRCPPause(BT_t *, char **);
void BC127ProcessEventAVRCPPause(BT_t *, char **);
void BC127ProcessEventBuild(BT_t *, char **);
void BC127ProcessEventCall(BT_t *, uint8_t);
void BC127ProcessEventCloseOk(BT_t *, char **);
void BC127ProcessEventLink(BT_t *, char **);
void BC127ProcessEventLinkLoss(BT_t *, char **);
void BC127ProcessEventList(BT_t *, char **);
void BC127ProcessEventName(BT_t *, char **, char *);
void BC127ProcessEventOk(BT_t *, char **);
void BC127ProcessEventOpenError(BT_t *, char **);
void BC127ProcessEventOpenOk(BT_t *, char **);
void BC127ProcessEventPBPull(BT_t *, char **, char *, uint8_t);
void BC127ProcessEventPBPullData(BT_t *, char *);
void BC127ProcessEventSCO(BT_t *, uint8_t);
void BC127ProcessEventState(BT_t *, char **);
void BC127Process(BT_t *);
void BC127SendCommand(BT_t *, char *);
void BC127SendCommandEmpty(BT_t *);

void BC127ConvertMACIDToHex(char *, unsigned char *);
uint8_t BC127ConnectionCloseProfile(BTConnection_t *, char *);
void BC127ConnectionOpenProfile(BTConnection_t *, char *, uint8_t);
#endif /* BC127_H */
