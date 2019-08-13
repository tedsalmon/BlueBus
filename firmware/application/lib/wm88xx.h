/*
 * File:   wm88xx.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Utilities for use with the on-board WM8804 I2S transceiver
 */
#include "i2c.h"
#include "timer.h"
#include "log.h"

#define WM88XX_I2C_ADDR 0x3A
#define WM88XX_POLL_INT 5000
#define WM88XX_REGISTER_PLL_K_3 3
#define WM88XX_REGISTER_PLL_K_2 4
#define WM88XX_REGISTER_PLL_K_1 5
#define WM88XX_REGISTER_PLL_N 6
#define WM88XX_REGISTER_PLLMODE 7
#define WM88XX_REGISTER_PLLCLK 8
#define WM88XX_REGISTER_AIFTX 27
#define WM88XX_REGISTER_AIFRX 28
#define WM88XX_REGISTER_SPDRX1 29
#define WM88XX_REGISTER_PWR 30

void WM88XXInit();
void WM88XXPollTimer(void *);
