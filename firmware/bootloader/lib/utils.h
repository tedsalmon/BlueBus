/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#ifndef UTILS_H
#define UTILS_H
#include <xc.h>
#include "../mappings.h"

#define UTILS_MAX_RPOR_PIN 31

/* Check if a bit is set in a byte */
#define CHECK_BIT(var, pos) ((var) & (1 <<(pos)))
/* Return a programmable output port register */
#define GET_RPOR(num) (((uint16_t *) &RPOR0) + num)

uint8_t UtilsGetBoardVersion();
void UtilsSetRPORMode(uint8_t, uint16_t);
#endif /* UTILS_H */
