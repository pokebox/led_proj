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

    weatherInit();
    socketInit();

    bool ok;
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
