/*
 * File:   pcm51xx.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Utilities for use with the on-board PCM5122 DAC
 */
#include "config.h"
#include "i2c.h"
#include "log.h"
#include "timer.h"

#define PCM51XX_I2C_ADDR 0x4C
#define PCM51XX_REGISTER_REQUEST_STBY_PWRDN 0x02
#define PCM51XX_REGISTER_ERROR_IGNORE 0x25
#define PCM51XX_REGISTER_VOLL 0x3D
#define PCM51XX_REGISTER_VOLR 0x3E
#define PCM51XX_POLL_INT 5000

void PCM51XXInit();
void PCM51XXPollTimer(void *);
void PCM51XXSetVolume(unsigned char);
void PCM51XXStartup();
