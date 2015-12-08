#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "server.h"

class AudioInput : public QObject
{
    Q_OBJECT
public:
    explicit AudioInput(QAudioDeviceInfo devinfo, QObject *parent = 0);

signals:
    void dataReady(QByteArray data);

public slots:
    QByteArray header();

private slots:
    void readyRead();

private:
    QAudioInput *audio;
    QIODevice *device;
    QAudioFormat format;
};

#endif // AUDIOINPUT_H
