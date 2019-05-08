/*
 * File:   i2c.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Implementation of the I2C Bus. Currently only I2C2 is implemented
 */
#include "i2c.h"
uint8_t I2CStatus;

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
    I2C2_SDA_MODE = 1;
    I2C2_SCL_MODE = 1;
    I2C2_SDA_DC = 1;
    I2C2_SCL_DC = 1;
    I2C2_SDA = 0;
    I2C2_SCL = 0;
    I2C2CONL = 0;
    I2C2CONLbits.I2CEN = 0;
    I2C2BRG = I2C_BRG_400;
    // Enable Slew Mode
    I2C2CONLbits.DISSLW = 0;
    SetI2CMAEV(2, 0);
    I2C2CONLbits.I2CEN = 1;
    // Set the ERR flag so we reset the bus state
    I2CStatus = I2C_STATUS_ERR;
}

/**
 * I2CClearErrors()
 *     Description:
 *         Clear the receive enable, the Bus and Write collision flags
 *     Params:
 *      void
 *     Returns:
 *         void
 */
void I2CClearErrors()
{
    I2C2CONLbits.RCEN = 0;
    I2C2STATbits.IWCOL = 0;
    I2C2STATbits.BCL = 0;
}

//Poll an I2C device to see if it is alive
//This should be done periodically, say every 1 second
//Also does error recovery of the I2C bus here, if indicated
//Returns:
//I2C_OK
//I2C_Err_BadAddr
//I2C_Err_CommFail
//I2C_Err_Hardware
int8_t I2CPoll(unsigned char deviceAddress)
{
    int8_t retval;
    unsigned char slaveAddress = (deviceAddress << 1) | 0;
    if (I2CStatus == I2C_STATUS_ERR) {
        I2CClearErrors();
        if (I2CRecoverBus() == I2C_OK) {
            I2CStatus = I2C_STATUS_OK;
        } else {
            return I2C_Err_Hardware;
        }
    }
    if(I2CStart() == I2C_OK) {
        retval = I2CWriteByte((char)slaveAddress);
        if(I2CStop() == I2C_OK) {
            // Even if we have an error sending, try to close I2C
            if(retval == I2C_ACK) {
                return I2C_OK;
            } else if (retval == I2C_Err_NAK) {
                return I2C_Err_BadAddr;
            } else {
                return I2C_Err_CommFail;
            }
        }
    }
    // Set the error flag again since something bad happened
    I2CStatus = I2C_STATUS_ERR;
    return I2C_Err_CommFail;
}

//High level function.  Reads data from target int16_to buffer
//Returns:
//I2C_Ok
//I2C_Err_BadAddr
//I2C_Err_BusDirty
//I2C_Err_CommFail
int8_t I2CRead(
    unsigned char address,
    unsigned char registerAddress,
    unsigned char *buffer
) {
    unsigned char slaveAddress;
    int16_t retval;
    if (I2CStatus == I2C_STATUS_ERR) {
        //Ignore requests until Poll cmd is called to fix err.
        return I2C_Err_BusDirty;
    }
    if (I2CStart() != I2C_OK) {
        //Failed to open bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    slaveAddress = (address << 1) | 0x00; //Device Address + Write bit
    retval = I2CWriteByte((char)slaveAddress);
    if (retval == I2C_Err_NAK) {
        //Bad Slave Address or I2C slave device stopped responding
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_BadAddr;
    } else if (retval < 0) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    // Register Addr
    if(I2CWriteByte((char)registerAddress) != I2C_OK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    //Repeated start - switch to read mode
    if (I2CRestart() != I2C_OK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    //Device Address + Read bit
    slaveAddress = (address << 1) | 0x01;
    if (I2CWriteByte((char)slaveAddress) != I2C_OK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    retval = (unsigned char) I2CReadByte(I2C_NACK);
    if(retval >= 0) {
        *buffer = retval;
    } else {
        //Error while reading byte.  Close connection and set error flag.
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    if(I2CStop() != I2C_OK) {
        //Failed to close bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    return I2C_OK;
}

//Initiates read of one byte from slave on I2C bus
//Slave must already be addressed, and in read mode
//Waits until completed before returning
//*Caution* Slave can cause a timeout by clock stretching too long
//Returns:
//0x0000-0x00FF Read value stored in low byte (returned integer will always be positive)
//  Error status is indicated by negative return values
//I2C_Err_Overflow
//I2C_Err_RcvTimeout (will happen if slave is clock stretching, or SCL suddenly shorted to ground)
//I2C_Err_SCL_stucklow.  SDA stuck low cannot be detected here.
uint16_t I2CReadByte(unsigned char ackFlag)
{
    uint16_t t = 0;
    // Set state in preparation for TX below
    if(ackFlag == I2C_NACK) {
        I2C2CONLbits.ACKDT = 1;
    } else {
        I2C2CONLbits.ACKDT = 0;
    }
    // Start receive
    I2C2CONLbits.RCEN = 1;
    while (!I2C2STATbits.RBF) {
        t++;
        if (t > 20000) {
            //SCL stuck low
            //RCEN cannot be cleared in SW. Will need to reset I2C interface, or wait until SCL goes high.
            return I2C_Err_RcvTimeout;
        }
    }
    // Set ACK enabled so the slave knows it can send the data
    I2C2CONLbits.ACKEN = 1;
    t = 0;
    while (I2C2CONLbits.ACKEN) {
        t++;
        if (t>1000) {
            // This will timeout if SCL stuck low
            // ACKEN cannot be cleared in SW. I2C interface must be reset after this error.
            return I2C_Err_SCL_low;
        }
    }
    // Check for overflows in the receive buffer
    if(I2C2STATbits.I2COV) {
        I2C2STATbits.I2COV = 0;
        return I2C_Err_Overflow;
    }
    // Reading this register clears RBF
    return I2C2RCV;
}

//Attempt to recover after I2C error
//Returns:
//I2C_OK
//I2C_Err_Hardware
int8_t I2CRecoverBus()
{
    // int status;
    //Level 2: reset devices on I2C network
    //Disable I2C so we can toggle pins
    // I2C2CONLbits.I2CEN = 0;
    // uint16_t i = 0;
    // Start with lines high
    //I2C_SDA = 1;
    //I2C_SCL = 1;

    //TimerDelayMicroseconds(10);
    //if (I2C_SCL_READ == 0) {
   //     status = I2C_Err_SCL_low; //SCL stuck low - is the pullup resistor loaded?
    //} else {
        //SCL ok, toggle until SDA goes high.
//        while(i <= 10) {
//            // Wait until SDA is high
//            if(I2C_SDA_READ == 1) {
//                break;
//            }
//            I2C_SCL = 0;
//            TimerDelayMicroseconds(10);
//            I2C_SCL = 1;
//            TimerDelayMicroseconds(10);
//            i++;
//        }
//        // We are ok if SCL and SDA high
//        if(I2C_SDA_READ == 0 && I2C_SCL_READ == 0) {
//             status = I2C_Err_SDA_low;
//        } else {
//            I2C_SDA = 0; //SDA LOW while SCL HIGH -> START
//            TimerDelayMicroseconds(10);
//            I2C_SDA = 1; //SDA HIGH while SCL HIGH -> STOP
//            TimerDelayMicroseconds(10);
//            status = I2C_OK;
//        }
//    }
//    if (status > 0){
//        //Fatal I2C error, nothing we can do about it
//        return I2C_Err_Hardware;
//    }
//    I2C2CONLbits.I2CEN = 1;
    return I2C_OK;
}

//Initiates repeated start sequence on I2C bus
//Waits until completed before returning
//Returns:
//I2C_OK
//I2C_Err_BCL
//I2C_Err_SCL_low.  SDA stuck low cannot be detected here.
int8_t I2CRestart()
{
    uint16_t t = 0;
    I2C2CONLbits.RSEN = 1; //Initiate restart condition
    while (I2C2CONLbits.RSEN) {
        t++;
        if (t > 1000) {
            // Will timeout if SCL stuck low
            // RSEN cannot be cleared in SW. Will need to reset I2C interface.
            return I2C_Err_SCL_low;
        }
    }
    if (I2C2STATbits.BCL) {
        // SDA stuck low
        I2C2STATbits.BCL = 0; //Clear error to regain control of I2C
        return I2C_Err_BCL;
    }
    return I2C_OK;
}

//Initiates start sequence on I2C bus
//Waits until completed before returning
//Returns:
//I2C_OK
//I2C_Err_BCL
//I2C_Err_IWCOL
//I2C_Err_TimeoutHW
int8_t I2CStart()
{
    uint16_t t = 0;
    I2C2CONLbits.SEN = 1; //Initiate Start condition
    Nop();
    // SCL or SDA stuck low
    if (I2C2STATbits.BCL) {
        I2C2CONLbits.SEN = 0; //Cancel request (will still be set if we had previous BCL)
        I2C2STATbits.BCL = 0; //Clear error to regain control of I2C
        return I2C_Err_BCL;
    }
    //Not sure how this happens but it occurred once, so trap here
    if (I2C2STATbits.IWCOL) {
        I2C2CONLbits.SEN = 0; //Clear just in case set
        I2C2STATbits.IWCOL = 0; //Clear error
        return I2C_Err_IWCOL;
    }
    while (I2C2CONLbits.SEN)  {
        t++;
        if (t > 1000) {
            //Since SCL and SDA errors are trapped by BCL error above, this should never happen
            return I2C_Err_TimeoutHW;
        }
    }
    //If a second start request is issued after first one, the I2C module will instead:
    //generate a stop request, clear SEN, and flag BCL.  Test for BCL here.
    if (I2C2STATbits.BCL) {
        I2C2STATbits.BCL = 0; //Clear error to regain control of I2C
        return I2C_Err_BCL;
    }
    return I2C_OK;
}

//Initiates stop sequence on I2C bus
//Waits until completed before returning
//Returns:
//I2C_OK
//I2C_Err_BCL
//I2C_Err_SCL_low.  SDA stuck low cannot be detected here.
int8_t I2CStop()
{
    uint16_t t = 0;
    I2C2CONLbits.PEN = 1; //Initiate stop condition
    Nop();
    if (I2C2STATbits.BCL) {
        //Not sure if this can ever happen here
        I2C2STATbits.BCL = 0; //Clear error
        return I2C_Err_BCL; //Will need to reset I2C interface.
    }
    while(I2C2CONLbits.PEN) {
        t++;
        if (t > 1000) {
            //Will timeout if SCL stuck low
            //PEN cannot be cleared in SW. Will need to reset I2C interface.
            return I2C_Err_SCL_low;
        }
    }
    return I2C_OK;
}

//High level function.  Writes buffered data to target address.
//Returns:
//I2C_OK
//I2C_Err_BadAddr
//I2C_Err_BusDirty
//I2C_Err_CommFail
int8_t I2CWrite(
    unsigned char deviceAddress,
    unsigned char registerAddress,
    unsigned char data
) {
    int8_t retval;
    unsigned char slaveAddress;
    if (I2CStatus == I2C_STATUS_ERR) {
        //Ignore requests until Poll cmd is called to fix err.
        return I2C_Err_BusDirty;
    }
    if (I2CStart() != 0) {
        //Failed to open bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    // Device Address + Write bit
    slaveAddress = (deviceAddress << 1) | 0; 
    retval = I2CWriteByte((char)slaveAddress);
    if (retval == I2C_Err_NAK) {
        //Bad Slave Address or I2C slave device stopped responding
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_BadAddr;
    } else if(retval < 0) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    if (I2CWriteByte((char)registerAddress) != I2C_ACK) {
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    if (I2CWriteByte(data) != I2C_ACK) {
        //Error while writing byte.  Close connection and set error flag.
        I2CStop();
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    if(I2CStop() != I2C_OK) {
        //Failed to close bus
        I2CStatus = I2C_STATUS_ERR;
        return I2C_Err_CommFail;
    }
    return I2C_OK;
}

//Sends a byte to a slave
//Waits until completed before returning
//Returns:
//I2C_ACK
//I2C_Err_BCL
//I2C_Err_NAK
//I2C_Err_SCL_low
//I2C_Err_TBF
int8_t I2CWriteByte(char data)
{
    uint16_t t = 0;
    // Check to see if there is a write pending
    if(I2C2STATbits.TBF) {
        return I2C_Err_TBF;
    }
    I2C2TRN = data;
    while(I2C2STATbits.TRSTAT) {
        t++;
        if (t > 8000) {
            //This is bad because TRSTAT will still be set
            return I2C_Err_SCL_low; //Must reset I2C interface, and possibly slave devices
        }
    }
    if (I2C2STATbits.BCL) {
        I2C2STATbits.BCL = 0; //Clear error to regain control of I2C
        return I2C_Err_BCL;
    }
    // Return the slave response
    if (I2C2STATbits.ACKSTAT == 1) {
        return I2C_Err_NAK;
    }
    return I2C_ACK;
}
