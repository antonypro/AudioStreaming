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
    ~AudioInput();

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
    QPointer<QAudioInput> m_audio_input;
    QPointer<QIODevice> m_device;
    QAudioFormat m_format;
    QAudioFormat m_supported_format;
};

#endif // AUDIOINPUT_H
