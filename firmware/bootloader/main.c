/*
 * File: main.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     The main code for our PIC24FJ bootloader
 */
#include <xc.h>
#include "sysconfig.h"
#include "mappings.h"
#include "lib/eeprom.h"
#include "lib/protocol.h"
#include "lib/timer.h"
#include "lib/uart.h"

int main(void)
{
    // Set all used ports to digital mode
    ANSB = 0;
    ANSD = 0;
    ANSE = 0;
    ANSG = 0;

    // Set the UART mode to MCU by default
    // UART_SEL_BT_MODE = UART_SEL_MODE_DISABLE;
    // UART_SEL_MCU_MODE = UART_SEL_MODE_ENABLE;
    // Set the LED mode
    ON_LED_MODE = 0;
    struct UART_t systemUart = UARTInit(UART_BAUD_115200);
    TimerInit();
    EEPROMInit();

    uint8_t BOOT_MODE = BOOT_MODE_APPLICATION;
    unsigned char configuredBootmode = EEPROMReadByte(CONFIG_BOOTLOADER_MODE);
    if (configuredBootmode != 0x00) {
        BOOT_MODE = BOOT_MODE_BOOTLOADER;
        EEPROMWriteByte(CONFIG_BOOTLOADER_MODE, 0x00);
        ON_LED = 1;
    }
    while ((TimerGetMillis() <= BOOTLOADER_TIMEOUT || 
           BOOT_MODE == BOOT_MODE_BOOTLOADER) &&
           BOOT_MODE != BOOT_MODE_NOW
    ) {
        TimerUpdate();
        UARTReadData(&systemUart);
        if (systemUart.rxQueueSize > 0) {
            if ((TimerGetMillis() - systemUart.rxLastTimestamp) > UART_RX_QUEUE_TIMEOUT) {
                UARTResetRxQueue(&systemUart);
            }
            // Process Message
            struct ProtocolPacket_t packet = ProtocolProcessPacket(&systemUart);
            if (packet.status == PROTOCOL_PACKET_STATUS_OK) {
                // Lock the device in the bootloader since we have a good packet
                if (BOOT_MODE == 0) {
                    ON_LED = 1;
                    BOOT_MODE = BOOT_MODE_BOOTLOADER;
                }
                switch (packet.command) {
                    case PROTOCOL_CMD_PLATFORM_REQUEST:
                        ProtocolSendStringPacket(
                            &systemUart,
                            (unsigned char) PROTOCOL_CMD_PLATFORM_RESPONSE,
                            (char *) BOOTLOADER_PLATFORM
                        );
                        break;
                    case PROTOCOL_CMD_WRITE_DATA_REQUEST:
                        ProtocolFlashWrite(&systemUart, &packet);
                        break;
                    case PROTOCOL_CMD_BC127_MODE_REQUEST:
                        ProtocolSendPacket(
                            &systemUart,
                            (unsigned char) PROTOCOL_CMD_BC127_MODE_RESPONSE,
                            0,
                            0
                        );
                        ProtocolBC127Mode();
                        break;
                    case PROTOCOL_CMD_START_APP_REQUEST:
                        ProtocolSendPacket(
                            &systemUart,
                            (unsigned char) PROTOCOL_CMD_START_APP_RESPONSE,
                            0,
                            0
                        );
                        // Nop() So the packet makes it to the receiver
                        uint16_t i = 0;
                        while (i < NOP_COUNT) {
                            Nop();
                            i++;
                        }
                        BOOT_MODE = BOOT_MODE_NOW;
                        break;
                    case PROTOCOL_CMD_WRITE_SN_REQUEST:
                        ProtocolWriteSerialNumber(&systemUart, &packet);
                        break;
                }
            } else if (packet.status == PROTOCOL_PACKET_STATUS_BAD) {
                ProtocolSendPacket(
                    &systemUart,
                    (unsigned char) PROTOCOL_BAD_PACKET_RESPONSE,
                    0,
                    0
                );
            }
        }
    }

    // Close the UART module so the application can utilize it
    UARTDestroy(&systemUart);
    // Close the EEPROM (SPI module) so that the application can utilize it
    EEPROMDestroy();
    ON_LED = 0;
    // Call the application code
    void (*appptr)(void);
    appptr = (void (*)(void))BOOTLOADER_APPLICATION_START;
    appptr();

    return 0;
}
