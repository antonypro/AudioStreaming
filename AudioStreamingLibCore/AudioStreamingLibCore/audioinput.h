#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "common.h"

class AudioInput : public QObject
{
    Q_OBJECT
public:
    explicit AudioInput(QObject *parent = 0);

signals:
    void error(QString);
    void readyRead(QByteArray);

public slots:
    void start(const QAudioDeviceInfo &devinfo,
               const QAudioFormat &format);

private slots:
    void readyReadPrivate();

private:
    QAudioInput *m_audio_input;
    QIODevice *m_device;
    QAudioFormat m_format;
};

#endif // AUDIOINPUT_H
