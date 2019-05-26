#include "bmp180.h"

int getbmp180(int *rpi_i2c1,double *temp, double *pressure)
{
    /*
    int *rpi_i2c1;
    *rpi_i2c1=open("/dev/i2c-1",O_RDWR);
    if(*rpi_i2c1<0){
        printf("error");
        return -2;
    }
    else{
        */
        if(ioctl(*rpi_i2c1,I2C_SLAVE,BMP180_I2CADDR)<0) {
            printf("I2C error \n");
            return (-1);
        }
        
        //Get default chipID, it should be 0X55.
        char buf[] = { BMP180_ID }; // Data to send; Chip-ID register
        char len = sizeof(buf) / sizeof(char);
        /*
        int w_return = write(*rpi_i2c1, &buf, len);
        if(w_return == RPI_I2C_OK){
            printf("Write to I2C: %02X \n", buf[0]);
        }
        //Read chip ID.
        
        int r_return = read(*rpi_i2c1, &buf, len);
        if (r_return == RPI_I2C_OK) {
            printf("Read from I2C: %02X\n", buf[0]); //Should be 0x55 for MP180
        }
        */
        write(*rpi_i2c1, &buf, len);
        read(*rpi_i2c1, &buf, len);
        usleep(90);

        uint16_t cal_AC1;
        uint16_t cal_AC2;
        uint16_t cal_AC3;
        uint16_t cal_AC4;
        uint16_t cal_AC5;
        uint16_t cal_AC6;
        uint16_t cal_B1;
        uint16_t cal_B2;
        uint16_t cal_MB;
        uint16_t cal_MC;
        uint16_t cal_MD;

        get_cal(*rpi_i2c1, &cal_AC1, &cal_AC2, &cal_AC3,
                &cal_AC4, &cal_AC5, &cal_AC6,
                &cal_B1, &cal_B2,
                &cal_MB, &cal_MC, &cal_MD);

        int16_t AC1 = (int16_t)cal_AC1;
        int16_t AC2 = (int16_t)cal_AC2;
        int16_t AC3 = (int16_t)cal_AC3;
        uint16_t AC4 = cal_AC4;
        uint16_t AC5 = cal_AC5;
        uint16_t AC6 = cal_AC6;
        int16_t B1 = (int16_t)cal_B1;
        int16_t B2 = (int16_t)cal_B2;
        int16_t MB = (int16_t)cal_MB;
        int16_t MC = (int16_t)cal_MC;
        int16_t MD = (int16_t)cal_MD;

        //get temperature and pressure
        char t_command[2] = {BMP180_CONTROL , BMP180_READTEMPCMD}; //temperature cmd
        char p_command[2] = {BMP180_CONTROL , BMP180_READPRESSURECMD}; //pressure cmd
        char t_reg[1] = {BMP180_DATA};
        char pressure_reg[3] = {BMP180_DATA, BMP180_DATA+1, BMP180_DATA+2};
        write(*rpi_i2c1, &t_command, 2);
        usleep(5000);
        long T_raw = read_16(*rpi_i2c1, t_reg);

        write(*rpi_i2c1, &p_command, 2);
        usleep(10000);
        uint8_t msb = read_8(*rpi_i2c1, &pressure_reg[0]);
        uint8_t lsb = read_8(*rpi_i2c1, &pressure_reg[1]);
        uint8_t xlsb = read_8(*rpi_i2c1, &pressure_reg[2]);
        long P_raw = ((msb << 16) + (lsb << 8) + xlsb) >> 8;

        long X1 = ((T_raw - AC6) * AC5) >> 15;
        long X2 = (MC << 11) / (X1 + MD);
        long B5 = X1 + X2;
        double x_temp = ((B5 + 8) >> 4) / 10.0;

        //printf("temperature %f *C \n", temp);
        *temp=x_temp;

        long p;
        long B6 = B5 - 4000;
        X1 = (B2 * (B6*B6) >> 12) >> 11;
        X2 = (AC2 * B6) >> 11;
        long X3 = X1 + X2;
        long B3 = ((AC1*4+X3)+2)/4;
        X1 = (AC3 * B6) >> 13;
        X2 = (B1*(B6*B6)>>12)>>16;
        X3 = (X1+X2+2) >> 2;
        unsigned long B4 = (AC4 * (unsigned long)(X3 + 32768)) >> 15;
        unsigned long B7 = (unsigned long)(P_raw-B3)*50000;
        if(B7 < 0x80000000) {
            p = (B7 * 2) / B4;
        } else {
            p = (B7 / B4) * 2;
        }
        X1 = (p >> 8) * (p >> 8);
        X1 = (X1 * 3038) >> 16;
        X2 = (-7357 * p) >> 16;
        p = p + ((X1 + X2 + 3791) >> 4);
        double x_pressure = p * 0.01;
        //printf("pressure %fhPa \n", pressure);
        *pressure=x_pressure;
    //}
    //close(*rpi_i2c1);
    return 0;
}

uint16_t read_16(int *rpi_i2c, char *buf)
{
    uint16_t ret;
    char rbuf[2];
    write(rpi_i2c, buf, 1);
    read(rpi_i2c, rbuf, 2);
    ret = rbuf[0] << 8 | rbuf[1];
    return ret;
}

uint8_t read_8(int *rpi_i2c, char *buf)
{
    uint8_t ret;
    char rbuf[1];
    write(rpi_i2c, buf, 1);
    read(rpi_i2c, rbuf, 1);
    ret = rbuf[0];
    return ret;
}

void get_cal(int *rpi_i2c, uint16_t *cal_AC1, uint16_t *cal_AC2, uint16_t *cal_AC3,
             uint16_t *cal_AC4, uint16_t *cal_AC5, uint16_t *cal_AC6,
             uint16_t *cal_B1, uint16_t *cal_B2,
             uint16_t *cal_MB, uint16_t *cal_MC, uint16_t *cal_MD)
{
    char buf[1];
    buf[0] = BMP180_CAL_AC1;
    *cal_AC1 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_AC2;
    *cal_AC2 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_AC3;
    *cal_AC3 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_AC4;
    *cal_AC4 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_AC5;
    *cal_AC5 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_AC6;
    *cal_AC6 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_B1;
    *cal_B1 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_B2;
    *cal_B2 = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_MB;
    *cal_MB = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_MC;
    *cal_MC = read_16(rpi_i2c, buf);

    buf[0] = BMP180_CAL_MD;
    *cal_MD = read_16(rpi_i2c, buf);
}
