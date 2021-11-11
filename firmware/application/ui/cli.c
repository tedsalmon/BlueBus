/*
 * File:   cli.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a CLI to pass commands to the device
 */
#include "cli.h"

static CLI_t cli;

/**
 * CLIInit()
 *     Description:
 *         Initialize our CLI object
 *     Params:
 *         UART_t *uart - A pointer to the UART module object
 *         BC127_t *bt - A pointer to the BC127 object
 *         IBus_t *bt - A pointer to the IBus object
 *     Returns:
 *         void
 */
void CLIInit(UART_t *uart, BC127_t *bt, IBus_t *ibus)
{
    cli.uart = uart;
    cli.bt = bt;
    cli.ibus = ibus;
    cli.terminalReady = 0;
    cli.terminalReadyTaskId = TimerRegisterScheduledTask(
        &CLITimerTerminalReady,
        &cli,
        250
    );
    cli.lastChar = 0;
    cli.lastRxTimestamp = 0;
}

/**
 * CLIProcess()
 *     Description:
 *         Read the RX queue and process the messages into meaningful data
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void CLIProcess()
{
    while (cli.lastChar != cli.uart->rxQueue.writeCursor) {
        unsigned char nextChar = CharQueueGet(&cli.uart->rxQueue, cli.lastChar);
        if (nextChar != CLI_MSG_DELETE_CHAR) {
            UARTSendChar(cli.uart, nextChar);
        }
        if (cli.lastChar >= CHAR_QUEUE_SIZE) {
            cli.lastChar = 0;
        } else {
            cli.lastChar++;
        }
    }
    if (cli.terminalReady == 0 && SYS_DTR_STATUS == 0) {
        cli.terminalReady = 1;
        TimerResetScheduledTask(cli.terminalReadyTaskId);
    }
    if (cli.terminalReady == 2 && SYS_DTR_STATUS == 1) {
        cli.terminalReady = 0;
    }
    // Check for the backspace character
    uint16_t backspaceLegnth = CharQueueSeek(&cli.uart->rxQueue, CLI_MSG_DELETE_CHAR);
    if (backspaceLegnth > 0) {
        if (cli.lastChar < 2) {
            cli.lastChar = CHAR_QUEUE_SIZE - (3 - cli.lastChar);
        } else {
            cli.lastChar = cli.lastChar - 2;
        }
        // Remove the backspace character
        CharQueueRemoveLast(&cli.uart->rxQueue);
        // Send the "back one" character, space character and then back one again
        if (cli.uart->rxQueue.size > 0) {
            UARTSendChar(cli.uart, '\b');
            UARTSendChar(cli.uart, ' ');
            UARTSendChar(cli.uart, '\b');
        }
        // Remove the character before it
        CharQueueRemoveLast(&cli.uart->rxQueue);
    }
    uint16_t messageLength = CharQueueSeek(&cli.uart->rxQueue, CLI_MSG_END_CHAR);
    if (messageLength > 0) {
        // Send a newline to keep the CLI pretty
        UARTSendChar(cli.uart, 0x0A);
        char msg[messageLength];
        uint16_t i;
        uint8_t delimCount = 1;
        for (i = 0; i < messageLength; i++) {
            char c = CharQueueNext(&cli.uart->rxQueue);
            if (c == CLI_MSG_DELIMETER) {
                delimCount++;
            }
            if (c != CLI_MSG_END_CHAR) {
                msg[i] = c;
            } else {
                // 0x0D delimits messages, so we change it to a null
                // terminator instead
                msg[i] = '\0';
            }
        }
        uint8_t cmdSuccess = 1;
        if (messageLength > 1) {
            // Copy the message, since strtok adds a null terminator after the first
            // occurrence of the delimiter, it will not cause issues with string
            // functions
            char tmpMsg[messageLength];
            strcpy(tmpMsg, msg);
            char *msgBuf[delimCount];
            char *p = strtok(tmpMsg, " ");
            i = 0;
            while (p != NULL) {
                msgBuf[i++] = p;
                p = strtok(NULL, " ");
            }
            if (UtilsStricmp(msgBuf[0], "BOOTLOADER") == 0) {
                LogRaw("Rebooting into bootloader\r\n");
                uint32_t now = TimerGetMillis();
                // Wait 15ms before going into the bootloader
                // so our message debug goes through to the CLI
                while (TimerGetMillis() - now <= 15);
                ConfigSetBootloaderMode(0x01);
                UtilsReset();
            } else if (UtilsStricmp(msgBuf[0], "BT") == 0) {
                if (UtilsStricmp(msgBuf[1], "CONFIG") == 0) {
                    BC127SendCommand(cli.bt, "CONFIG");
                } else if (UtilsStricmp(msgBuf[1], "CVC") == 0) {
                    if (delimCount == 2) {
                        cmdSuccess = 0;
                    } else {
                        if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                            BC127SendCommand(cli.bt, "SET HFP_CONFIG=ON ON ON ON ON OFF");
                            unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
                            unsigned char micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
                            unsigned char micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
                            BC127CommandSetMicGain(cli.bt, micGain, micBias, micPreamp);
                        } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                            BC127SendCommand(cli.bt, "SET HFP_CONFIG=OFF ON ON OFF ON OFF");
                            BC127CommandWrite(cli.bt);
                        } else {
                            cmdSuccess = 0;
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "HFP") == 0) {
                    if (delimCount == 2) {
                        if (ConfigGetSetting(CONFIG_SETTING_HFP) == CONFIG_SETTING_ON) {
                            LogRaw("HFP: On\r\n");
                        } else {
                            LogRaw("HFP: Off\r\n");
                        }
                    } else {
                        if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                            ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_ON);
                            BC127CommandSetProfiles(cli.bt, 1, 1, 0, 1);
                        } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                            ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_OFF);
                            BC127CommandSetProfiles(cli.bt, 1, 1, 0, 0);
                        } else {
                            cmdSuccess = 0;
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "LICENSE") == 0) {
                    cmdSuccess = 0;
                    if (delimCount == 2) {
                        BC127CommandLicense(cli.bt, 0, 0);
                        cmdSuccess = 1;
                    }
                    if (delimCount == 3 && (
                        UtilsStricmp(msgBuf[2], "CVC") == 0 ||
                        UtilsStricmp(msgBuf[2], "APTX") == 0)
                    ) {
                        BC127CommandLicense(cli.bt, msgBuf[2], 0);
                        cmdSuccess = 1;
                    }
                    if (delimCount == 8 && (
                        UtilsStricmp(msgBuf[2], "CVC") == 0 ||
                        UtilsStricmp(msgBuf[2], "APTX") == 0)
                    ) {
                        char license[25];
                        memset(license, 0, 25);
                        snprintf(
                            license,
                            25,
                            "%s %s %s %s %s",
                            msgBuf[3],
                            msgBuf[4],
                            msgBuf[5],
                            msgBuf[6],
                            msgBuf[7]
                        );
                        BC127CommandLicense(cli.bt, msgBuf[2], license);
                        cmdSuccess = 1;
                    }
                } else if (UtilsStricmp(msgBuf[1], "MGAIN") == 0) {
                    if (delimCount == 2) {
                        unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
                        LogRaw("BT Mic Gain Set to: %02X\r\n", micGain);
                    } else {
                        unsigned char micGain = UtilsStrToHex(msgBuf[2]);
                        if (micGain < 0xC0 || micGain > 0xD6) {
                            LogRaw("Mic Gain '%02X' out of range: C0 - D6\r\n", micGain);
                        } else {
                            // Store it as a smaller value
                            micGain = micGain - 0xC0;
                            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, micGain);
                            unsigned char micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
                            unsigned char micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
                            BC127CommandSetMicGain(
                                cli.bt,
                                micGain,
                                micBias,
                                micPreamp
                            );
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "MBIAS") == 0) {
                    if (delimCount == 2) {
                        LogRaw("Set the Mic Bias Generator");
                    } else {
                        unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
                        unsigned char micPreamp = ConfigGetSetting(CONFIG_SETTING_MIC_PREAMP);
                        if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                            BC127CommandSetMicGain(cli.bt, micGain, 1, micPreamp);
                            ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_ON);
                        } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                            BC127CommandSetMicGain(cli.bt, micGain, 0, micPreamp);
                            ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_OFF);
                        } else {
                            cmdSuccess = 0;
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "MPREAMP") == 0) {
                    if (delimCount == 2) {
                        LogRaw("Set the Mic Pre-Amp");
                    } else {
                        unsigned char micGain = ConfigGetSetting(CONFIG_SETTING_MIC_GAIN);
                        unsigned char micBias = ConfigGetSetting(CONFIG_SETTING_MIC_BIAS);
                        if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                            BC127CommandSetMicGain(cli.bt, micGain, micBias, 1);
                            ConfigSetSetting(CONFIG_SETTING_MIC_PREAMP, CONFIG_SETTING_ON);
                        } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                            BC127CommandSetMicGain(cli.bt, micGain, micBias, 0);
                            ConfigSetSetting(CONFIG_SETTING_MIC_PREAMP, CONFIG_SETTING_OFF);
                        } else {
                            cmdSuccess = 0;
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "REBOOT") == 0) {
                    BC127CommandReset(cli.bt);
                } else if (UtilsStricmp(msgBuf[1], "PAIR") == 0) {
                    BC127CommandBtState(cli.bt, BC127_STATE_ON, BC127_STATE_ON);
                } else if (UtilsStricmp(msgBuf[1], "UNPAIR") == 0) {
                    BC127CommandUnpair(cli.bt);
                } else if (UtilsStricmp(msgBuf[1], "NAME") == 0) {
                    char nameBuf[33];
                    memset(nameBuf, 0, 33);
                    uint8_t wordLength = delimCount - 2;
                    uint8_t wordCounter = 2;
                    while (wordLength != 0) {
                        uint8_t wordStrLen = strlen(msgBuf[wordCounter]);
                        uint8_t nameStrLen = strlen(nameBuf);
                        if (nameStrLen + wordStrLen <= 32) {
                            uint8_t i = 0;
                            for (i = 0; i < wordStrLen; i++) {
                                nameBuf[nameStrLen + i] = msgBuf[wordCounter][i];
                            }
                        } else {
                            wordLength = 0;
                            cmdSuccess = 0;
                        }
                        wordLength--;
                        wordCounter++;
                        if (cmdSuccess != 0 && wordLength != 0) {
                            nameStrLen = strlen(nameBuf);
                            // Ensure we do not overflow the buffer
                            if (nameStrLen <= 32) {
                                // Add the space we will have taken away
                                nameBuf[nameStrLen] = ' ';
                            }
                        }
                    }
                    if (cmdSuccess != 0) {
                        BC127CommandSetModuleName(cli.bt, nameBuf);
                    }
                } else if (UtilsStricmp(msgBuf[1], "PIN") == 0) {
                    if (strlen(msgBuf[2]) == 4) {
                        BC127CommandSetPin(cli.bt, msgBuf[2]);
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "PBAP") == 0) {
                    BC127SendCommand(cli.bt, "PB_PULL 16 3 1 5 0 87");
                } else if (UtilsStricmp(msgBuf[1], "VERSION") == 0) {
                    BC127CommandVersion(cli.bt);
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "GET") == 0) {
                if (UtilsStricmp(msgBuf[1], "BYTE") == 0 && delimCount == 3) {
                    unsigned char byte = UtilsStrToHex(msgBuf[2]);
                    if (byte >= CONFIG_SETTING_START_ADDRESS &&
                        byte <= CONFIG_SETTING_END_ADDRESS
                    ) {
                        unsigned char value = ConfigGetSetting(byte);
                        LogRaw("Byte 0x%02X = 0x%02X\r\n", byte, value);
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "IBUS") == 0) {
                    IBusCommandDIAGetIdentity(cli.ibus, IBUS_DEVICE_GT);
                    IBusCommandDIAGetIdentity(cli.ibus, IBUS_DEVICE_RAD);
                } else if (UtilsStricmp(msgBuf[1], "LCM") == 0) {
                    IBusCommandDIAGetIdentity(cli.ibus, IBUS_DEVICE_LCM);
                } else if (UtilsStricmp(msgBuf[1], "ERR") == 0) {
                    // Errors
                    LogRaw("Trap Counts: \r\n");
                    LogRaw("    Oscilator Failures: %d\r\n", ConfigGetTrapCount(CONFIG_TRAP_OSC));
                    LogRaw("    Address Failures: %d\r\n", ConfigGetTrapCount(CONFIG_TRAP_ADDR));
                    LogRaw("    Stack Failures: %d\r\n", ConfigGetTrapCount(CONFIG_TRAP_STACK));
                    LogRaw("    Math Failures: %d\r\n", ConfigGetTrapCount(CONFIG_TRAP_MATH));
                    LogRaw("    NVM Failures: %d\r\n", ConfigGetTrapCount(CONFIG_TRAP_NVM));
                    LogRaw("    General Failures: %d\r\n", ConfigGetTrapCount(CONFIG_TRAP_GEN));
                    LogRaw("    Last Trap: %02x\r\n", ConfigGetTrapLast());
                    LogRaw("BC127 Boot Failures: %u\r\n", ConfigGetBC127BootFailures());
                } else if (UtilsStricmp(msgBuf[1], "UI") == 0) {
                    unsigned char uiMode = ConfigGetUIMode();
                    if (uiMode == CONFIG_UI_CD53) {
                        LogRaw("UI Mode: CD53\r\n");
                    } else if (uiMode == CONFIG_UI_BMBT) {
                        LogRaw("UI Mode: Navigation\r\n");
                    } else if (uiMode == CONFIG_UI_MID) {
                        LogRaw("UI Mode: MID\r\n");
                    } else if (uiMode == CONFIG_UI_MID_BMBT) {
                        LogRaw("UI Mode: MID / Navigation\r\n");
                    } else if (uiMode == CONFIG_UI_BUSINESS_NAV) {
                        LogRaw("UI Mode: Business Navigation\r\n");
                    } else {
                        LogRaw("UI Mode: Not set or Invalid\r\n");
                    }
                } else if (UtilsStricmp(msgBuf[1], "DAC") == 0) {
                    int8_t status;
                    unsigned char buffer;
                    status = I2CRead(0x4C, 0x5E, &buffer);
                    LogRaw("PCM5122: I2SSTAT %02X (0x5E) [%d]\r\n", buffer, status);
                    status = I2CRead(0x4C, 0x76, &buffer);
                    LogRaw("PCM5122: PWRSTAT %02X (0x76) [%d]\r\n", buffer, status);
                    LogRaw("PCM5122: Volume configured to %02X\r\n", ConfigGetSetting(CONFIG_SETTING_DAC_AUDIO_VOL));
                } else if (UtilsStricmp(msgBuf[1], "I2S") == 0) {
                    int8_t status;
                    unsigned char buffer;
                    unsigned char version2;
                    unsigned char version;
                    unsigned char rev;
                    I2CRead(0x3A, 0x00, &version2);
                    I2CRead(0x3A, 0x01, &version);
                    I2CRead(0x3A, 0x02, &rev);
                    LogRaw("WM8804: DeviceID: %02X%02X Rev: %d\r\n", version, version2, rev);
                    status = I2CRead(0x3A, 0x0C, &buffer);
                    LogRaw("WM8804: SPDSTAT %02X (0x0C) [%d]\r\n", buffer, status);
                    status = I2CRead(0x3A, 0x0B, &buffer);
                    LogRaw("WM8804: INTSTAT %02X (0x0B) [%d]\r\n", buffer, status);
                } else if (UtilsStricmp(msgBuf[1], "PWROFF") == 0) {
                    if (ConfigGetSetting(CONFIG_SETTING_AUTO_POWEROFF) == CONFIG_SETTING_ON) {
                        LogRaw("Auto-Power Off: On\r\n");
                    } else {
                        LogRaw("Auto-Power Off: Off\r\n");
                    }
                } else if (UtilsStricmp(msgBuf[1], "SELFPLAY") == 0) {
                    if (ConfigGetSetting(CONFIG_SETTING_SELF_PLAY) == CONFIG_SETTING_ON) {
                        LogRaw("Self Play: On\r\n");
                    } else {
                        LogRaw("Self Play: Off\r\n");
                    }
                } else if (UtilsStricmp(msgBuf[1], "VIN") == 0) {
                    // Get VIN
                    unsigned char currentVehicleId[5] = {};
                    ConfigGetVehicleIdentity(currentVehicleId);
                    char currentVinTwo[] = {currentVehicleId[0], currentVehicleId[1], '\0'};
                    LogRaw(
                        "Vehicle VIN: %s%d%d%d%d%d\r\n",
                        currentVinTwo,
                        (currentVehicleId[2] >> 4) & 0xF,
                        currentVehicleId[2] & 0xF,
                        (currentVehicleId[3] >> 4) & 0xF,
                        currentVehicleId[3] & 0xF,
                        currentVehicleId[4]
                    );
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "REBOOT") == 0) {
                UtilsReset();
            } else if (UtilsStricmp(msgBuf[0], "RESET") == 0) {
                if (UtilsStricmp(msgBuf[1], "TRAPS") == 0) {
                    ConfigSetTrapCount(CONFIG_TRAP_OSC, 0);
                    ConfigSetTrapCount(CONFIG_TRAP_ADDR, 0);
                    ConfigSetTrapCount(CONFIG_TRAP_STACK, 0);
                    ConfigSetTrapCount(CONFIG_TRAP_MATH, 0);
                    ConfigSetTrapCount(CONFIG_TRAP_NVM, 0);
                    ConfigSetTrapCount(CONFIG_TRAP_GEN, 0);
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "SEND") == 0) {
                if (UtilsStricmp(msgBuf[1], "IBUS") == 0) {
                    uint8_t idx = 2;
                    unsigned char message[delimCount - 4];
                    unsigned char src = 0x00;
                    unsigned char dst = 0x00;
                    size_t size = 0;
                    while (idx < delimCount) {
                        if (idx == 2) {
                            src = UtilsStrToHex(msgBuf[idx]);
                        } else if (idx == 3) {
                            // Ignore the size
                        } else if (idx == 4) {
                            dst = UtilsStrToHex(msgBuf[idx]);
                        } else if (idx != (delimCount - 1)) {
                            message[size] = UtilsStrToHex(msgBuf[idx]);
                            size++;
                        }
                        idx++;
                    }
                    if (size > 0) {
                        IBusSendCommand(cli.ibus, src, dst, message, size);
                    }
                }
            } else if (UtilsStricmp(msgBuf[0], "SET") == 0) {
                if (UtilsStricmp(msgBuf[1], "BYTE") == 0 && delimCount == 4) {
                    unsigned char byte = UtilsStrToHex(msgBuf[2]);
                    unsigned char value = UtilsStrToHex(msgBuf[3]);
                    if (byte >= CONFIG_SETTING_START_ADDRESS &&
                        byte <= CONFIG_SETTING_END_ADDRESS
                    ) {
                        ConfigSetSetting(byte, value);
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "COMFORT") == 0) {
                    if (UtilsStricmp(msgBuf[2], "BLINKERS") == 0) {
                        uint8_t blinks = UtilsStrToInt(msgBuf[3]);
                        if (blinks > 1 && blinks <= 8) {
                            ConfigSetSetting(CONFIG_SETTING_COMFORT_BLINKERS, blinks);
                        } else {
                            cmdSuccess = 0;
                        }
                    } else if (UtilsStricmp(msgBuf[2], "LOCK") == 0) {
                        if (UtilsStricmp(msgBuf[3], "10") == 0) {
                            ConfigSetComfortLock(CONFIG_SETTING_COMFORT_LOCK_10KM);
                        } else if (UtilsStricmp(msgBuf[3], "20") == 0) {
                            ConfigSetComfortLock(CONFIG_SETTING_COMFORT_LOCK_20KM);
                        } else if (UtilsStricmp(msgBuf[3], "OFF") == 0) {
                            ConfigSetComfortLock(CONFIG_SETTING_OFF);
                        } else {
                            cmdSuccess = 0;
                        }
                    } else if (UtilsStricmp(msgBuf[2], "UNLOCK") == 0) {
                        if (UtilsStricmp(msgBuf[3], "POS1") == 0) {
                            ConfigSetComfortUnlock(
                                CONFIG_SETTING_COMFORT_UNLOCK_POS_1
                            );
                        } else if (UtilsStricmp(msgBuf[3], "POS0") == 0) {
                            ConfigSetComfortUnlock(
                                CONFIG_SETTING_COMFORT_UNLOCK_POS_0
                            );
                        } else if (UtilsStricmp(msgBuf[3], "OFF") == 0) {
                            ConfigSetComfortUnlock(CONFIG_SETTING_OFF);
                        } else {
                            cmdSuccess = 0;
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "DAC") == 0) {
                    if (UtilsStricmp(msgBuf[2], "GAIN") == 0) {
                        unsigned char currentVolume = UtilsStrToHex(msgBuf[3]);
                        ConfigSetSetting(CONFIG_SETTING_DAC_AUDIO_VOL, currentVolume);
                        PCM51XXSetVolume(currentVolume);
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "DSP") == 0) {
                    if (UtilsStricmp(msgBuf[2], "INPUT") == 0) {
                        if (UtilsStricmp(msgBuf[3], "ANALOG") == 0) {
                            ConfigSetSetting(
                                CONFIG_SETTING_DSP_INPUT_SRC,
                                CONFIG_SETTING_DSP_INPUT_ANALOG
                            );
                        } else if (UtilsStricmp(msgBuf[3], "DIGITAL") == 0) {
                            ConfigSetSetting(
                                CONFIG_SETTING_DSP_INPUT_SRC,
                                CONFIG_SETTING_DSP_INPUT_SPDIF
                            );
                        } else if (UtilsStricmp(msgBuf[3], "DEFAULT") == 0) {
                            ConfigSetSetting(
                                CONFIG_SETTING_DSP_INPUT_SRC,
                                CONFIG_SETTING_OFF
                            );
                        } else {
                            cmdSuccess = 0;
                        }
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "UI") == 0) {
                    if (UtilsStricmp(msgBuf[2], "1") == 0) {
                        ConfigSetUIMode(CONFIG_UI_CD53);
                    } else if (UtilsStricmp(msgBuf[2], "2") == 0) {
                        ConfigSetUIMode(CONFIG_UI_BMBT);
                    } else if (UtilsStricmp(msgBuf[2], "3") == 0) {
                        ConfigSetUIMode(CONFIG_UI_MID);
                    } else if (UtilsStricmp(msgBuf[2], "4") == 0) {
                        ConfigSetUIMode(CONFIG_UI_MID_BMBT);
                    } else if (UtilsStricmp(msgBuf[2], "5") == 0) {
                        ConfigSetUIMode(CONFIG_UI_BUSINESS_NAV);
                    } else if (UtilsStricmp(msgBuf[2], "6") == 0) {
                        // Force static GT UI mode
                        ConfigSetUIMode(CONFIG_UI_BMBT);
                        ConfigSetNavType(IBUS_GT_MKIV_STATIC);
                    } else {
                        LogRaw("Invalid UI Mode specified\r\n");
                    }
                } else if (UtilsStricmp(msgBuf[1], "IGN") == 0) {
                    if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        IBusCommandIgnitionStatus(cli.ibus, 0x00);
                        EventTriggerCallback(
                            IBUS_EVENT_IKEIgnitionStatus,
                            0x00
                        );
                        cli.ibus->ignitionStatus = 0;
                    } else if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        unsigned char ignitionStatus = 0x01;
                        IBusCommandIgnitionStatus(cli.ibus, ignitionStatus);
                        EventTriggerCallback(
                            IBUS_EVENT_IKEIgnitionStatus,
                            (unsigned char *)&ignitionStatus
                        );
                        cli.ibus->cdChangerFunction = IBUS_CDC_FUNC_PLAYING;
                        cli.ibus->ignitionStatus = 1;
                    } else if (UtilsStricmp(msgBuf[2], "ALWAYSON") == 0) {
                        if (UtilsStricmp(msgBuf[3], "ON") == 0) {
                            ConfigSetSetting(CONFIG_SETTING_IGN_ALWAYS_ON, CONFIG_SETTING_ON);
                        } else if (UtilsStricmp(msgBuf[3], "OFF") == 0) {
                            ConfigSetSetting(CONFIG_SETTING_IGN_ALWAYS_ON, CONFIG_SETTING_OFF);
                        } else {
                            cmdSuccess = 0;
                        }
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "LOG") == 0) {
                    unsigned char system = 0xFF;
                    unsigned char value = 0xFF;
                    // Get the system
                    if (UtilsStricmp(msgBuf[2], "BT") == 0) {
                        system = CONFIG_DEVICE_LOG_BT;
                    } else if (UtilsStricmp(msgBuf[2], "IBUS") == 0) {
                        system = CONFIG_DEVICE_LOG_IBUS;
                    } else if (UtilsStricmp(msgBuf[2], "SYS") == 0) {
                        system = CONFIG_DEVICE_LOG_SYSTEM;
                    } else if (UtilsStricmp(msgBuf[2], "UI") == 0) {
                        system = CONFIG_DEVICE_LOG_UI;
                    }
                    // Get the value
                    if (UtilsStricmp(msgBuf[3], "OFF") == 0) {
                        value = 0;
                    } else if (UtilsStricmp(msgBuf[3], "ON") == 0) {
                        value = 1;
                    }
                    if (system != 0xFF && value != 0xFF) {
                        ConfigSetLog(system, value);
                    } else {
                        LogRaw("Invalid Parameters for SET LOG\r\n");
                    }
                } else if (UtilsStricmp(msgBuf[1], "PWROFF") == 0) {
                    if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        ConfigSetSetting(CONFIG_SETTING_AUTO_POWEROFF, CONFIG_SETTING_ON);
                    } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        ConfigSetSetting(CONFIG_SETTING_AUTO_POWEROFF, CONFIG_SETTING_OFF);
                    }
                } else if (UtilsStricmp(msgBuf[1], "SELFPLAY") == 0) {
                    if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        ConfigSetSetting(CONFIG_SETTING_SELF_PLAY, CONFIG_SETTING_ON);
                    } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        ConfigSetSetting(CONFIG_SETTING_SELF_PLAY, CONFIG_SETTING_OFF);
                    }
                } else if (UtilsStricmp(msgBuf[1], "TEL") == 0) {
                    if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        // Enable the amp and mute the radio
                        PAM_SHDN = 1;
                        TEL_MUTE = 1;
                    } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        // Disable the amp and unmute the radio
                        PAM_SHDN = 0;
                        TimerDelayMicroseconds(250);
                        TEL_MUTE = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "TIME") == 0) {
                    if (delimCount == 4) {
                        uint8_t hour = UtilsStrToInt(msgBuf[2]);
                        uint8_t minutes = UtilsStrToInt(msgBuf[3]);
                        IBusCommandIKESetTime(cli.ibus, hour, minutes);
                    } else {
                        cmdSuccess = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "VIN") == 0) {
                    if (UtilsStricmp(msgBuf[2], "CLEAR") == 0) {
                        unsigned char vin[] = {0x00, 0x00, 0x00, 0x00, 0x00};
                        ConfigSetVehicleIdentity(vin);
                    } else {
                        cmdSuccess = 0;
                    }
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "RESTORE") == 0) {
                BC127CommandUnpair(cli.bt);
                BC127CommandSetAudio(cli.bt, 0, 1);
                BC127CommandSetAudioAnalog(cli.bt, 3, 15, 1, "OFF");
                BC127CommandSetAudioDigital(
                    cli.bt,
                    BC127_AUDIO_SPDIF,
                    "44100",
                    "0",
                    "0"
                );
                BC127CommandSetBtVolConfig(cli.bt, 15, 100, 10, 1);
                BC127CommandSetProfiles(cli.bt, 1, 1, 1, 1);
                BC127CommandSetBtState(cli.bt, 2, 2);
                BC127CommandSetCodec(cli.bt, 1, "OFF");
                BC127CommandSetMetadata(cli.bt, 1);
                BC127CommandSetModuleName(cli.bt, "BlueBus");
                BC127SendCommand(cli.bt, "SET HFP_CONFIG=ON ON ON ON ON OFF");
                BC127CommandSetCOD(cli.bt, 300420);
                
                // Save
                BC127CommandWrite(cli.bt);
                // Reset the UI
                ConfigSetUIMode(0x00);
                ConfigSetNavType(0x00);
                // Reset the VIN
                unsigned char vin[] = {0x00, 0x00, 0x00, 0x00, 0x00};
                ConfigSetVehicleIdentity(vin);
                // Reset all settings
                uint8_t idx = CONFIG_SETTING_START_ADDRESS;
                while (idx <= CONFIG_SETTING_END_ADDRESS) {
                    ConfigSetSetting(idx, 0x00);
                    idx++;
                }
                // Settings
                // Enable Auto Power Off
                ConfigSetSetting(CONFIG_SETTING_AUTO_POWEROFF, CONFIG_SETTING_ON);
                // -10dB Gain for the DAC
                ConfigSetSetting(CONFIG_SETTING_DAC_AUDIO_VOL, 0x44);
                PCM51XXSetVolume(0x44);
                // -10dB Gain for the DAC in Telephone Mode
                ConfigSetSetting(CONFIG_SETTING_DAC_TEL_TCU_MODE_VOL, 0x44);
                ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_ON);
                ConfigSetSetting(CONFIG_SETTING_MIC_BIAS, CONFIG_SETTING_ON);
                // Set the Mic Gain to -17.5dB by default
                ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, 0x03);
            } else if (UtilsStricmp(msgBuf[0], "VERSION") == 0) {
                char version[9];
                ConfigGetFirmwareVersionString(version);
                LogRaw("BlueBus Firmware: %s\r\n", version);
                LogRaw("Serial Number: %u\r\n", ConfigGetSerialNumber());
                LogRaw("Build Date: %d/%d\r\n", ConfigGetBuildWeek(), ConfigGetBuildYear());
            } else if (UtilsStricmp(msgBuf[0], "HELP") == 0 || UtilsStricmp(msgBuf[0], "?") == 0) {
                LogRaw("Available Commands:\r\n");
                LogRaw("    BOOTLOADER - Reboot into the bootloader immediately\r\n");
                LogRaw("    BT CONFIG - Get the BC127 Configuration\r\n");
                LogRaw("    BT CVC ON/OFF - Enable or Disable Clear Voice Capture\r\n");
                LogRaw("    BT HFP ON/OFF - Enable or Disable HFP. Get the HFP Status without a param.\r\n");
                LogRaw("    BT MGAIN x - Set the Mic gain to x where x is octal C0-D6\r\n");
                LogRaw("    BT MPREAMP ON/OFF - Enable the microphone pre-amp so non-OE microphones work well\r\n");
                LogRaw("    BT PAIR - Enable pairing mode\r\n");
                LogRaw("    BT NAME <name> - Set the module name, up to 32 chars\r\n");
                LogRaw("    BT PIN <pin> - Set the module pin, up to 4 digits\r\n");
                LogRaw("    BT REBOOT - Reboot the BC127\r\n");
                LogRaw("    BT UNPAIR - Unpair all devices from the BC127\r\n");
                LogRaw("    BT VERSION - Get the BC127 Version Info\r\n");
                LogRaw("    GET DAC - Get info from the PCM5122 DAC\r\n");
                LogRaw("    GET ERR - Get the Error counter\r\n");
                LogRaw("    GET IBUS - Get debug info from the IBus\r\n");
                LogRaw("    GET UI - Get the current UI Mode\r\n");
                LogRaw("    GET I2S - Read the WM8804 INT/SPD Status registers\r\n");
                LogRaw("    GET VIN - Read the stored vehicle VIN\r\n");
                LogRaw("    REBOOT - Reboot the device\r\n");
                LogRaw("    SET COMFORT BLINKERS x - Set the comfort blinkers between 1 and 8\r\n");
                LogRaw("    SET COMFORT LOCK x - Lock the car at the given KM/h. 10, 20 or OFF\r\n");
                LogRaw("    SET COMFORT UNLOCK x - Unock the car at the given ignition position. POS0, POS1 or OFF\r\n");
                LogRaw("    SET DAC GAIN xx - Set the PCM5122 gain from 0x00 - 0xCF (higher is lower)\r\n");
                LogRaw("    SET DSP INPUT ANALOG/DIGITAL/DEFAULT - Set the CD Changer DSP input\r\n");
                LogRaw("    SET IGN ON/OFF/ALWAYSON - Send the ignition status message or configure the BlueBus to assume the ignition is always on\r\n");
                LogRaw("    SET LOG x ON/OFF - Change logging for x (BT, IBUS, SYS, UI)\r\n");
                LogRaw("    SET PWROFF ON/OFF - Enable or disable auto power off\r\n");
                LogRaw("    SET TEL ON/OFF - Enable/Disable output as the TCU\r\n");
                LogRaw("    SET TIME HH MM - Set the IKE Time\r\n");
                LogRaw("    SET UI x - Set the UI to x, where x:\r\n");
                LogRaw("        x = 1. CD53 (Business Radio)\r\n");
                LogRaw("        x = 2. BMBT (Navigation)\r\n");
                LogRaw("        x = 3. MID (Multi-Info Display)\r\n");
                LogRaw("        x = 4. BMBT / MID\r\n");
                LogRaw("        x = 5. Business Navigation\r\n");
                LogRaw("    RESTORE - Fully Reset the BlueBus and BC127 to factory defaults\r\n");
                LogRaw("    VERSION - Get the BlueBus Hardware/Software Versions\r\n");
            } else {
                cmdSuccess = 0;
            }
            if (cmdSuccess == 0) {
                LogRaw("Command not found. Try HELP or ?\r\n# ");
            } else {
                LogRaw("OK\r\n# ");
            }
        } else {
            if (((TimerGetMillis() - cli.lastRxTimestamp) / 1000) > CLI_BANNER_TIMEOUT ||
                cli.lastRxTimestamp == 0
            ) {
                char version[9];
                ConfigGetFirmwareVersionString(version);
                LogRaw("~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
                LogRaw("BlueBus Firmware: %s\r\n", version);
                LogRaw("Try HELP or ?\r\n");
                LogRaw("~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
            }
            LogRaw("# ");
        }
        cli.lastRxTimestamp = TimerGetMillis();
    }
}

/**
 * CLITimerTerminalReady()
 *     Description:
 *         Read the RX queue and process the messages into meaningful data
 *     Params:
 *         void *ctx - Pointer to the CLI context
 *     Returns:
 *         void
 */
void CLITimerTerminalReady(void *ctx)
{
    if (cli.terminalReady == 1) {
        cli.terminalReady = 2;
        char version[9];
        ConfigGetFirmwareVersionString(version);
        LogRaw("~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
        LogRaw("BlueBus Firmware: %s\r\n", version);
        LogRaw("Try HELP or ?\r\n");
        LogRaw("~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
        LogRaw("# ");
        cli.lastRxTimestamp = TimerGetMillis();
    }
}
