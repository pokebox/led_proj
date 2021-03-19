#include "sensorthread.h"
#include "mainwindow.h"

sensorThread::sensorThread(QObject *parent) : QThread(parent)
{

}

void sensorThread::run()
{
    f_i2c=open("/dev/i2c-1",O_RDWR);
    if(f_i2c<0)
    {
        printf("err open file:%s\r\n",strerror(errno));
    }
    while (1) {
        onSensor();
        usleep(100000);
    }
}

void sensorThread::onSensor()
{
    bool ok;
    QDateTime time = QDateTime::currentDateTime();
    float light=getbh1750(&f_i2c);

    double grayscale;
    if( (time.toString("hh").toInt(&ok) >= 7 ) && (time.toString("hh").toInt(&ok) <= 19 ) )
    {
        grayscale=255;
    }
    else if(light>=0)
    {
        if(light>30)
        {
            grayscale=250;
        }
        else if(light<0.9)
        {
            grayscale=25;
        }
        else if(light<10)
        {
            grayscale=int((light+25)*1.9);
        }
        else
        {
            grayscale=int((light+20)*3.5);
        }
    }
    else
    {
        printf("read light error\r\n");
        grayscale=255;
    }
    emit updateSensor();
    printf("light is %6.3f\t %d\r\n", light, grayscale);

    double temp=0,pressure=0,humidity=0;
    if(getbmp180(&f_i2c,&temp,&pressure)==0)
    {
        emit setSensorValue(temp, pressure, humidity);
    }

    if (time.toString("ss").toInt(&ok)%30 == 0 ) {
        QString data = "{\"datetime\":\"" + time.toString("yyyy-MM-dd hh:mm:ss")
                + "\",\"sensor\":{\"temperature\":"
                + QString::number(temp,10,1) + ",\"humidity\":"
                + QString::number(humidity,10,1) + ",\"pressure\":"
                + QString::number(pressure, 10, 2) + ",\"luminance\":"
                + QString::number(light, 10, 1) + "}}";
        emit socketStr(data);
    }

    if (time.toString("ss").toInt(&ok)%2 == 0 )
    {
        getCPUtemp();
    }
}

void sensorThread::getCPUtemp()
{
    cputemp = new QFile("/sys/class/thermal/thermal_zone0/temp");
    if(!cputemp->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug()<<"读取温度失败";
    }
    else
    {
        QTextStream stream(cputemp);
        QString string = stream.readAll();
        cputemp->close();
        QString s = QString::number(string.toLong() /1000);
        emit setCPUStr("CPU: "+s+" ℃");
    }
    cputemp->close();
}
