/*
 * File: main.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     The main code for our PIC24FJ bootloader
 */
#include <xc.h>
#include "config.h"
#include "mappings.h"
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
    struct UART_t systemUart = UARTInit(
        SYSTEM_UART_MODULE,
        SYSTEM_UART_RX_PIN,
        SYSTEM_UART_TX_PIN,
        UART_BAUD_115200
    );
    struct UART_t btUart = UARTInit(
        BC127_UART_MODULE,
        BC127_UART_RX_PIN,
        BC127_UART_TX_PIN,
        UART_BAUD_115200
    );
    TimerInit();

    uint8_t BOOT_MODE = BOOT_MODE_APPLICATION;
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
                    ON_LED_MODE = 0;
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
                    case PROTOCOL_CMD_VERSION_REQUEST:
                        ProtocolSendStringPacket(
                            &systemUart,
                            (unsigned char) PROTOCOL_CMD_VERSION_RESPONSE,
                            (char *) BOOTLOADER_VERSION
                        );
                        break;
                    case PROTOCOL_CMD_WRITE_DATA_REQUEST:
                        ProtocolFlashWrite(&systemUart, &packet);
                        break;
                    case PROTOCOL_CMD_BC127_PROXY_REQUEST:
                        ProtocolSendPacket(
                            &systemUart,
                            (unsigned char) PROTOCOL_CMD_BC127_PROXY_RESPONSE,
                            0,
                            0
                        );
                        ProtocolBC127Proxy(&systemUart, &btUart);
                        break;
                    case PROTOCOL_CMD_START_APP_REQUEST:
                        BOOT_MODE = BOOT_MODE_NOW;
                        ProtocolSendPacket(
                            &systemUart,
                            (unsigned char) PROTOCOL_CMD_START_APP_RESPONSE,
                            0,
                            0
                        );
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
    // Close the UART modules so the application can utilize them
    UARTDestroy(&systemUart);
    UARTDestroy(&btUart);
    ON_LED = 0;
    // Call the application code
    void (*appptr)(void);
    appptr = (void (*)(void))BOOTLOADER_APPLICATION_START;
    appptr();

    return 0;
}
