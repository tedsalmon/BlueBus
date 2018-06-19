#include <stdlib.h>
#include "char_queue.h"
#include "uart.h"

UART_t *UARTInit(uint8_t rxPin, uint8_t txPin, uint8_t baud)
{
    UART_t *uart = malloc(sizeof(UART_t));
    if (uart != NULL) {
        uart->messageQueue = CharQueueInit();
        // Assigne Function Pointers
        uart->destroy = &UARTDestroy;
        uart->sendData = &UARTSendData;
    }
    return uart;
}

void UARTDestroy(struct UART_t *uart)
{
    uart->messageQueue->destroy(uart->messageQueue);
    free(uart);
}

void UARTSendData(struct UART_t *uart, unsigned char *data)
{
    unsigned char c;
    while ((c = *data++)) {
        // Place it in the TX Register
    }
}