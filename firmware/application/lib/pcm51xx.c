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
        /**
         * Register 37 - ERROR_IGNORE
         * bit   7 - MCLKSRC - CLK2 0 or OSCCLK 1
         * bit   6 - ALWAYSVALID - Use INVALID Flag 0 or ignore INVALID Flag 1
         * bit   5 - FILLMODE - Data remains static 0 or data is zero filled 1
         * bit   4 - CLKOUTDIS - Disabled 0 or Enabled 1
         * bit   3 - CLKOUTSRC - CLK1 0 or OSCCLK 1
         * bit 2:0 - always 0
         */
        //status = I2CWrite(PCM51XX_I2C_ADDR, PCM51XX_REGISTER_ERROR_IGNORE, 0b01111100);
        //if (status != 0x00) {
        //    LogError("PCM51XX failed to set ERROR_IGNORE [%d]", status);
        //}
        unsigned char volume = ConfigGetSetting(CONFIG_SETTING_DAC_VOL);
        PCM51XXSetVolume(volume);
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
    volume = volume + 0x30;
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

