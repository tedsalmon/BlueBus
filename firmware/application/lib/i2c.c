/*
 * File:   i2c.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the I2C Bus. Currently only I2C3 is implemented
 */
#include "i2c.h"
static uint8_t I2CStatus;

/**
 * I2CInit()
 *     Description:
 *         Initialize the I2C connection
 *     Params:
 *      void
 *     Returns:
 *         void
 */
void I2CInit()
{
    I2C3_SDA_MODE = 1;
    I2C3_SCL_MODE = 1;
    I2C3_SDA = 0;
    I2C3_SCL = 0;
    I2C3CONL = 0;
    I2C3CONLbits.I2CEN = 0;
    I2C3BRG = I2C_BRG_400;
    // Enable Slew Mode
    I2C3CONLbits.DISSLW = 0;
    SetI2CMAEV(2, 0);
    I2C3CONLbits.I2CEN = 1;
    // Set the ERR flag so we reset the bus state
    I2CStatus = I2C_STATUS_ERR;
}

/**
 * I2CReadByte()
 *     Description:
 *         Read a single byte from an I2C slave. The Slave should already
 *         be in an I2C read-ready state. Note that the Slave clock streching
 *         can make this fail.
 *     Params:
 *         unsigned char ackFlag - Whether or not to send a NACK
 *     Returns:
 *         uint16_t The byte we read
 */
static uint16_t I2CReadByte(unsigned char ackFlag)
{
    if (ackFlag == I2C_NACK) {
        I2C3CONLbits.ACKDT = 1;
    } else {
        I2C3CONLbits.ACKDT = 0;
    }
    // Start receive
    I2C3CONLbits.RCEN = 1;
    while (!I2C3STATbits.RBF) {

    }
    // Set ACK enabled so the slave knows it can send the data
    I2C3CONLbits.ACKEN = 1;
    uint16_t cycles = 0;
    while (I2C3CONLbits.ACKEN) {
        if (cycles > I2C_SCL_TIMEOUT) {
            // SCL is stuck low and RCEN cannot be cleared, so we need to reset
            return I2C_ERR_SCLLow;
        }
        cycles++;
    }
    if (I2C3STATbits.I2COV) {
        I2C3STATbits.I2COV = 0;
        return I2C_ERR_Overflow;
    }
    return I2C3RCV;
}

/**
 * I2CWriteByte()
 *     Description:
 *         Write a byte to the bus
 *     Params:
 *         unsigned char data - The data to write
 *     Returns:
 *         int8_t The status
 */
static int8_t I2CWriteByte(unsigned char data)
{
    // Check to see if there is a write pending
    if (I2C3STATbits.TBF) {
        return I2C_ERR_TBF;
    }
    I2C3TRN = data;
    // Ensure that the byte is written within a reasonable time frame
    uint16_t cycles = 0;
    while (I2C3STATbits.TRSTAT) {
        if (cycles > I2C_SCL_WRITE_TIMEOUT) {
            return I2C_ERR_SCLLow;
        }
        cycles++;
    }
    // Check for a Bus Collision
    if (I2C3STATbits.BCL) {
        I2C3STATbits.BCL = 0;
        return I2C_ERR_BCL;
    }
    // Return the slave response
    if (I2C3STATbits.ACKSTAT == 1) {
        return I2C_ERR_NAK;
    }
    return I2C_ACK;
}

/**
 * I2CClearErrors()
 *     Description:
 *         Clear the receive enable, the Bus and Write collision flags
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void I2CClearErrors()
{
    I2C3CONLbits.RCEN = 0;
    I2C3STATbits.IWCOL = 0;
    I2C3STATbits.BCL = 0;
}

/**
 * I2CPoll()
 *     Description:
 *         Poll a given I2C device to check if it is alive.
 *         We perform error recovery here, too.
 *     Params:
 *         unsigned char deviceAdress - The device address to poll
 *     Returns:
 *         int8_t The I2C status
 */
int8_t I2CPoll(unsigned char deviceAddress)
{
    int8_t retval;
    unsigned char slaveAddress = (deviceAddress << 1) | 0;
    if (I2CStatus == I2C_STATUS_ERR) {
        I2CClearErrors();
        if (I2CRecoverBus() == I2C_STATUS_OK) {
            I2CStatus = I2C_STATUS_OK;
        } else {
            return I2C_ERR_Hardware;
        }
    }
    if (I2CStart() == I2C_STATUS_OK) {
        retval = I2CWriteByte((char)slaveAddress);
        if (I2CStop() == I2C_STATUS_OK) {
            if (retval == I2C_ACK) {
                return I2C_STATUS_OK;
            } else if (retval == I2C_ERR_NAK) {
                return I2C_ERR_BadAddr;
            }
        }
    }
    // Set the error flag again since something bad happened
    I2CStatus = I2C_STATUS_ERR;
    return I2C_ERR_CommFail;
}

/**
 * I2CRead()
 *     Description:
 *         Perform an I2C read request for a specific device and read it into a
 *         byte
 *     Params:
 *         unsigned char deviceAdress - The device address to poll
 *         unsigned char registerAddress - The register to read
 *         unsigned char *buffer - The byte to store the read data to
 *     Returns:
 *         int8_t - The read Status
 */
int8_t I2CRead(
    unsigned char deviceAdress,
    unsigned char registerAddress,
    unsigned char *buffer
) {
    unsigned char slaveAddress;
    unsigned char retval;
    if (I2CStatus == I2C_STATUS_ERR) {
        return I2C_ERR_BusDirty;
    }
    if (I2CStart() != I2C_STATUS_OK) {
        // Failed to open bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    // Device Address + Read bit
    slaveAddress = (deviceAdress << 1) | 0x00;
    retval = I2CWriteByte((char)slaveAddress);
    if (retval == I2C_ERR_NAK) {
        // Bad Slave Address or I2C slave device stopped responding
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_BadAddr;
    } else if (retval < 0) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    // Register Addr
    if(I2CWriteByte((char)registerAddress) != I2C_STATUS_OK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    // Repeated start
    if (I2CRestart() != I2C_STATUS_OK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    // Device Address + Read bit
    slaveAddress = (deviceAdress << 1) | 0x01;
    if (I2CWriteByte((char)slaveAddress) != I2C_STATUS_OK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    retval = (unsigned char) I2CReadByte(I2C_NACK);
    if (retval >= 0) {
        *buffer = retval;
    } else {
        // Error while reading byte.  Close connection and set error flag.
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    if (I2CStop() != I2C_STATUS_OK) {
        // Failed to close bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    return I2C_STATUS_OK;
}

/**
 * I2CRecoverBus()
 *     Description:
 *         Handle a bus recovery
 *     Params:
 *         void
 *     Returns:
 *         int8_t The status
 */
int8_t I2CRecoverBus()
{
    int8_t status = I2C_STATUS_OK;
    uint8_t i = 0;

    // Disable the bus and pull both lines high
    I2C3CONLbits.I2CEN = 0;
    I2C3_SDA = 1;
    I2C3_SCL = 1;

    TimerDelayMicroseconds(10);
    if (I2C3_SCL_STATUS == 0) {
        status = I2C_ERR_SCLLow;
    } else {
        // SCL is good -- toggle until SDA goes high.
        while (i <= 10) {
            // Wait until SDA is high
            if (I2C3_SCL_STATUS == 1) {
                break;
            }
            I2C3_SCL = 0;
            TimerDelayMicroseconds(10);
            I2C3_SCL = 1;
            TimerDelayMicroseconds(10);
            i++;
        }
        if (I2C3_SCL_STATUS == 0 && I2C3_SDA_STATUS == 0) {
             status = I2C_ERR_SDALow;
        } else {
            // Restart the I2C bus by resetting the SDA / SCL lines
            I2C3_SDA = 0;
            TimerDelayMicroseconds(10);
            I2C3_SDA = 1;
            TimerDelayMicroseconds(10);
            status = I2C_STATUS_OK;
        }
    }
    if (status < 0) {
        LogError("I2C Error - Status is %d", status);
        return I2C_ERR_Hardware;
    }
    I2C3CONLbits.I2CEN = 1;
    return I2C_STATUS_OK;
}

/**
 * I2CRestart()
 *     Description:
 *         Perform a repeated start sequence
 *     Params:
 *         void
 *     Returns:
 *         int8_t The status
 */
int8_t I2CRestart()
{
    I2C3CONLbits.RSEN = 1;
    uint16_t cycles = 0;
    while (I2C3CONLbits.RSEN) {
        if (cycles > I2C_SCL_TIMEOUT) {
            return I2C_ERR_SCLLow;
        }
        cycles++;
    }
    // Check for a bus collision
    if (I2C3STATbits.BCL) {
        I2C3STATbits.BCL = 0;
        return I2C_ERR_BCL;
    }
    return I2C_STATUS_OK;
}

/**
 * I2CStart()
 *     Description:
 *         Initiate an I2C start sequence on the bus
 *     Params:
 *         void
 *     Returns:
 *         int8_t The status
 */
int8_t I2CStart()
{
    I2C3CONLbits.SEN = 1;
    Nop();
    // Check for a Bus Collision
    if (I2C3STATbits.BCL) {
        I2C3CONLbits.SEN = 0;
        I2C3STATbits.BCL = 0;
        return I2C_ERR_BCL;
    }
    // Check for a write collision
    if (I2C3STATbits.IWCOL) {
        I2C3CONLbits.SEN = 0;
        I2C3STATbits.IWCOL = 0;
        return I2C_ERR_IWCOL;
    }
    // Ensure that the start condition is cleared, otherwise timeout
    uint16_t cycles = 0;
    while (I2C3CONLbits.SEN) {
        if (cycles > I2C_SCL_TIMEOUT) {
            return I2C_ERR_TimeoutHW;
        }
        cycles++;
    }
    // If two start requests are initiated consecutively, the I2C module will
    // instead: Generate a stop request then clear SEN and BCL.
    if (I2C3STATbits.BCL) {
        I2C3STATbits.BCL = 0;
        return I2C_ERR_BCL;
    }
    return I2C_STATUS_OK;
}

/**
 * I2CStop()
 *     Description:
 *         Initiate an I2C sop sequence on the bus
 *     Params:
 *         void
 *     Returns:
 *         int8_t The status
 */
int8_t I2CStop()
{
    I2C3CONLbits.PEN = 1;
    Nop();
    if (I2C3STATbits.BCL) {
        I2C3STATbits.BCL = 0;
        return I2C_ERR_BCL;
    }
    // Ensure that the stop condition is cleared, otherwise timeout
    uint16_t cycles = 0;
    while (I2C3CONLbits.PEN) {
        if (cycles > I2C_SCL_TIMEOUT) {
            return I2C_ERR_SCLLow;
        }
        cycles++;
    }
    return I2C_STATUS_OK;
}

/**
 * I2CWrite()
 *     Description:
 *         Write a byte to a given register on a given device
 *     Params:
 *         unsigned char deviceAdress - The device address to write to
 *         unsigned char registerAddress - The register to write
 *         unsigned char data - The data to write
 *     Returns:
 *         int8_t The status
 */
int8_t I2CWrite(
    unsigned char deviceAddress,
    unsigned char registerAddress,
    unsigned char data
) {
    int8_t retval;
    unsigned char slaveAddress;
    if (I2CStatus == I2C_STATUS_ERR) {
        // Ignore requests until Poll command is called to fix error
        return I2C_ERR_BusDirty;
    }
    if (I2CStart() != 0) {
        // Failed to open bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    // Device Address + Write bit
    slaveAddress = (deviceAddress << 1) | 0; 
    retval = I2CWriteByte((char)slaveAddress);
    if (retval == I2C_ERR_NAK) {
        // Bad Slave Address or I2C slave device stopped responding
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_BadAddr;
    } else if(retval < 0) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    if (I2CWriteByte((char)registerAddress) != I2C_ACK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    if (I2CWriteByte(data) != I2C_ACK) {
        // Error while writing byte.  Close connection and set error flag.
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    if(I2CStop() != I2C_STATUS_OK) {
        // Failed to close bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_ERR_CommFail;
    }
    return I2C_STATUS_OK;
}
