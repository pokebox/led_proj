#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QIODevice>

#ifdef LINUX
void MainWindow::onSensor()
{
    bool ok;
    QDateTime time = QDateTime::currentDateTime();
    float light=getbh1750(&f_i2c);
    //qDebug()<<time.toString("hh");

    if( (time.toString("hh").toInt(&ok) >= 7 ) && (time.toString("hh").toInt(&ok) <= 19 ) )
    {
        grayscale=255;
        upInterface();
        //qDebug()<<"isokssssssss";
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
        upInterface();
    }
    else
    {
        printf("read light error\r\n");
        grayscale=255;
        upInterface();
    }
    printf("light is %6.3f\t %d\r\n",light,grayscale);

    double temp=0,pressure=0,humidity=0;
    if(getbmp180(&f_i2c,&temp,&pressure)==0)
    {
        ui->lb_sensor->setText("温度:"+QString::number(temp,10,1)+" 气压:"+QString::number(pressure, 10, 1));
    }

    if (time.toString("ss").toInt(&ok)%30 == 0 ) {
        QString data = "{\"datetime\":\"" + time.toString("yyyy-MM-dd hh:mm:ss")
                + "\",\"sensor\":{\"temperature\":"
                + QString::number(temp,10,1) + ",\"humidity\":"
                + QString::number(humidity,10,1) + ",\"pressure\":"
                + QString::number(pressure, 10, 2) + ",\"luminance\":"
                + QString::number(light, 10, 1) + "}}";
        socketWrite(data.toUtf8());
        ws_client->sendTextMessage(data);
    }
    timer_sensor->start(200);
}

void MainWindow::getDHT()
{
    /*
    float humidity = 0, temperature = 0;
    int result = pi_2_dht_read(22, 4, &humidity, &temperature);
    printf("\n\thum: %6.3f  temp: %6.3f\n",humidity,temperature);
    if((humidity != 0) && (temperature != 0))
    {
        ui->lb_sensor->setText("温度：" + QString::number(temperature) +
                    "℃ 湿度："+ QString::number(humidity) + "%");
    }
    */

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
        ui->lb_cputemp->setText("CPU: "+s+" ℃");
    }
    cputemp->close();
}

void MainWindow::get_net_usage()
{
    QProcess process;
    process.start("cat /proc/net/dev");        //读取文件/proc/net/dev获取网络收发包数量，再除取样时间得到网络速度
    process.waitForFinished();
    process.readLine();
    process.readLine();
    while(!process.atEnd())
    {
        QString str = process.readLine();
        str.replace("\n","");
        str.replace(QRegExp("( ){1,}")," ");
        //qDebug()<<str;
        auto lst = str.split(" ");
        qDebug()<<lst[0];
        if(lst.size() > 9 && lst[1] == "eth0:")
        {
            double recv = 0;
            double send = 0;
            if(lst.size() > 1)
                recv = lst[1].toDouble();
            if(lst.size() > 9)
                send = lst[9].toDouble();
            //qDebug()<<lst[0].toStdString().c_str();
            //qDebug()<<(recv - m_recv_bytes__) / (m_timer_interval__ / 1000.0),(send - m_send_bytes__) / (m_timer_interval__ / 1000.0);

            qDebug("%s  接收速度:%.0lfbyte/s 发送速度:%.0lfbyte/s",lst[0].toStdString().c_str(),(recv - m_recv_bytes__) / (1000 / 1000.0),(send - m_send_bytes__) / (1000 / 1000.0));
            m_recv_bytes__ = recv;
            m_send_bytes__ = send;
        }
    }
}

#endif
