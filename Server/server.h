#ifndef SERVER_H
#define SERVER_H

#include <QtCore>
#include <QtNetwork>

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);

    static QByteArray IntToArray(qint32 value);

signals:
    void error(QString);

public slots:
    void start(quint16 port);
    void writeData(const QByteArray &data);
    void setHeader(const QByteArray &data);

private slots:
    void newConnection();
    void disconnected();

private:
    QTcpServer *server;
    QList<QTcpSocket*> sockets;
    QByteArray header;
};

#endif // SERVER_H
