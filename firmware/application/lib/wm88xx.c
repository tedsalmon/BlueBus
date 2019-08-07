/*
 * File:   wm88xx.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Utilities for use with the on-board WM8804 I2S transceiver
 */
#include "wm88xx.h"

/**
 * WM88XXInit()
 *     Description:
 *         Initialize our WM88XX module by writing the requisite registers
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void WM88XXInit()
{
    int8_t status = I2CPoll(WM88XX_I2C_ADDR);
    if (status != 0x00) {
        LogError("WM88XX Responded with %d during initialization", status);
    } else {
        LogDebug(LOG_SOURCE_SYSTEM, "WM88XX Responded to Poll");
        /**
         * Register 7 - PLL_MODE
         * bit 7:6 - always 0
         * bit 5:4 - CLKOUTDIV - 00 = 512fs, 01 = 256fs, 10 = 128fs, 11 = 64fs
         * bit   3 - MCLKDIV - See table 23 for PLL User Mode or 28 for reciever
         * bit   2 - FRACEN - Fractional 1 or integer PLL 0
         * bit 1:0 - FREQMODE - Automatically set by the S/PDIF receiver, so always 00
         */
        // MCLK Set to 1, so 128fs CLK2 & CLKOUTDIV to 512fs
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PLLMODE, 0b00001100);
        if (status != 0x00) {
            LogError("WM88XX failed to set PLLMODE");
        }

        /**
         * Register 8 - PLL_CLK
         * bit   7 - MCLKSRC - CLK2 0 or OSCCLK 1
         * bit   6 - ALWAYSVALID - Use INVALID Flag 0 or ignore INVALID Flag 1
         * bit   5 - FILLMODE - Data remains static 0 or data is zero filled 1
         * bit   4 - CLKOUTDIS - Disabled 0 or Enabled 1
         * bit   3 - CLKOUTSRC - CLK1 0 or OSCCLK 1
         * bit 2:0 - always 0
         */
        // Output to CLK1 & Fill data to all zeros
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PLLCLK, 0b00110000);
        if (status != 0x00) {
            LogError("WM88XX failed to set PLLCLK");
        }

        /**
         * Register 27 - AIFTX
         * bit 7:6 - always 0
         * bit   5 - LRCLK polarity - 0 normal or 1 Inverted
         * bit   4 - BCLK invert - 0 normal or 1 Inverted
         * bit 3:2 - Word length - 10 (24bits), 01 (20 bits), or 00 (16bits)
         * bit 1:0 - Format: 11 (DSP), 10 (I2S), 01 (LJ), 00 (RJ)
         */
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_AIFTX, 0b00001010);
        if (status != 0x00) {
            LogError("WM88XX failed to set AIFTX");
        }

        /**
         * Register 28 - AIFRX
         * bit   7 - Keep BLCK/LRCK Enabled always - 0 is no or 1 yes
         * bit   6 - Mode Select - 0 slave or 1 master
         * bit   5 - LRCLK polarity - 0 normal or 1 Inverted
         * bit   4 - BCLK invert - 0 normal or 1 Inverted
         * bit 3:2 - Word length - 10 (24bits), 01 (20 bits), or 00 (16bits)
         * bit 1:0 - Format: 11 (DSP), 10 (I2S), 01 (LJ), 00 (RJ)
         */
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_AIFRX, 0b01001010);
        if (status != 0x00) {
            LogError("WM88XX failed to set AIFRX");
        }
        
        /**
         * Set the PLL_N and PLL_K factors
         * 
         * Register 6 - PLL_N
         *
         * PLL_K to 36FD21
         * Register 5 -> 0x36
         * Register 4-> 0xFD
         * Register 3 -> 0x21
         */
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PLL_N, 7);
        if (status != 0x00) {
            LogError("WM88XX failed to set PLL_N");
        }
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PLL_K_1, 0x36);
        if (status != 0x00) {
            LogError("WM88XX failed to set first bit of PLL_K");
        }
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PLL_K_2, 0xFD);
        if (status != 0x00) {
            LogError("WM88XX failed to set second bit of PLL_K");
        }
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PLL_K_3, 0x21);
        if (status != 0x00) {
            LogError("WM88XX failed to set third bit of PLL_K");
        }
        
        /**
         * Register 29 - SPDRX1
         * bit   7 - SPD_192K_EN - 192khz Streams disabled 0 or enabled 1
         * bit   6 - WL_MASK - Word length truncated 0 or not truncated 1
         * bit   5 - Always 0
         * bit   4 - WITHFLAG - With flags disabled 0 or with flags enabled 1
         * bit   3 - CONT - Disabled 0 or Enabled 1
         * bit 2:0 - READMUX - See Page 61 [000 default]
         */
        // Set the receiver to disable 192khz streams
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_SPDRX1, 0);
        if (status != 0x00) {
            LogError("WM88XX failed to set SPDRX1");
        }
        // Power the device up
        status = I2CWrite(WM88XX_I2C_ADDR, WM88XX_REGISTER_PWR, 0);
        if (status != 0x00) {
            LogError("WM88XX failed to power on");
        }
        TimerRegisterScheduledTask(&WM88XXPollTimer, 0, WM88XX_POLL_INT);
    }
}

/**
 * WM88XXPollTimer()
 *     Description:
 *         Initialize our WM88XX module by writing the requisite registers
 *     Params:
 *         void *ctx - The context provided at registration
 *     Returns:
 *         void
 */
void WM88XXPollTimer(void *ctx)
{
    int8_t status = I2CPoll(WM88XX_I2C_ADDR);
    if (status != 0x00) {
        LogError("WM88XX Responded with %d", status);
    } else {
        LogDebug(LOG_SOURCE_SYSTEM, "WM88XX Responded to Poll");
    }
}
