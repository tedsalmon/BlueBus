/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#include "utils.h"

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
