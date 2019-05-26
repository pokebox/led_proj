#ifndef _BH1750_H_
#define _BH1750_H_

#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#define I2C_ADDR 0x23

float getbh1750(int *f_bh1750);
#endif
