/*
 * File:   pcm51xx.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Functions to control the TI PCM51xx DAC via I2C
 */
#include "pcm51xx.h"

void PCM51XXInit()
{
    I2CInit();
}
