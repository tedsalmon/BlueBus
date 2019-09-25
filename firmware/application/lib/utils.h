/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#ifndef UTILS_H
#define UTILS_H
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xc.h>
#define UTILS_DISPLAY_TEXT_SIZE 255
/* Check if a bit is set in a byte */
#define CHECK_BIT(var, pos) ((var) & (1 <<(pos)))
/* Return a programmable output port register */
#define GET_RPOR(num) (((uint16_t *) &RPOR0) + num)
/*
 * UtilsAbstractDisplayValue_t
 *  This is a struct to hold text values to be displayed
 *  text: The text to display
 *  index: A variable to track what the last displayed index of text was
 *  length: The length of the text
 *  status: 0 for inactive and 1 for active
 *  timeout: The amount of iterations to display the text for. -1 is indefinite
 */
typedef struct UtilsAbstractDisplayValue_t {
    char text[UTILS_DISPLAY_TEXT_SIZE];
    uint8_t index;
    uint8_t length;
    uint8_t status;
    int8_t timeout;
} UtilsAbstractDisplayValue_t;
UtilsAbstractDisplayValue_t UtilsDisplayValueInit(char *, uint8_t);
void UtilsRemoveNonAscii(char *, const char *);
void UtilsRemoveSubstring(char *, const char *);
void UtilsSetRPORMode(uint8_t, uint16_t);
unsigned char UtilsStrToHex(char *);
uint8_t UtilsStrToInt(char *);
int8_t UtilsStricmp(const char *, const char *);
#endif /* UTILS_H */
