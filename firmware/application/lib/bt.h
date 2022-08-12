/*
 * File:   bt.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the abstract Bluetooth Module API
 */
#ifndef BT_H
#define BT_H
#include <stdint.h>
#include "../mappings.h"
#include "bt/bt_bc127.h"
#include "bt/bt_bm83.h"
#include "bt/bt_common.h"
#include "uart.h"

BT_t BTInit();
void BTCommandCallAccept(BT_t *);
void BTCommandCallEnd(BT_t *);
void BTCommandDisconnect(BT_t *);
void BTCommandConnect(BT_t *, BTPairedDevice_t *);
void BTCommandGetMetadata(BT_t *);
void BTCommandList(BT_t *);
void BTCommandPause(BT_t *);
void BTCommandPlay(BT_t *);
void BTCommandPlaybackTrackFastforwardStart(BT_t *);
void BTCommandPlaybackTrackFastforwardStop(BT_t *);
void BTCommandPlaybackTrackRewindStart(BT_t *);
void BTCommandPlaybackTrackRewindStop(BT_t *);
void BTCommandPlaybackTrackNext(BT_t *);
void BTCommandPlaybackTrackPrevious(BT_t *);
void BTCommandProfileOpen(BT_t *);
void BTCommandSetConnectable(BT_t *, unsigned char);
void BTCommandSetDiscoverable(BT_t *, unsigned char);
void BTCommandToggleVoiceRecognition(BT_t *);
uint8_t BTHasActiveMacId(BT_t *);
void BTProcess(BT_t *);
#endif /* BT_H */
