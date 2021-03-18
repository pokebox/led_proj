#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QIODevice>

void MainWindow::msgProc(QByteArray &json)
{
    QJsonParseError json_err;
    QJsonDocument json_doc = QJsonDocument::fromJson(json,&json_err);
    if(json_err.error == QJsonParseError::NoError)
    {
        if(json_doc.isObject())
        {
            QJsonObject obj = json_doc.object();
            if (obj.value("cmd").toString() == "heartbeat")
            {
                if (obj.value("model").toString() == "gateway")
                {
                    //qDebug()<<obj.value("token").toString();
                }
            }
            else if(obj.value("cmd").toString() == "report")	//主动回复
            {
                qDebug()<<obj;
                if(obj.value("model") == "motion")	//人体传感器
                {
                    if(obj.value("data").toObject().value("status") == "motion")	//有人移动
                    {
                        ui->label_1->setText("有人走动");
                    }
                    else if (!obj.value("data").toObject().value("no_motion").isUndefined())	//如果存在没人移动
                    {
                        ui->label_1->setText("");
                    }
                }
                if(obj.value("model") == "magnet")	//门窗传感器
                {
                    if(obj.value("data").toObject().value("status") == "open")
                    {
                        ui->label_1->setText("门开了");
                    }
                    if(obj.value("data").toObject().value("status") == "close")
                    {
                        ui->label_1->setText("门关闭");
                    }
                }
                if(obj.value("model") == "switch")	//按钮
                {
                    if(obj.value("data").toObject().value("status") == "click")	//单击
                    {
                        //ui->label_2->setText("单击");
                        color_mode++;
                        if(color_mode>7) color_mode=1;
                        switch (color_mode) {
                            case 1:
                                color_R=1;
                                color_G=0;
                                color_B=0;
                                break;
                            case 2:
                                color_R=0;
                                color_G=1;
                                color_B=0;
                                break;
                            case 3:
                                color_R=0;
                                color_G=0;
                                color_B=1;
                                break;
                            case 4:
                                color_R=1;
                                color_G=1;
                                color_B=0;
                                break;
                            case 5:
                                color_R=1;
                                color_G=0;
                                color_B=1;
                                break;
                            case 6:
                                color_R=0;
                                color_G=1;
                                color_B=1;
                                break;
                            case 7:
                                color_R=1;
                                color_G=1;
                                color_B=1;
                                break;
                            default:
                                color_R=1;
                                color_G=1;
                                color_B=1;
                                break;
                        }
                        upInterface();
                        qDebug()<<color_mode;
                    }
                    else if (obj.value("data").toObject().value("status") == "double_click")	//双击
                    {
                        //ui->label_2->setText("双击");
                        qDebug()<<"双击";
                        getWeather();
                    }
                    else if (obj.value("data").toObject().value("status") == "long_click_press")	//长按
                    {
                        ui->label_2->setText("长按");
                    }
                    else if (obj.value("data").toObject().value("status") == "long_click_release")	//长按松开
                    {
                        ui->label_2->setText("长按松开");
                    }
                }
            }
            else if(obj.value("control").toString() == "display")
            {
                qDebug()<<obj.value("data");
                if(obj.value("data").toObject().value("mode") == "serialdata")
                {
                    //发串口数据，打开投影电源
                    //serial->open();
                    if(serial->open(QIODevice::ReadWrite))
                    {
                        serial->setBaudRate(BaudRate);
                        serial->setDataBits(QSerialPort::Data8);			//设置数据位
                        serial->setParity(QSerialPort::NoParity);			//设置校验位
                        serial->setStopBits(QSerialPort::OneStop);			//设置停止位
                        serial->setFlowControl(QSerialPort::NoFlowControl);
                        qDebug()<<"串口已打开";

                        QString data=obj.value("data").toObject().value("value").toString();
                        QByteArray sendhex;
                        qDebug()<<"writeData将写入："+data;
                        StringToHex(data,sendhex);
                        serial->write(sendhex);
                        serial->flush();
                        serial->close();
                    }
                    else
                    {
                        qDebug()<<"串口无法打开";
                        //ui->label_1->setText("串口打开失败");
                    }
                }
                if(obj.value("data").toObject().value("mode") == "color")
                {
                    int color=obj.value("data").toObject().value("value").toInt();
                    qDebug()<<color;
                    if(color>7) color=1;
                    switch (color) {
                        case 1:
                            color_R=1;
                            color_G=0;
                            color_B=0;
                            break;
                        case 2:
                            color_R=0;
                            color_G=1;
                            color_B=0;
                            break;
                        case 3:
                            color_R=0;
                            color_G=0;
                            color_B=1;
                            break;
                        case 4:
                            color_R=1;
                            color_G=1;
                            color_B=0;
                            break;
                        case 5:
                            color_R=1;
                            color_G=0;
                            color_B=1;
                            break;
                        case 6:
                            color_R=0;
                            color_G=1;
                            color_B=1;
                            break;
                        case 7:
                            color_R=1;
                            color_G=1;
                            color_B=1;
                            break;
                        default:
                            color_R=1;
                            color_G=1;
                            color_B=1;
                            break;
                    }
                    upInterface();
                }
            }
            // 显示固定文本信息
            else if (!obj.value("ledproj").isUndefined()) {
                if (obj.value("ledproj").toObject().value("show1").isString()) {
                    ui->label_1->setText(obj.value("ledproj").toObject().value("show1").toString());
                }
                if (obj.value("ledproj").toObject().value("show2").isString()) {
                    ui->label_2->setText(obj.value("ledproj").toObject().value("show2").toString());
                }
                if (obj.value("ledproj").toObject().value("show3").isString()) {
                    ui->label_3->setText(obj.value("ledproj").toObject().value("show3").toString());
                }
                if (obj.value("ledproj").toObject().value("show4").isString()) {
                    ui->label_4->setText(obj.value("ledproj").toObject().value("show4").toString());
                }
                if (obj.value("ledproj").toObject().value("show5").isString()) {
                    ui->lb_cputemp->setText(obj.value("ledproj").toObject().value("show5").toString());
                }
                if (obj.value("ledproj").toObject().value("show6").isString()) {
                    ui->lb_sensor->setText(obj.value("ledproj").toObject().value("show6").toString());
                }
            }
            else
            {
                qDebug()<<obj;
            }
        }
        else if(json_doc.isArray()){
            qDebug()<<"是数组";
            QJsonArray array = json_doc.array();
            for(int i=0;i<array.size();i++)
            {
                QJsonObject obj=array[i].toObject();
                qDebug()<<obj;
                if(!obj.value("data").isUndefined())	//确定data是存在的
                {
                    QJsonObject dataobj=obj.value("data").toObject();

                    if(obj.value("model").toString() == "magnet")	//门磁传感器
                    {
                        qDebug()<<dataobj.value("status").toString();
                        /*
                        ui->label_3->setText("门状态："+dataobj.value("status").toString() +
                                              "电池电压：" + QString::number(dataobj.value("voltage").toDouble()/1000.0));
                        */
                        qDebug()<<dataobj.value("voltage").toDouble();

                        if(!dataobj.value("voltage").isUndefined())
                        {
                            if(dataobj.value("voltage").toDouble()<LOW_BATTERY)
                            {
                                ui->label_1->setText("门磁电量低");
                            }
                        }
                    }
                    else if(obj.value("model").toString() == "sensor_ht")	//温湿度传感器
                    {
                        bool ok;
                        qDebug()<<"温湿度传感器";
                        /*
                        ui->label_1->setText("温度："+dataobj.value("temperature").toString() +
                                                  "湿度："+dataobj.value("humidity").toString() +
                                                  "电池电压："+QString::number(dataobj.value("voltage").toDouble()/1000.0));
                        */
                        if(dataobj.value("temperature").isUndefined())		//没有温度数据
                        {
                            if(!dataobj.value("humidity").isUndefined())	//有湿度数据
                            {
                                ui->label_4->setText("湿度："+QString::number(dataobj.value("humidity").toString().toDouble(&ok)/100.0) + "%");
                            }
                        }
                        else if(dataobj.value("humidity").isUndefined())	//没有湿度数据
                        {
                            if(!dataobj.value("temperature").isUndefined())	//有温度数据
                            {
                                ui->label_4->setText("温度："+QString::number(dataobj.value("temperature").toString().toDouble(&ok)/100.0) + "℃");
                            }
                        }
                        else
                        {
                            ui->label_4->setText("温度："+QString::number(dataobj.value("temperature").toString().toDouble(&ok)/100.0) + "℃ " +
                                                 "湿度："+QString::number(dataobj.value("humidity").toString().toDouble(&ok)/100.0) + "%");
                        }

                        if(!dataobj.value("voltage").isUndefined())
                        {
                            if(dataobj.value("voltage").toDouble()<LOW_BATTERY)
                                ui->label_1->setText("温湿度电量低");
                        }

                    }
                    else if(obj.value("model").toString() == "motion")	//人体感应
                    {
                        qDebug()<<"人体感应";
                        //sensor_sid=obj.value("sid").toString();
                        //ui->label_4->setText("人体感应：\n电池电压："+QString::number(dataobj.value("voltage").toDouble()/1000.0));
                        if(!dataobj.value("voltage").isUndefined())
                            if(dataobj.value("voltage").toDouble()<LOW_BATTERY)
                                ui->label_1->setText("人体感应电量低");
                    }
                    else if(obj.value("model").toString() == "plug")	//插座
                    {
                        qDebug()<<"插座";
                        //plug_sid=obj.value("sid").toString();
                        /*
                        bool ok;
                        ui->label_2->setText("插座状态："  + dataobj.value("status").toString() + "\n" +
                                             "累计用电量：" + QString::number(dataobj.value("power_consumed").toString().toDouble(&ok)/1000.0) + "kWh\n" +
                                             "负载功率："  + QString::number(dataobj.value("load_power").toString().toDouble(&ok)/1000.0));
                        qDebug()<<dataobj;
                        */

                        qDebug()<<dataobj.value("power_consumed");
                    }
                }

                qDebug()<<"计数器："+QString::number(i)+obj.value("model").toString();
            }
        }
    }
}
