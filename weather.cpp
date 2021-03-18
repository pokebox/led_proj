#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QIODevice>

void MainWindow::weatherInit()
{
    //天气
    config->beginGroup("weather");
    if(config->value("apikey").toString() == "")
        config->setValue("apikey","");
    else
        weather_apikey=config->value("apikey").toString();

    if(config->value("city").toString() == "")
        config->setValue("city","");
    else
        weather_city=config->value("city").toString();
    config->endGroup();
}

void MainWindow::getWeather()
{
    if(weather_apikey=="")
    {
        ui->lb_weather->setText("没有设置apikey");
    }
    else
    {
        QString city = "";
        if(weather_city!="")	//检测是否设置了位置
        {
            city = weather_city;
            char quest_array[256]="http://restapi.amap.com/v3/weather/weatherInfo?key=";
            sprintf(quest_array,"%s%s&city=%s",quest_array,weather_apikey.toUtf8().data(),city.toUtf8().data());
            qDebug()<<quest_array;
            manager->get(QNetworkRequest(QUrl(quest_array)));

            //取预测天气
            sprintf(quest_array,"%s&extensions=all",quest_array);
            manager->get(QNetworkRequest(QUrl(quest_array)));
            qDebug()<<quest_array;
        }
        else	//从ip获取位置信息
        {
            char ip_array[256]="http://restapi.amap.com/v3/ip?key=";
            sprintf(ip_array,"%s%s",ip_array,weather_apikey.toUtf8().data());
            manager->get(QNetworkRequest(QUrl(ip_array)));
        }
    }
}

void MainWindow::replyFinished(QNetworkReply *reply)
{
    //QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(reply->error() == QNetworkReply::NoError)
    {
        QString Json = reply->readAll();
        qDebug()<<Json;

        QByteArray byte_array;
        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(byte_array.append(Json),&json_error);

        if(json_error.error == QJsonParseError::NoError)
        {
            QJsonObject all = parse_doucment.object();
            qDebug()<<all;

            if(all.value("info") == "OK")
            {
                if(!all.value("lives").isUndefined())	//实时天气
                {
                    QJsonObject obj=all.value("lives").toArray()[0].toObject();
                    ui->lb_weather->setText(obj.value("city").toString() + " " +
                                            obj.value("weather").toString() + " " +
                                            obj.value("temperature").toString() + "℃ 湿度" +
                                            obj.value("humidity").toString() + "%");
                }
                else if(!all.value("forecasts").isUndefined())
                {
                    QJsonArray arr=all.value("forecasts").toArray()[0].toObject().value("casts").toArray();
                    QJsonObject tomorrow=arr[1].toObject();
                    if(tomorrow.value("dayweather").toString() == tomorrow.value("nightweather").toString())
                    {
                        ui->label_2->setText("明天 " +
                                             tomorrow.value("dayweather").toString() + " " +
                                             tomorrow.value("daytemp").toString() + "~" +
                                             tomorrow.value("nighttemp").toString() + "℃");
                    }
                    else
                    {
                        ui->label_2->setText("明天 " +
                                             tomorrow.value("dayweather").toString() + "转" +
                                             tomorrow.value("nightweather").toString() + " " +
                                             tomorrow.value("daytemp").toString() + "~" +
                                             tomorrow.value("nighttemp").toString() + "℃");
                    }

                }
                else if(!all.value("adcode").isUndefined())	//有地理位置信息
                {
                    qDebug()<<"有地理位置信息";
                    if(all.value("adcode").isString())
                    {
                        weather_city=all.value("adcode").toString();
                        getWeather();	//得到地理位置后再获取一次天气
                        qDebug()<<"地理位置："+weather_city;
                    }
                    else
                    {
                        weather_city="310000";	//如果一直获取不到有效位置则随便设置一个避免死循环获取位置信息
                        getWeather();
                    }
                }
                //将天气信息送到socket
                socketWrite(QString(QJsonDocument(all).toJson(QJsonDocument::Compact)).toUtf8());
                ws_client->sendTextMessage(QString(QJsonDocument(all).toJson(QJsonDocument::Compact)));
            }
        }
        else
        {
            qDebug()<<"Json Error";
        }
    }
    else
    {
        qDebug()<<"网络错误";
    }

}
