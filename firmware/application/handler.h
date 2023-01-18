/*
 * File: handler.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#ifndef HANDLER_H
#define HANDLER_H
#include "handler/handler_bt.h"
#include "handler/handler_common.h"
#include "handler/handler_ibus.h"
#include "lib/bt/bt_bc127.h"
#include "lib/bt/bt_bm83.h"
#include "lib/bt.h"
#include "lib/log.h"
#include "lib/event.h"
#include "lib/ibus.h"
#include "lib/timer.h"
#include "lib/utils.h"

void HandlerInit(BT_t *, IBus_t *);
void HandlerUICloseConnection(void *, unsigned char *);
void HandlerUIInitiateConnection(void *, unsigned char *);
void HandlerTimerPoweroff(void *);
#endif /* HANDLER_H */
