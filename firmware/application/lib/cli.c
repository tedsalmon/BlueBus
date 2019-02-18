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
 *     Returns:
 *         void
 */
CLI_t CLIInit(UART_t *uart, BC127_t *bt)
{
    CLI_t cli;
    cli.uart = uart;
    cli.bt = bt;
    cli.lastChar = 0;
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
        UARTSendChar(cli->uart, CharQueueGet(&cli->uart->rxQueue, cli->lastChar));
        if (cli->lastChar >= 255) {
            cli->lastChar = 0;
        } else {
            cli->lastChar++;
        }
    }
    uint8_t messageLength = CharQueueSeek(&cli->uart->rxQueue, CLI_MSG_END_CHAR);
    if (messageLength > 0) {
        // Send a newline to keep the CLI pretty
        UARTSendChar(cli->uart, 0x0A);
        char msg[messageLength];
        uint8_t i;
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

        if (strcmp(msgBuf[0], "BOOTLOADER") == 0) {
            LogRaw("Rebooting into bootloader\r\n");
            ConfigSetBootloaderMode(0x01);
            // Nop until the message reaches the terminal
            uint16_t i;
            for (i = 0; i < 256; i++) {
                Nop();
            }
            __asm__ volatile ("reset");
        } else if (strcmp(msgBuf[0], "BTREBOOT") == 0) {
            BC127CommandReset(cli->bt);
        } else if (strcmp(msgBuf[0], "GET") == 0) {
            if (strcmp(msgBuf[1], "UI") == 0) {
                unsigned char uiMode = ConfigGetUIMode();
                if (uiMode == IBus_UI_CD53) {
                    LogRaw("UI Mode: CD53\r\n");
                } else if (uiMode == IBus_UI_BMBT) {
                    LogRaw("UI Mode: BMBT\r\n");
                } else {
                    LogRaw("UI Mode: Not set or Invalid\r\n");
                }
            }
        } else if (strcmp(msgBuf[0], "REBOOT") == 0) {
            __asm__ volatile ("reset");
        } else if (strcmp(msgBuf[0], "SET") == 0) {
            if (strcmp(msgBuf[1], "UI") == 0) {
                if (strcmp(msgBuf[2], "1") == 0) {
                    ConfigSetUIMode(IBus_UI_CD53);
                    LogRaw("UI Mode: CD53\r\n");
                } else if (strcmp(msgBuf[2], "2") == 0) {
                    ConfigSetUIMode(IBus_UI_BMBT);
                    LogRaw("UI Mode: BMBT\r\n");
                } else {
                    LogError("Invalid UI Mode specified");
                }
            } else if(strcmp(msgBuf[1], "BTAUD_DIG") == 0) {
                if (delimCount < 5) {
                    LogRaw("Not enough parameters");
                } else {
                    BC127CommandSetAudioDigital(
                        cli->bt,
                        msgBuf[2],
                        msgBuf[3],
                        msgBuf[4],
                        msgBuf[5]
                    );
                }
            } else if (strcmp(msgBuf[1], "BTAUTOCONN") == 0) {
                if (strcmp(msgBuf[2], "0") == 0) {
                    BC127CommandSetAutoConnect(cli->bt, 0);
                } else if (strcmp(msgBuf[2], "1") == 0) {
                    BC127CommandSetAutoConnect(cli->bt, 1);
                } else {
                    LogRaw("Invalid AUTOCONN parameter");
                }
            }
        } else if (strcmp(msgBuf[0], "HELP") == 0) {
            LogRaw("BlueBus Firmware version: 1.0.1\r\n");
            LogRaw("Available Commands:\r\n");
            LogRaw("    BOOTLOADER - Reboot into the bootloader immediately\r\n");
            LogRaw("    BTREBOOT - Reboot the BC127\r\n");
            LogRaw("    GET UI - Get the current UI Mode\r\n");
            LogRaw("    REBOOT - Reboot the device\r\n");
            LogRaw("    SET BTAUTOCONN <0 for off 1 for on>\r\n");
            LogRaw("    SET BTAUD_DIG <format> <rate> <param1> <param2>\r\n");
            LogRaw("    SET UI x - Set the UI to x, ");
            LogRaw("where 1 is CD53 and 2 is BMBT\r\n");
        } else {
            LogError("Command Unknown. Try HELP");
        }
    }
}
