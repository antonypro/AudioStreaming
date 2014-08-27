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

private slots:
    void readyRead();

private:
    QAudioInput *audio;
    QIODevice *device;
};

#endif // AUDIOINPUT_H
