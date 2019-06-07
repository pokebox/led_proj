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
	void upInterface();

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
#ifdef LINUX
	QTimer *timer_DHT;
	QTimer *timer_sensor;
	int f_i2c;
	QFile *cputemp;
#endif

	QNetworkAccessManager *manager;
	QTcpSocket *socket;
	QSettings *config;
	QString weather_apikey = "";
	QString weather_city = "";
	QString socketIP="127.0.0.1";
	quint16 socketPort=19085;

	double m_send_bytes__ = 0;
	double m_recv_bytes__ = 0;
	int grayscale=255;	//灰度
	int color_R = 1;	//红色
	int color_G = 1;	//绿色
	int color_B = 1;	//蓝色
	int color_mode = 7;	//七种颜色模式
};

#endif // MAINWINDOW_H
