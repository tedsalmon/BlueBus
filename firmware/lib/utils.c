/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#include "utils.h"

/**
 * removeNonAscii()
 *     Description:
 *         Ignore non-ASCII characters from input and put the rest into string
 *     Params:
 *         char *string - The subject
 *         const char *input - The string to copy from
 *     Returns:
 *         void
 */
void removeNonAscii(char *string, const char *input)
{
    uint16_t idx;
    uint16_t strIdx = 0;
    for (idx = 0; idx < strlen(input); idx++) {
        char c = input[idx];
        if (c >= 0x20 && c <= 0x7E) {
            string[strIdx] = c;
            strIdx++;
        }
    }
    string[strIdx] = '\0';
}

/**
 * removeSubstring()
 *     Description:
 *         Remove the given substring from the given subject
 *     Params:
 *         char *string - The subject
 *         const char *trash - The substring to remove
 *     Returns:
 *         void
 */
void removeSubstring(char *string, const char *trash)
{
    uint16_t removeLength = strlen(trash);
    while((string = strstr(string, trash) )){
        memmove(string, string + removeLength, 1 + strlen(string + removeLength));
    }
}

/**
 * strToInt()
 *     Description:
 *         Convert a string to an integer
 *     Params:
 *         char *string - The subject
 *     Returns:
 *         uint8_t The Unsigned 8-bit integer representation
 */
uint8_t strToInt(char *string)
{
    char *ptr;
    return (uint8_t) strtol(string, &ptr, 10);
}
