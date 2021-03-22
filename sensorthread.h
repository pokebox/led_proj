#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QObject>
#include <QThread>
#include <QDateTime>
#include <QFile>
#include <QDebug>

#ifdef LINUX
extern "C" {
    #include "i2c/bh1750.h"
    #include "i2c/bmp180.h"
    #include "dht/Raspberry_Pi_2/pi_2_dht_read.h"
}
#endif

class sensorThread : public QThread
{
    Q_OBJECT
public:
    explicit sensorThread(QObject *parent = nullptr);
    void run();
    void onSensor();
    void getCPUtemp();

signals:
    void setCPUStr(QString data);
    void socketStr(QString data);
    void updateSensor(int light);
    void setSensorValue(double t, double p, double h);
public slots:

private:
    int f_i2c;
    QString cputemp;
    bool send_ws_flag=false;
};

#endif // MYTHREAD_H
