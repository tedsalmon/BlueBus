/*
 * File: handler.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement the logic to have the BC127 and IBus communicate
 */
#ifndef HANDLER_H
#define HANDLER_H
#include "lib/bt/bt_common.h"
#include "lib/ibus.h"

void HandlerInit(BT_t *, IBus_t *);
void HandlerUICloseConnection(void *, unsigned char *);
void HandlerUIInitiateConnection(void *, unsigned char *);
void HandlerTimerPoweroff(void *);
#endif /* HANDLER_H */
