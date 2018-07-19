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
void removeSubstring(char *, const char *);
uint8_t strToInt(char *);
#endif /* UTILS_H */
