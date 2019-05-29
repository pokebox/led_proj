#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QIODevice>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
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

	this->setStyleSheet("color: rgb("+QString::number(grayscale)+", "+QString::number(grayscale)+", "+QString::number(grayscale)+");background-color: rgb(0, 0, 0);");
	this->setCursor(Qt::BlankCursor);	//隐藏鼠标

	manager = new QNetworkAccessManager(this);
	connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyFinished(QNetworkReply*)));
	showFullScreen();
	getWeather();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	//窗口大小改变修改字体大小
	int fontsize =  int(this->width() / 31.55);	//计算字体大小
	QFont font = ui->lb_date->font();		//取得现有字体
	font.setPointSize(fontsize);				//设置字体大小
	ui->lb_date->setFont(font);
	ui->lb_weather->setFont(font);
	ui->lb_cputemp->setFont(font);
	ui->lb_sensor->setFont(font);
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
		this->setStyleSheet("color: rgb("+QString::number(grayscale)+", "+QString::number(grayscale)+", "+QString::number(grayscale)+");background-color: rgb(0, 0, 0);");
		break;
	case Qt::Key_F3:
		if(grayscale>10)
			grayscale-=5;
		this->setStyleSheet("color: rgb("+QString::number(grayscale)+", "+QString::number(grayscale)+", "+QString::number(grayscale)+");background-color: rgb(0, 0, 0);");
		break;
	}
}

void MainWindow::onTimerOut()
{
	QDateTime time = QDateTime::currentDateTime();
	ui->lb_date->setText(time.toString("yyyy年MM月dd日 ddd"));
	ui->lcd_time->display(time.toString("hh:mm:ss"));

	bool ok=true;
	if((time.toString("mm") == "10") || (time.toString("mm") == "30") || (time.toString("mm") == "50"))
	{
		if((time.toString("ss").toInt(&ok,10) >= 0) && (time.toString("ss").toInt(&ok,10) <= 1))
			getWeather();
	}
	//get_net_usage();
}

#ifdef LINUX
void MainWindow::onSensor()
{
	bool ok;
	QDateTime time = QDateTime::currentDateTime();
	float light=getbh1750(&f_i2c);
	if( (time.toString("hh").toInt(&ok) >= 7 ) && (time.toString("hh").toInt(&ok) <= 19 ) )
	{
		grayscale=255;
		this->setStyleSheet("color: rgb("+QString::number(grayscale)+", "+QString::number(grayscale)+", "+QString::number(grayscale)+");background-color: rgb(0, 0, 0);");
	}
	else if(light>=0)
	{
		if(light>30)
		{
			grayscale=250;
		}
		else if(light<3.5)
		{
			grayscale=25;
		}
		else
		{
			grayscale=int(light*8);
		}
		this->setStyleSheet("color: rgb("+QString::number(grayscale)+", "+QString::number(grayscale)+", "+QString::number(grayscale)+");background-color: rgb(0, 0, 0);");
	}
	else
	{
		printf("read light error\r\n");
		grayscale=255;
		this->setStyleSheet("color: rgb("+QString::number(grayscale)+", "+QString::number(grayscale)+", "+QString::number(grayscale)+");background-color: rgb(0, 0, 0);");
	}
	printf("light is %6.3f\r\n",light);

	double temp=0,pressure=0;
	if(getbmp180(&f_i2c,&temp,&pressure)==0)
	{
		ui->lb_sensor->setText("温度："+QString::number(temp,10,2)+" 气压："+QString::number(pressure, 10, 2));
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
	QString city = "青浦";
	char quest_array[256]="http://www.tianqiapi.com/api/?version=v6&city=";
	sprintf(quest_array,"%s%s",quest_array,city.toUtf8().data());
	manager->get(QNetworkRequest(QUrl(quest_array)));
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
			QJsonObject obj = parse_doucment.object();
			qDebug()<<obj;
			ui->lb_weather->setText(obj.take("city").toString() + "区 " +
									obj.take("wea").toString() + " " +
									obj.take("tem").toString() + "℃ 湿度" +
									obj.take("humidity").toString());
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


