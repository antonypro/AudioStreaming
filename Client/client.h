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
    void audioReady(QByteArray);

public slots:
    QAudioFormat currentAudioFormat();

private slots:
    void readyRead();
    void readHeader(QByteArray data);

private:
    QTcpSocket *socket;
    AudioOutput *output;
    QAudioFormat format;
};

#endif // CLIENT_H
