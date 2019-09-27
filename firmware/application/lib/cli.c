/*
 * File:   cli.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement a CLI to pass commands to the device
 */
#include "cli.h"

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
CLI_t CLIInit(UART_t *uart, BC127_t *bt, IBus_t *ibus)
{
    CLI_t cli;
    cli.uart = uart;
    cli.bt = bt;
    cli.ibus = ibus;
    cli.lastChar = 0;
    cli.lastRxTimestamp = 0;
    return cli;
}

/**
 * CLIProcess()
 *     Description:
 *         Read the RX queue and process the messages into meaningful data
 *     Params:
 *         UART_t *uart - A pointer to the UART module object
 *     Returns:
 *         void
 */
void CLIProcess(CLI_t *cli)
{
    while (cli->lastChar != cli->uart->rxQueue.writeCursor) {
        unsigned char nextChar = CharQueueGet(&cli->uart->rxQueue, cli->lastChar);
        UARTSendChar(cli->uart, nextChar);
        if (cli->lastChar >= (CHAR_QUEUE_SIZE - 1)) {
            cli->lastChar = 0;
        } else {
            cli->lastChar++;
        }
    }
    // Check for the backspace character
    uint16_t backspaceLegnth = CharQueueSeek(&cli->uart->rxQueue, CLI_MSG_DELETE_CHAR);
    if (backspaceLegnth > 0) {
        if (cli->lastChar < 2) {
            cli->lastChar = CHAR_QUEUE_SIZE - (3 - cli->lastChar);
        } else {
            cli->lastChar = cli->lastChar - 2;
        }
        // Remove the backspace character
        CharQueueRemoveLast(&cli->uart->rxQueue);
        // Send the "back one" character, space character and then back one again
        if (cli->uart->rxQueue.size > 0) {
            UARTSendChar(cli->uart, '\b');
            UARTSendChar(cli->uart, ' ');
            UARTSendChar(cli->uart, '\b');
        }
        // Remove the character before it
        CharQueueRemoveLast(&cli->uart->rxQueue);
    }
    uint16_t messageLength = CharQueueSeek(&cli->uart->rxQueue, CLI_MSG_END_CHAR);
    if (messageLength > 0) {
        // Send a newline to keep the CLI pretty
        UARTSendChar(cli->uart, 0x0A);
        char msg[messageLength];
        uint16_t i;
        uint8_t delimCount = 1;
        for (i = 0; i < messageLength; i++) {
            char c = CharQueueNext(&cli->uart->rxQueue);
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
                ConfigSetBootloaderMode(0x01);
                __asm__ volatile ("reset");
            } else if (UtilsStricmp(msgBuf[0], "BT") == 0) {
                if (UtilsStricmp(msgBuf[1], "CONFIG") == 0) {
                    BC127SendCommand(cli->bt, "CONFIG");
                } else if (UtilsStricmp(msgBuf[1], "CVC") == 0) {
                    if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        BC127SendCommand(cli->bt, "SET HFP_CONFIG=ON ON ON ON OFF OFF");
                        BC127CommandWrite(cli->bt);
                    } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        BC127SendCommand(cli->bt, "SET HFP_CONFIG=OFF ON ON OFF OFF OFF");
                        BC127CommandWrite(cli->bt);
                    } else if (UtilsStricmp(msgBuf[2], "NB") == 0) {
                        BC127CommandCVC(cli->bt, "NB", 0, 0);
                    } else if (UtilsStricmp(msgBuf[2], "WB") == 0) {
                        BC127CommandCVC(cli->bt, "WB", 0, 0);
                    }
                } else if (UtilsStricmp(msgBuf[1], "INIT") == 0) {
                    BC127CommandSetAudio(cli->bt, 0, 1);
                    BC127CommandSetAudioAnalog(cli->bt, "11", "15", "1", "OFF");
                    BC127CommandSetAudioDigital(
                        cli->bt,
                        BC127_AUDIO_SPDIF,
                        "44100",
                        "0",
                        "0"
                    );
                    BC127CommandSetBtState(cli->bt, 2, 2);
                    BC127CommandSetCodec(cli->bt, 1, "OFF");
                    BC127CommandSetMetadata(cli->bt, 1);
                    BC127CommandSetModuleName(cli->bt, "BlueBus");
                    BC127CommandSetUART(cli->bt, 115200, "OFF", 0);
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
                            BC127CommandSetProfiles(cli->bt, 1, 1, 0, 1);
                        } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                            ConfigSetSetting(CONFIG_SETTING_HFP, CONFIG_SETTING_OFF);
                            BC127CommandSetProfiles(cli->bt, 1, 1, 0, 0);
                        } else {
                            cmdSuccess = 0;
                        }
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
                            ConfigSetSetting(CONFIG_SETTING_MIC_GAIN, micGain);
                            BC127CommandSetMicGain(cli->bt, micGain);
                        }
                    }
                } else if (UtilsStricmp(msgBuf[1], "REBOOT") == 0) {
                    BC127CommandReset(cli->bt);
                } else if (UtilsStricmp(msgBuf[1], "PAIR") == 0) {
                    BC127CommandBtState(cli->bt, BC127_STATE_ON, BC127_STATE_ON);
                } else if (UtilsStricmp(msgBuf[1], "UNPAIR") == 0) {
                    BC127CommandUnpair(cli->bt);
                } else if (UtilsStricmp(msgBuf[1], "VERSION") == 0) {
                    BC127CommandVersion(cli->bt);
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "GET") == 0) {
                if (UtilsStricmp(msgBuf[1], "IBUS") == 0) {
                    IBusCommandDIAGetIdentity(cli->ibus, IBUS_DEVICE_GT);
                    IBusCommandDIAGetIdentity(cli->ibus, IBUS_DEVICE_RAD);
                } else if (UtilsStricmp(msgBuf[1], "LCM") == 0) {
                    IBusCommandDIAGetIdentity(cli->ibus, IBUS_DEVICE_LCM);
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
                } else if (UtilsStricmp(msgBuf[1], "UI") == 0) {
                    unsigned char uiMode = ConfigGetUIMode();
                    if (uiMode == IBus_UI_CD53) {
                        LogRaw("UI Mode: CD53\r\n");
                    } else if (uiMode == IBus_UI_BMBT) {
                        LogRaw("UI Mode: Navigation\r\n");
                    } else if (uiMode == IBus_UI_MID) {
                        LogRaw("UI Mode: MID\r\n");
                    } else if (uiMode == IBus_UI_MID_BMBT) {
                        LogRaw("UI Mode: MID / Navigation\r\n");
                    } else if (uiMode == IBus_UI_BUSINESS_NAV) {
                        LogRaw("UI Mode: Business Navigation\r\n");
                    } else {
                        LogRaw("UI Mode: Not set or Invalid\r\n");
                    }
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
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "REBOOT") == 0) {
                __asm__ volatile ("reset");
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
            } else if (UtilsStricmp(msgBuf[0], "SET") == 0) {
                if (UtilsStricmp(msgBuf[1], "UI") == 0) {
                    if (UtilsStricmp(msgBuf[2], "1") == 0) {
                        ConfigSetUIMode(IBus_UI_CD53);
                        LogRaw("UI Mode: CD53\r\n");
                    } else if (UtilsStricmp(msgBuf[2], "2") == 0) {
                        ConfigSetUIMode(IBus_UI_BMBT);
                        LogRaw("UI Mode: Navigation\r\n");
                    } else if (UtilsStricmp(msgBuf[2], "3") == 0) {
                        ConfigSetUIMode(IBus_UI_MID);
                        LogRaw("UI Mode: MID\r\n");
                    } else if (UtilsStricmp(msgBuf[2], "4") == 0) {
                        ConfigSetUIMode(IBus_UI_MID_BMBT);
                        LogRaw("UI Mode: MID / Navigation\r\n");
                    } else if (UtilsStricmp(msgBuf[2], "5") == 0) {
                        ConfigSetUIMode(IBus_UI_BUSINESS_NAV);
                        LogRaw("UI Mode: Business Navigation\r\n");
                    } else {
                        LogRaw("Invalid UI Mode specified\r\n");
                    }
                } else if(UtilsStricmp(msgBuf[1], "IGN") == 0) {
                    if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        IBusCommandIgnitionStatus(cli->ibus, 0x00);
                        cli->ibus->ignitionStatus = 0;
                        EventTriggerCallback(IBusEvent_IgnitionStatus, 0x00);
                    } else if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        IBusCommandIgnitionStatus(cli->ibus, 0x01);
                        cli->ibus->ignitionStatus = 1;
                        EventTriggerCallback(IBusEvent_IgnitionStatus, 0x00);
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
                } else if (UtilsStricmp(msgBuf[1], "TEL") == 0) {
                    if (UtilsStricmp(msgBuf[2], "ON") == 0) {
                        // Enable the amp and mute the radio
                        PAM_SHDN = 1;
                        TEL_MUTE = 1;
                    } else if (UtilsStricmp(msgBuf[2], "OFF") == 0) {
                        // Disable the amp and unmute the radio
                        PAM_SHDN = 0;
                        TEL_MUTE = 0;
                    }
                } else if (UtilsStricmp(msgBuf[1], "PWROFF") == 0) {
                    unsigned char timeout = (unsigned char) UtilsStrToInt(msgBuf[2]);
                    ConfigSetPoweroffTimeout(timeout);
                } else {
                    cmdSuccess = 0;
                }
            } else if (UtilsStricmp(msgBuf[0], "VERSION") == 0) {
                LogRaw("BlueBus\r\nFirmware Version: 1.0.9.4\r\nHardware Revision: C\r\n");
            } else if (UtilsStricmp(msgBuf[0], "HELP") == 0 || UtilsStricmp(msgBuf[0], "?") == 0) {
                LogRaw("Available Commands:\r\n");
                LogRaw("    BOOTLOADER - Reboot into the bootloader immediately\r\n");
                LogRaw("    BT CONFIG - Get the BC127 Configuration\r\n");
                LogRaw("    BT CVC ON/OFF - Enable or Disable CVC.\r\n");
                LogRaw("    BT HFP ON/OFF - Enable or Disable HFP. Get the HFP Status without a param.\r\n");
                LogRaw("    BT MGAIN x - Set the Mic gain to x where x is octal C0-D6\r\n");
                LogRaw("    BT PAIR - Enable pairing mode\r\n");
                LogRaw("    BT REBOOT - Reboot the BC127\r\n");
                LogRaw("    BT UNPAIR - Unpair all devices from the BC127\r\n");
                LogRaw("    BT VERSION - Get the BC127 Version Info\r\n");
                LogRaw("    GET ERR - Get the Error counter\r\n");
                LogRaw("    GET IBUS - Get debug info from the IBus\r\n");
                LogRaw("    GET UI - Get the current UI Mode\r\n");
                LogRaw("    GET I2S - Read the WM8804 INT/SPD Status registers\r\n");
                LogRaw("    REBOOT - Reboot the device\r\n");
                LogRaw("    SET IGN ON/OFF - Send the ignition status message [DEBUG]\r\n");
                LogRaw("    SET LOG x ON/OFF - Change logging for x (BT, IBUS, SYS, UI)\r\n");
                LogRaw("    SET PWROFF x - Set the time in minutes that we should wait before powering off\r\n");
                LogRaw("    SET TEL ON/OFF - Enable/Disable output as the TCU\r\n");
                LogRaw("    SET UI x - Set the UI to x, where x:\r\n");
                LogRaw("        x = 1. CD53 (Business Radio)\r\n");
                LogRaw("        x = 2. BMBT (Navigation)\r\n");
                LogRaw("        x = 3. MID (Multi-Info Display)\r\n");
                LogRaw("        x = 4. BMBT / MID\r\n");
                LogRaw("        x = 5. Business Navigation\r\n");
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
            if (((TimerGetMillis() - cli->lastRxTimestamp) / 1000) > CLI_BANNER_TIMEOUT ||
                cli->lastRxTimestamp == 0
            ) {
                LogRaw("~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
                LogRaw("BlueBus Firmware: 1.0.9.4\r\n");
                LogRaw("Try HELP or ?\r\n");
                LogRaw("~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
            }
            LogRaw("# ");

        }
        cli->lastRxTimestamp = TimerGetMillis();
    }
}
