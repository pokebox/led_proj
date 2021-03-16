#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLCDNumber>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QFont>
#include <QKeyEvent>

#include <QFile>
#include <QTextStream>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTcpSocket>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QSettings>

#include <QProcess>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QWebSocket>

#define LOW_BATTERY 2800
#ifdef LINUX
extern "C" {
	#include "i2c/bh1750.h"
	#include "i2c/bmp180.h"
	#include "dht/Raspberry_Pi_2/pi_2_dht_read.h"
}
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	void resizeEvent(QResizeEvent* event);
	void keyPressEvent(QKeyEvent *event);
	void socketWrite(QByteArray data);
	void socketOpen();
	void socketRead();
    void msgProc(QByteArray &json);
	void upInterface();
	void StringToHex(QString str, QByteArray & senddata);
	char ConvertHexChar(char ch);

public slots:
	void onTimerOut();
	void replyFinished(QNetworkReply *reply);
	void getWeather();
#ifdef LINUX
	void onSensor();
	void getDHT();
	void get_net_usage();
#endif

private:
	Ui::MainWindow *ui;
	QTimer *timer;
	QSerialPort *serial;    //全局串口
#ifdef LINUX
	QTimer *timer_DHT;
	QTimer *timer_sensor;
	int f_i2c;
	QFile *cputemp;
#endif

	QNetworkAccessManager *manager;
	QTcpSocket *socket;
    QWebSocket *ws_client;
	QSettings *config;
	QString weather_apikey = "";
	QString weather_city = "";
	QString socketIP="127.0.0.1";
	quint16 socketPort=19085;
    QString wscUrl = "";    //ws客户端连接地址
    bool wscEnabled = true;
	QString PortName = "";
	quint16 BaudRate=9600;

	double m_send_bytes__ = 0;
	double m_recv_bytes__ = 0;
	int grayscale=255;	//灰度
	int color_R = 1;	//红色
	int color_G = 1;	//绿色
	int color_B = 1;	//蓝色
	int color_mode = 7;	//七种颜色模式
};

#endif // MAINWINDOW_H
