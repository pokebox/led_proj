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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QProcess>
extern "C" {
	#include "i2c/bh1750.h"
	#include "i2c/bmp180.h"
	#include "dht/Raspberry_Pi_2/pi_2_dht_read.h"
}
#define LINUX

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	void resizeEvent(QResizeEvent* event);
	void keyPressEvent(QKeyEvent *event);

public slots:
	void onTimerOut();
	void replyFinished(QNetworkReply *reply);
	void getWeather();
	void get_net_usage();
#ifdef LINUX
	void onSensor();
	void getDHT();
#endif

private:
	Ui::MainWindow *ui;
	QTimer *timer;
#ifdef LINUX
	QTimer *timer_DHT;
	QTimer *timer_sensor;
	int f_i2c;
#endif
	QFile *cputemp;

	QNetworkAccessManager *manager;

	double m_send_bytes__ = 0;
	double m_recv_bytes__ = 0;
	int grayscale=255;
};

#endif // MAINWINDOW_H
