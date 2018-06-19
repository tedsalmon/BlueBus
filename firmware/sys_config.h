/*
 * Configure the system variables
 */
#ifndef SYS_CONFIG_H
#define	SYS_CONFIG_H

// Configure the system

#ifndef SYS_CLOCK
#define SYS_CLOCK 16000000
#endif

// Boot Segment may be written
#pragma config BWRP = OFF
// Boot Segment Code-Protect Level bits (No Protection (other than BWRP))
#pragma config BSS = DISABLED
// Boot Segment Control bit (No Boot Segment)
#pragma config BSEN = OFF
// General Segment Write-Protect bit (General Segment may be written)
#pragma config GWRP = OFF
// General Segment Code-Protect Level bits (No Protection (other than GWRP))
#pragma config GSS = DISABLED
// Configuration Segment Write-Protect bit (Configuration Segment may be written)
#pragma config CWRP = OFF
// Configuration Segment Code-Protect Level bits (No Protection (other than CWRP))
#pragma config CSS = DISABLED
// Alternate Interrupt Vector Table bit (Disabled AIVT)
#pragma config AIVTDIS = OFF

// FBSLIM
// Boot Segment Flash Page Address Limit bits (Boot Segment Flash page address  limit)
#pragma config BSLIM = 0x1FFF

// FOSCSEL
// Oscillator Source Selection (Primary Oscillator with PLL module (XT + PLL, HS + PLL, EC + PLL))
#pragma config FNOSC = PRIPLL
// PLL Mode Selection (96 MHz PLL. (8 MHz input))
#pragma config PLLMODE = PLL96DIV2
// Two-speed Oscillator Start-up Enable bit (Start up with user-selected oscillator source)
#pragma config IESO = OFF

// FOSC
// Primary Oscillator Mode Select bits (HS Crystal Oscillator Mode)
#pragma config POSCMD = HS
// OSC2 Pin Function bit (OSC2 is clock output)
#pragma config OSCIOFCN = OFF
// SOSC Power Selection Configuration bits (SOSC is used in crystal (SOSCI/SOSCO) mode)
#pragma config SOSCSEL = ON
// PLL Secondary Selection Configuration bit (PLL is fed by the Primary oscillator)
#pragma config PLLSS = PLL_PRI
// Peripheral pin select configuration bit (Allow only one reconfiguration)
#pragma config IOL1WAY = ON
// Clock Switching Mode bits (Both Clock switching and Fail-safe Clock Monitor are disabled)
#pragma config FCKSM = CSDCMD

// FWDT
// Watchdog Timer Postscaler bits (1:32,768)
#pragma config WDTPS = PS32768
// Watchdog Timer Prescaler bit (1:128)
#pragma config FWPSA = PR128
// Watchdog Timer Enable bits (WDT and SWDTEN disabled)
#pragma config FWDTEN = OFF
// Watchdog Timer Window Enable bit (Watchdog Timer in Non-Window mode)
#pragma config WINDIS = OFF
// Watchdog Timer Window Select bits (WDT Window is 25% of WDT period)
#pragma config WDTWIN = WIN25
// WDT MUX Source Select bits (WDT clock source is determined by the WDTCLK Configuration bits)
#pragma config WDTCMX = WDTCLK
// WDT Clock Source Select bits (WDT uses LPRC)
#pragma config WDTCLK = LPRC

// FPOR
// Brown Out Enable bit (Brown Out Enable Bit)
#pragma config BOREN = ON
// Low power regulator control (Retention Sleep controlled by RETEN)
#pragma config LPCFG = ON
// Downside Voltage Protection Enable bit (Downside protection enabled using ZPBOR when BOR is inactive)
#pragma config DNVPEN = ENABLE

// FICD
// ICD Communication Channel Select bits (Communicate on PGEC2 and PGED2)
#pragma config ICS = PGD2
// JTAG Enable bit
#pragma config JTAGEN = OFF
// BOOTSWP Disable (BOOTSWP instruction disabled)
#pragma config BTSWP = OFF

// FDEVOPT1
// Alternate Comparator Input Enable bit (C1INC, C2INC, and C3INC are on their standard pin locations)
#pragma config ALTCMPI = DISABLE
// Tamper Pin Enable bit (TMPRN pin function is disabled)
#pragma config TMPRPIN = OFF
// SOSC High Power Enable bit (valid only when SOSCSEL = 1 (Enable SOSC high power mode (default))
#pragma config SOSCHP = ON
// Alternate Voltage Reference Location Enable bit (VREF+ and CVREF+ on RB0, VREF- and CVREF- on RB1)
#pragma config ALTVREF = ALTVREFDIS

#endif	/* SYS_CONFIG_H */
