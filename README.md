View this project on [CADLAB.io](https://cadlab.io/project/1479). 

# BlueBus
This project aims to create a Bluetooth interface for vehicles equipped with
the BMW IBus. It supports A2DP, AVRCP and the HFP protocols. AVRCP Metadata
will be displayed on all supported devices along with full integration with the
vehicle to control playback. Additional functionality will be added to allow users
access to their vehicles service data and module information. Additional support 
has been added to emulate the factory telephone system.


# Features
* Firmware upgradable
* Full integration with CD53 / BoardMonitor / Multi-Information Display
* User Configurable
* Natively DSP Compatible
* Emulation of the OE telephone system

# Hardware
* PIC24FJ1024GA606 16-bit MCU
* BC127 Bluetooth Module
* Melexis TH3122.4 LIN Transceiver
* WM8804 low jitter S/PDIF decoder
* PAM8406 2-watt amplifier
* PCM5122 Line-Level DAC / 2.1v RMS / 113 dB SNR
* 25LC1024 1MB EEPROM
* FT232RL USB to UART converter
* TMUX154E SPDT IC Switch to change the FT232RL source from MCU to BC127
* Littleblock 750mA replaceable fuse on input side
* Littleblock 500mA fuse on 3.3v rail to prevent error conditions from sinking current

