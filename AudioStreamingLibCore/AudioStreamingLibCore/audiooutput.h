#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "audiostreaminglibcore.h"
#include "common.h"
#include "levelmeter.h"

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = 0);

signals:
    void veryOutputData(QByteArray);
    void currentlevel(qreal);
    void error(QString);

public slots:
    void start(const QAudioDeviceInfo &devinfo,
               const QAudioFormat &format,
               int time_to_buffer,
               bool is_very_output_enabled);

    void setVolume(int volume);
    void write(const QByteArray &data);

private slots:
    void verifyBuffer();
    void preplay();
    void play();

private:
    bool m_initialized;
    QAudioOutput *m_audio_output;
    QIODevice *m_device;
    QByteArray m_buffer;
    bool m_is_get_very_output_enabled;
    bool m_buffer_requested;
    bool m_play_called;
    int m_volume;
    int m_size_to_buffer;
    int m_time_to_buffer;
    int m_max_size_to_buffer;
    QAudioFormat m_format;
    LevelMeter *m_level_meter;
};

#endif // AUDIOOUTPUT_H
