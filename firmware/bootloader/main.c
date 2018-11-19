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
    struct UART_t uart = UARTInit(UART_BAUD_115200);
    TimerInit();

    uint8_t BOOT_MODE = BOOT_MODE_APPLICATION;
    while (TimerGetMillis() <= BOOTLOADER_TIMEOUT || 
           BOOT_MODE == BOOT_MODE_BOOTLOADER
    ) {
        TimerUpdate();
        UARTReadData(&uart);
        if (uart.rxQueueSize > 0) {
            if ((TimerGetMillis() - uart.rxLastTimestamp) > UART_RX_QUEUE_TIMEOUT) {
                UARTResetRxQueue(&uart);
            }
            // Process Message
            struct ProtocolPacket_t packet = ProtocolProcessPacket(&uart);
            if (packet.status == PROTOCOL_PACKET_STATUS_OK) {
                // Lock the device in the bootloader since we have a good packet
                if (BOOT_MODE == 0) {
                    ON_LED_MODE = 0;
                    ON_LED = 1;
                    BOOT_MODE = 1;
                }
                switch (packet.command) {
                    case PROTOCOL_CMD_PLATFORM_REQUEST:
                        ProtocolSendStringPacket(
                            &uart,
                            (unsigned char) PROTOCOL_CMD_PLATFORM_RESPONSE,
                            (char *) BOOTLOADER_PLATFORM
                        );
                        break;
                    case PROTOCOL_CMD_VERSION_REQUEST:
                        ProtocolSendStringPacket(
                            &uart,
                            (unsigned char) PROTOCOL_CMD_VERSION_RESPONSE,
                            (char *) BOOTLOADER_VERSION
                        );
                        break;
                    case PROTOCOL_CMD_WRITE_DATA_REQUEST:
                        ProtocolFlashWrite(&uart, &packet);
                        break;
                    case PROTOCOL_CMD_START_APP_REQUEST:
                        BOOT_MODE = BOOT_MODE_APPLICATION;
                        ProtocolSendPacket(
                            &uart,
                            (unsigned char) PROTOCOL_CMD_START_APP_RESPONSE,
                            0,
                            0
                        );
                        break;
                }
            } else if (packet.status == PROTOCOL_PACKET_STATUS_BAD) {
                ProtocolSendPacket(
                    &uart,
                    (unsigned char) PROTOCOL_BAD_PACKET_RESPONSE,
                    0,
                    0
                );
            }
        }
    }
    ON_LED = 1;
    // Jump to the application
    void (*fptr)(void);
    fptr = (void (*)(void))BOOTLOADER_APPLICATION_START;
    fptr();

    return 0;
}
