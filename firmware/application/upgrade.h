/*
 * File: upgrade.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implement Upgrade Tasks
 */
#ifndef UPGRADE_H
#define UPGRADE_H
#include "lib/bc127.h"
#include "lib/config.h"
#include "lib/ibus.h"
#include "lib/log.h"
#include "lib/pcm51xx.h"
#define UPGRADE_MINOR_IS_NEWER 0
#define UPGRADE_MINOR_IS_SAME 1
#define UPGRADE_MINOR_IS_OLDER 2
uint8_t UpgradeProcess(BC127_t *, IBus_t *);
uint8_t UpgradeVersionCompare(
    unsigned char,
    unsigned char,
    unsigned char,
    unsigned char,
    unsigned char,
    unsigned char
);
#endif /* UPGRADE_H */
