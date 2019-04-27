#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QtCore>
#include <QtNetwork>

//\cond HIDDEN_SYMBOLS
class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);

signals:
    void serverIncomingConnection(qintptr);

private:
    void incomingConnection(qintptr handle);
};
//\endcond

#endif // TCPSERVER_H
