#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "server.h"
#ifdef OPUS
#include <r8brain.h>
#include <opusencode.h>
#endif

class AudioInput : public QObject
{
    Q_OBJECT
public:
    explicit AudioInput(QObject *parent = 0);

signals:
    void error(QString);
    void adjustSettings(QAudioFormat);
    void dataReady(QByteArray);

public slots:
    void start(const QAudioDeviceInfo &devinfo, int samplesize, int samplerate, int channels, int sampletype, int byteorder);
    QByteArray header();

private slots:
    void readyRead();

private:
    QAudioInput *audio;
    QIODevice *device;
    QAudioFormat format;
#ifdef OPUS
    r8brain *res;
    OpusEncode *opus;
    QByteArray buffer;
#endif
};

#endif // AUDIOINPUT_H
