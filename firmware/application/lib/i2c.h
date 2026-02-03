/*
 * File:   i2c.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the I2C Bus. Currently only I2C3 is implemented
 */
#ifndef I2C_H
#define I2C_H
#include <xc.h>
#include "../mappings.h"
#include "log.h"
#include "sfr_setters.h"
#include "timer.h"

#define I2C_BRG_100 0x26
#define I2C_BRG_400 0x12

#define I2C_ACK 0
#define I2C_NACK 1
#define I2C_ERR_HARDWARE -1
#define I2C_ERR_SCL_LOW -2
#define I2C_ERR_SDA_LOW -3
#define I2C_ERR_BCL -4
#define I2C_ERR_IWCOL -5
#define I2C_ERR_NAK -6
#define I2C_ERR_TBF -7
#define I2C_ERR_OVERFLOW -9
#define I2C_ERR_RCV_TIMEOUT -10
#define I2C_ERR_BUS_DIRTY -100
#define I2C_ERR_TIMEOUT_HW -101
#define I2C_ERR_COMM_FAIL -102
#define I2C_ERR_BAD_ADDR -103
#define I2C_STATUS_OK 0
#define I2C_STATUS_ERR 1
#define I2C_SCL_TIMEOUT 2000
#define I2C_SCL_WRITE_TIMEOUT 8000

void I2CInit();
void I2CClearErrors();
int8_t I2CPoll(unsigned char);
int8_t I2CRead(unsigned char, unsigned char, unsigned char *);
int8_t I2CRecoverBus();
int8_t I2CRestart();
int8_t I2CStart();
int8_t I2CStop();
int8_t I2CWrite(unsigned char, unsigned char, unsigned char);
#endif /* I2C_H */
