#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QtCore>
#include <QtNetwork>

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = 0);

signals:
    void serverIncomingConnection(qintptr);

private:
    void incomingConnection(qintptr handle);
};

#endif // TCPSERVER_H
