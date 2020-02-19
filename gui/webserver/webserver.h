#ifndef WEBSERVER_H
#define WEBSERVER_H

#pragma once

#include <QtNetwork>
#include <QTcpSocket>
#include <QObject>
#include <QByteArray>

class QTcpServer;
class QTcpSocket;

class WebServer : public QObject
{
    Q_OBJECT
    WebServer *server;

public:
    WebServer();
    ~WebServer();
    QTcpServer *tcpServer;
    int server_status;
    QMap<int,QTcpSocket *> SClients;

public Q_SLOTS:
    void startWebserver();
    void newUser();
    void slotReadClient();
};


#endif
