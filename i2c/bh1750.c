#include "bh1750.h"
float getbh1750(int *f_bh1750){
	//int f_bh1750;
	char val;
	char buf[3];
	float flight;
	/*
	f_bh1750 = open("/dev/i2c-1",O_RDWR);
	if(f_bh1750<0)
	{
		printf("err open file:%s\r\n",strerror(errno));
		return -1;
	}
	*/
	if(ioctl(*f_bh1750,I2C_SLAVE,I2C_ADDR) < 0)
	{
		printf("ioctl error : %s\r\n",strerror(errno));
		return -1;
	}
	val=0x01;
	if(write(*f_bh1750,&val,1)<0)
	{
		printf("write 0x01 err\r\n");
	}
	val=0x10;
	if(write(*f_bh1750,&val,1)<0)
	{
		printf("write 0x10 err\r\n");
	}

	if(read(*f_bh1750,&buf,3))
	{
		flight=(buf[0]*256+buf[1])/1.2;
		//printf("light is %6.3f\r\n",flight);
		return flight;  //int(flight);
	}
	//close(f_bh1750);
	return 0;
}
