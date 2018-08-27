/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xc.h>
/* Return a programmable output port register */
#define GET_RPOR(num) (((uint16_t *) &RPOR0) + num)
void removeNonAscii(char *, const char *);
void removeSubstring(char *, const char *);
void setRPORMode(uint8_t, uint16_t);
uint8_t strToInt(char *);
#endif /* UTILS_H */
