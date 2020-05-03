/*
 * File:   pcm51xx.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Utilities for use with the on-board PCM5122 DAC
 */
#include "pcm51xx.h"

/**
 * PCM51XXInit()
 *     Description:
 *         Initialize our PCM51XX module by writing the requisite registers
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void PCM51XXInit()
{
     int8_t status = I2CPoll(PCM51XX_I2C_ADDR);
    if (status != 0x00) {
        LogError("PCM51XX Responded with %d during initialization", status);
    } else {
        LogDebug(LOG_SOURCE_SYSTEM, "PCM51XX Responded to Poll");
        unsigned char volume = ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL);
        status = I2CWrite(PCM51XX_I2C_ADDR, PCM51XX_REGISTER_VOLL, volume);
        if (status != 0x00) {
            LogError("PCM51XX failed to set VOLL [%d]", status);
        }
        status = I2CWrite(PCM51XX_I2C_ADDR, PCM51XX_REGISTER_VOLR, volume);
        if (status != 0x00) {
            LogError("PCM51XX failed to set VOLR [%d]", status);
        }
        TimerRegisterScheduledTask(&PCM51XXPollTimer, 0, PCM51XX_POLL_INT);
    }
}

/**
 * PCM51XXPollTimer()
 *     Description:
 *         Periodically poll the PCM51XX
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void PCM51XXPollTimer(void *ctx)
{
    int8_t status = I2CPoll(PCM51XX_I2C_ADDR);
    if (status != 0x00) {
        LogError("PCM51XX Responded with %d", status);
    }
}

/**
 * PCM51XXSetVolume()
 *     Description:
 *         Set the PCM51XX Volume
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void PCM51XXSetVolume(unsigned char volume)
{
    int8_t status = I2CPoll(PCM51XX_I2C_ADDR);
    if (status != 0x00) {
        LogError("PCM51XX Responded with %d", status);
    } else {
        status = I2CWrite(PCM51XX_I2C_ADDR, PCM51XX_REGISTER_VOLL, volume);
        if (status != 0x00) {
            LogError("PCM51XX failed to set VOLL [%d]", status);
        }
        status = I2CWrite(PCM51XX_I2C_ADDR, PCM51XX_REGISTER_VOLR, volume);
        if (status != 0x00) {
            LogError("PCM51XX failed to set VOLR [%d]", status);
        }
    }
}

