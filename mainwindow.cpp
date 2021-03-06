#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QIODevice>


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setWindowTitle("LED投影时钟");

	qDebug()<<QCoreApplication::applicationDirPath();
	config=new QSettings(QCoreApplication::applicationDirPath()+"/led_proj.ini",QSettings::IniFormat);

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

	bool ok;
	//socket控制配置
	config->beginGroup("socket");
	if(config->value("ip").toString() == "")
		config->setValue("ip","127.0.0.1");
	else
		socketIP=config->value("ip").toString();

	if(config->value("port").toUInt(&ok) == 0 || ok == false)
		config->setValue("port",19085);
	else
		socketPort=config->value("port").toUInt(&ok);
	qDebug()<<socketPort;
	config->endGroup();


    //websocket控制配置
    config->beginGroup("wsClient");
    if(config->value("url").toString() == "")
        config->setValue("url", "ws://127.0.0.1:1880/ledproj");
    if((config->value("enabled").toString() == "1") || (config->value("enabled").toString() == "true"))
        wscEnabled=true;
    else if((config->value("enabled").toString() == "0") || (config->value("enabled").toString() == "false"))
        wscEnabled=false;
    else config->setValue("enabled", "true");

    wscUrl = config->value("url").toString();
    config->endGroup();

    ws_client = new QWebSocket();
    ws_client->setParent(this);
    if (wscEnabled) {
        ws_client->open(QUrl(wscUrl));
    }

    connect(ws_client, &QWebSocket::textMessageReceived, [this](const QString &msg) {
        qDebug()<<QString("ws data: ") + msg;
        QByteArray tmpjson = msg.toUtf8();
        msgProc(tmpjson);
    });

	//串口配置文件
	config->beginGroup("serial");
	//串口名
	if(config->value("PortName").toString() == "")
	{
		config->setValue("PortName","/dev/ttyUSB0");
		PortName="/dev/ttyUSB0";
	}
	else
		PortName=config->value("PortName").toString();
	//波特率
	if(config->value("BaudRate").toUInt(&ok) == 0 || ok == false)
		config->setValue("BaudRate",9600);
	else
		BaudRate=config->value("BaudRate").toUInt(&ok);
	qDebug()<<BaudRate;

	serial = new QSerialPort;
	serial->setPortName(PortName);	//先选择串口

	timer = new QTimer();
	connect(timer,SIGNAL(timeout()),this,SLOT(onTimerOut()));
	timer->start(1000);


#ifdef LINUX
	timer_DHT = new QTimer();
	connect(timer_DHT,SIGNAL(timeout()),this,SLOT(getDHT()));
	timer_DHT->start(10000);

	timer_sensor = new QTimer();
	connect(timer_sensor,SIGNAL(timeout()),this,SLOT(onSensor()));
	timer_sensor->start(800);

	f_i2c=open("/dev/i2c-1",O_RDWR);
	if(f_i2c<0)
	{
		printf("err open file:%s\r\n",strerror(errno));
		timer_sensor->stop();
	}
#endif

	ui->lcd_time->setDigitCount(8);			//设置lcd位数
	ui->lcd_time->setMode(QLCDNumber::Dec);	//设置显示模式十进制
	ui->lcd_time->setSegmentStyle(QLCDNumber::Flat);

	upInterface();	//更新界面
	this->setCursor(Qt::BlankCursor);	//隐藏鼠标

	socket = new QTcpSocket();
	connect(socket,&QTcpSocket::readyRead,this,&MainWindow::socketRead);

	manager = new QNetworkAccessManager(this);
	connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyFinished(QNetworkReply*)));
	showFullScreen();
	getWeather();
	socketOpen();
	socketWrite("{\"cmd\": \"getDeviceList\"}");
    ws_client->sendTextMessage("{\"from\":\"websocket\",\"cmd\": \"getDeviceList\"}");
}

MainWindow::~MainWindow()
{
	serial->clear();	//销毁程序前先关闭串口
	serial->close();
	delete ui;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	//窗口大小改变修改字体大小
	int width=this->width();
	if(width>1920)
		width=1920;
	int fontsize =  int(width / 31.55);	//计算字体大小
	QFont font = ui->lb_date->font();		//取得现有字体
	font.setPointSize(fontsize);				//设置字体大小
	qDebug()<<font.family();
	ui->lb_date->setFont(font);
	ui->lb_weather->setFont(font);
	ui->lb_cputemp->setFont(font);
	ui->lb_sensor->setFont(font);

	ui->label_1->setFont(font);
	ui->label_2->setFont(font);
	ui->label_3->setFont(font);
	ui->label_4->setFont(font);

    ui->label_1->setMaximumWidth((width/2)-2);
    ui->label_2->setMaximumWidth((width/2)-2);
    ui->label_3->setMaximumWidth((width/2)-2);
    ui->label_4->setMaximumWidth((width/2)-2);
    ui->lb_cputemp->setMaximumWidth((width/2)-2);
    ui->lb_sensor->setMaximumWidth((width/2)-2);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Escape:
		close();
		break;
	case Qt::Key_F5:
		if(this->isFullScreen())
		{
			showNormal();		//还原窗口
		}
		else {
			showFullScreen();	//全屏
		}
		break;
	case Qt::Key_F4 :
		if(grayscale<250)
			grayscale+=5;
		upInterface();
		break;
	case Qt::Key_F3:
		if(grayscale>10)
			grayscale-=5;
		upInterface();
		break;
	}
}

void MainWindow::onTimerOut()
{
	QDateTime time = QDateTime::currentDateTime();
	ui->lb_date->setText(time.toString("yyyy年MM月dd日 ddd"));
	ui->lcd_time->display(time.toString("hh:mm:ss"));

	bool ok=true;
    if((time.toString("mm") == "03") || (time.toString("mm") == "30") || (time.toString("mm") == "45"))
	{
		if((time.toString("ss").toInt(&ok,10) >= 0) && (time.toString("ss").toInt(&ok,10) <= 1))
		{
			getWeather();
			socketWrite("{\"cmd\": \"getDeviceList\"}");
		}
	}
	//get_net_usage();
}

void MainWindow::upInterface()	//界面颜色更新
{
    QString str = "color: rgb(20,20,20);background-color: rgb(0, 0, 0);";
    str = "background-color: rgb(0, 0, 0);color: rgb("+
            QString::number(grayscale*color_R)+", "+
            QString::number(grayscale*color_G)+", "+
            QString::number(grayscale*color_B)+");";
    /*
    this->setStyleSheet("color: rgb("+
                        QString::number(grayscale*color_R)+", "+
						QString::number(grayscale*color_G)+", "+
                        QString::number(grayscale*color_B)+");background-color: rgb(0, 0, 0);");
                        */
    this->setStyleSheet(str);
    ui->lcd_time->setStyleSheet(str);
    ui->centralWidget->setStyleSheet(str);

    QColor color(grayscale*color_R, grayscale*color_G, grayscale*color_B);
    ui->label_1->setColor(color);
    ui->label_3->setColor(color);
    //qDebug()<<this->styleSheet();
}

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

void MainWindow::socketOpen()
{
    if (!wscEnabled){
        socket->abort();
        socket->connectToHost(socketIP,socketPort);
        if(!socket->waitForConnected(3000))
        {
            qDebug()<<"连接失败";
            ui->label_1->setText("socket失败");
        }
        else {
            qDebug()<<"连接成功";
        }
    }
}

void MainWindow::socketWrite(QByteArray data)
{
    if (!socket->isOpen()){
        socketOpen();
    }
	qint64 writeResult = socket->write(data);
	bool boolFlush = socket->flush();
	if(writeResult != -1 && boolFlush==1)
	{
		if(writeResult==0)
		{
			qDebug()<<"写数据结果回0";
		}
		qDebug()<<"写数据成功\n";
	}
}

void MainWindow::socketRead()
{
    QByteArray buff;
    buff=socket->readAll();
    //qDebug()<<buff;
    msgProc(buff);
}

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


char MainWindow::ConvertHexChar(char ch)
{
	if((ch >= '0') && (ch <= '9'))
		return ch-0x30;
	else if((ch >= 'A') && (ch <= 'F'))
		return ch-'A'+10;
	else if((ch >= 'a') && (ch <= 'f'))
		return ch-'a'+10;
	//else return (-1);
	else
	{
		return ch-ch;		//不在0-f范围内的会发送成0
	}
}

void MainWindow::StringToHex(QString str, QByteArray & senddata)  //字符串转换成16进制数据0-F
{
	int hexdata,lowhexdata;
	int hexdatalen = 0;
	int len = str.length();
	senddata.resize(len/2);
	char lstr,hstr;
	for(int i=0; i<len; )
	{
		//char lstr,
		hstr=str[i].toLatin1();
		if(hstr == ' ')
		{
			i++;
			continue;
		}
		i++;
		if(i >= len)
			break;
		lstr = str[i].toLatin1();
		hexdata = ConvertHexChar(hstr);
		lowhexdata = ConvertHexChar(lstr);
		if((hexdata == 16) || (lowhexdata == 16))
			break;
		else
			hexdata = hexdata*16+lowhexdata;
		i++;
		senddata[hexdatalen] = static_cast<char>(hexdata);
		hexdatalen++;
	}
	senddata.resize(hexdatalen);
}
