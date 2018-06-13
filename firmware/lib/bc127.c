/* 
 * File:   bc127.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * This code implements the Sierra Wireless BC127 Bluetooth API
 */

#include "lib/bc127.h"

/*
 * BC127 Class Construct
 */
void newBC127(BC127 *c)
{
    c->avrcpStatus = BC127_AVRCP_STATUS_PAUSED;
    c->newAvrcpMeta = 0;
}

/*
 * Pause Playback on the device
 */
void pause(BC127 *c)
{
    sendCommand(*c, "MUSIC PAUSE");
}

void play(BC127 *c)
{
    sendCommand(*c, "MUSIC PLAY");
}

void sendCommand(BC127 *c, char *command)
{
    
}