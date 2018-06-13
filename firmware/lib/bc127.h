/* 
 * File:   bc127.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * This code implements the Sierra Wireless BC127 Bluetooth API
 */

#ifndef BC127_H
#define	BC127_H

#define BC127_AVRCP_STATUS_PAUSED 0
#define BC127_AVRCP_STATUS_PLAYING 1

typedef struct{
    int avrcpStatus;
    int newAvrcpMeta;
    char *artist;
    char *title;
    char *album;
} BC127;

void pause(BC127 *);
void play(BC127 *);
void sendCommand(BC127 *);

#endif	/* BC127_H */