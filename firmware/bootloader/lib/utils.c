/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#include "utils.h"

static int8_t BOARD_VERSION = -1;

/**
 * UtilsGetBoardVersion()
 *     Description:
 *         Get the board byte based on the I/O pin configuration
 *     Params:
 *         None
 *     Returns:
 *         uint8_t The identified board type
 */
uint8_t UtilsGetBoardVersion()
{
    if (BOARD_VERSION == -1) {
        if (BOARD_VERSION_STATUS == BOARD_VERSION_ONE) {
            BOARD_VERSION = BOARD_VERSION_ONE;
        } else {
            BOARD_VERSION = BOARD_VERSION_TWO;
        }
    }
    return BOARD_VERSION;
}

/**
 * UtilsSetRPORMode()
 *     Description:
 *         Set the mode of a programmable output pin
 *     Params:
 *         uint8_t pin - The pin to set
 *         uint8_t mode - The mode to set the given pin to
 *     Returns:
 *         void
 */
void UtilsSetRPORMode(uint8_t pin, uint16_t mode)
{
    // Prevent writing to memory that does not exist
    if (pin > UTILS_MAX_RPOR_PIN) {
        return;
    }
    uint8_t regNum = 0;
    if (pin > 1) {
        regNum = pin / 2;
    }
    volatile uint16_t *PROG_PIN = GET_RPOR(regNum);
    if ((pin % 2) == 0) {
        uint16_t msb = *PROG_PIN >> 8;
        // Set the least significant bits for the even pin number
        *PROG_PIN = (msb << 8) + mode;
    } else {
        uint16_t lsb = *PROG_PIN & 0xFF;
        // Set the least significant bits of the register for the odd pin number
        *PROG_PIN = (mode << 8) + lsb;
    }
}
