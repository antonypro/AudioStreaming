#ifndef CLIENT_H
#define CLIENT_H

#include <QtCore>
#include <QtNetwork>
#include "audiooutput.h"

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QString host, quint16 port, QObject *parent = 0);

signals:

public slots:

private slots:
    void readyRead();

private:
    QTcpSocket *socket;
    AudioOutput output;
};

#endif // CLIENT_H
