#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "audiostreaminglibcore.h"
#include "common.h"
#include "threadcommon.h"
#include "levelmeter.h"
#include "flowcontrol.h"

//\cond HIDDEN_SYMBOLS
class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput();

    CLASS_MEMBER_HEADER

signals:
    void veryOutputData(QByteArray);
    void currentlevel(float);
    void error(QString);

public slots:
    void start(const QAudioDeviceInfo &devinfo,
               const QAudioFormat &format,
               int time_to_buffer,
               bool is_very_output_enabled);

    void setVolume(int volume);
    void write(const QByteArray &data);

private slots:
    void startPrivate(const QAudioDeviceInfo &devinfo,
               const QAudioFormat &format,
               int time_to_buffer,
               bool is_very_output_enabled);

    void setVolumePrivate(int volume);
    void writePrivate(const QByteArray &data);
    void resampledData(const QByteArray &data);

    void verifyBuffer();
    void prePlay();
    void play();

private:
    bool m_initialized;
    QPointer<QAudioOutput> m_audio_output;
    QPointer<QIODevice> m_device;
    QPointer<r8brain> m_resampler;
    QByteArray m_buffer;
    bool m_is_get_very_output_enabled;
    bool m_buffer_requested;
    bool m_play_called;
    float m_volume;
    int m_size_to_buffer;
    int m_time_to_buffer;
    int m_last_size_to_buffer;
    int m_max_size_to_buffer;
    QAudioFormat m_format;
    QAudioFormat m_supported_format;
    QPointer<LevelMeter> m_level_meter;

#ifdef IS_TO_DEBUG
    qint64 m_index;
#endif

    //Smart buffer
    QElapsedTimer m_smart_buffer_timer;
    bool m_smart_buffer_enabled;
    bool m_smart_buffer_test_active;
    int m_bytes;
    int m_smart_bufer_min_size;
    int m_bytes_read;
};
//\endcond

#endif // AUDIOOUTPUT_H
