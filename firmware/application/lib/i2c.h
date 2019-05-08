/*
 * File:   i2c.h
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Logging mechanisms that we can use throughout the project
 */
#ifndef I2C_H
#define I2C_H
#include <xc.h>
#include "../mappings.h"
#include "sfr_setters.h"
#include "timer.h"

#define I2C_BRG_100 0x4E
#define I2C_BRG_400 0x12
//Return values.  Errors must be negative.
#define I2C_ACK 0
#define I2C_NACK 1
#define I2C_OK 0 //Success
#define I2C_ACK 0 //ACK, same as OK
#define I2C_Err_Hardware -1 //Hardware error with I2C bus, inspect PCB
#define I2C_Err_SCL_low -2 //Clock line stuck low - HW problem on PCB, or I2C slave holding line low
#define I2C_Err_SDA_low -3 //Data line stuck low
#define I2C_Err_BCL -4 //Bus collision detected during master operation. Reset I2C interface.
#define I2C_Err_IWCOL -5 //Attempted to write to I2CxTRN while byte still qued for sending
#define I2C_Err_NAK -6 //Slave refused/ignored byte sent by master - could re-send byte
#define I2C_Err_TBF -7 //Transmit buffer full - a byte already qued for sending. Indicates programming error.
#define I2C_Err_Overflow -9 //Received new byte without processing previous one
#define I2C_Err_RcvTimeout -10 //Timeout while waiting for byte from slave
#define I2C_Err_BusDirty -100 //Need to reset I2C bus before high level routines will work
#define I2C_Err_TimeoutHW -101 //Timeout, unknown reason
#define I2C_Err_CommFail -102 //General communications failure
#define I2C_Err_BadAddr -103 //Bad device address or device stopped responding
#define I2C_STATUS_OK 0
#define I2C_STATUS_ERR 1

void I2CInit();
void I2CClearErrors();
int8_t I2CPoll(unsigned char);
int8_t I2CRead(unsigned char, unsigned char, unsigned char *);
uint16_t I2CReadByte(unsigned char);
int8_t I2CRecoverBus();
int8_t I2CRestart();
int8_t I2CStart();
int8_t I2CStop();
int8_t I2CWrite(unsigned char, unsigned char, unsigned char);
int8_t I2CWriteByte(char data);
#endif /* I2C_H */
