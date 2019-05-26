#ifndef _BMP180_H_
#define _BMP180_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <unistd.h>

//BMP180 default address.
#define BMP180_I2CADDR            0x77

//Operating Modes
//Always us BMP180_ULTRALOWPOWER in this example
#define BMP180_ULTRALOWPOWER      0
#define BMP180_STANDARD           1
#define BMP180_HIGHRES            2
#define BMP180_ULTRAHIGHRES       3

//BMP180 Registers
#define BMP180_CAL_AC1            0xAA //R   Calibration data (16 bits)
#define BMP180_CAL_AC2            0xAC
#define BMP180_CAL_AC3            0xAE
#define BMP180_CAL_AC4            0xB0
#define BMP180_CAL_AC5            0xB2
#define BMP180_CAL_AC6            0xB4
#define BMP180_CAL_B1             0xB6
#define BMP180_CAL_B2             0xB8
#define BMP180_CAL_MB             0xBA
#define BMP180_CAL_MC             0xBC
#define BMP180_CAL_MD             0xBE
#define BMP180_CONTROL            0xF4
#define BMP180_DATA               0xF6
#define BMP180_ID                 0xD0

//Commands
#define BMP180_READTEMPCMD        0x2E
#define BMP180_READPRESSURECMD    0x34

#define RPI_I2C_OK                1
//example: BMP180
typedef signed char             int8_t;
typedef short int               int16_t;
typedef int                     int32_t;

typedef unsigned char           uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;

//This function read a byte back from the register address specified by buf.
uint8_t read_8(int *rpi_i2c, char *buf);

//This function read two byte back from the register address specified by buf.
uint16_t read_16(int *rpi_i2c, char *buf);

//Get the calibration value of the sensor.
void get_cal(int *rpi_i2c, uint16_t *cal_AC1, uint16_t *cal_AC2, uint16_t *cal_AC3,
             uint16_t *cal_AC4, uint16_t *cal_AC5, uint16_t *cal_AC6,
             uint16_t *cal_B1, uint16_t *cal_B2,
             uint16_t *cal_MB, uint16_t *cal_MC, uint16_t *cal_MD);

int getbmp180(int *rpi_i2c1,double *temp, double *pressure);
#endif