#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QIODevice>

void MainWindow::socketInit()
{
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

    socket = new QTcpSocket();
    connect(socket,&QTcpSocket::readyRead,this,&MainWindow::socketRead);

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

    connect(ws_client, &QWebSocket::disconnected,[this](){
        qDebug()<<"连接断开，尝试重连";
        ws_client->close();
        ws_client->open(QUrl(wscUrl));
    });
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

void MainWindow::wsWrite(QString data)
{
    ws_client->sendTextMessage(data);
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
