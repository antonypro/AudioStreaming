#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "common.h"

class AudioInput : public QObject
{
    Q_OBJECT
public:
    explicit AudioInput(QObject *parent = nullptr);

signals:
    void error(QString);
    void readyRead(QByteArray);

public slots:
    void start(const QAudioDeviceInfo &devinfo,
               const QAudioFormat &format);

private slots:
    void startPrivate(const QAudioDeviceInfo &devinfo, const QAudioFormat &format);
    void readyReadPrivate();

private:
    QAudioInput *m_audio_input;
    QIODevice *m_device;
    QAudioFormat m_format;
    QAudioFormat m_supported_format;
};

#endif // AUDIOINPUT_H
