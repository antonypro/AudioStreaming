#ifndef AUDIOSTREAMINGLIBCORE_H
#define AUDIOSTREAMINGLIBCORE_H

#include <QtCore>
#include <QtMultimedia>
#include <QtNetwork>
#include "discoverclient.h"

class Worker;

#define MIN_BUFFERED_TIME 10
#define MAX_BUFFERED_TIME 100 //Time to buffer plus this value

#define MAX_ID_LENGTH 20

class StreamingInfo
{
public:
    StreamingInfo()
    {
        //Default values
        m_id = QString(MAX_ID_LENGTH, (char)0);
        m_work_mode = StreamingWorkMode::Undefined;
        m_time_to_buffer = 300;
        m_input_dev_type = AudioDeviceType::LibraryAudioDevice;
        m_output_dev_type = AudioDeviceType::LibraryAudioDevice;
        m_input = QAudioDeviceInfo::defaultInputDevice();
        m_output = QAudioDeviceInfo::defaultOutputDevice();
        m_opus_bitrate = 128 * 1000;
        m_callback_enabled = false;
        m_is_get_audio_enabled = false;
        m_ssl_enabled = false;
    }

    typedef enum StreamingWorkMode
    {
        Undefined,
        BroadcastClient,
        BroadcastServer,
        WalkieTalkieClient,
        WalkieTalkieServer
    }StreamingWorkMode;

    typedef enum AudioDeviceType
    {
        LibraryAudioDevice,
        CustomAudioDevice
    }AudioDeviceType;

    QByteArray negotiationString() const
    {
        return m_negotiation_string;
    }
    QString ID() const
    {
        return m_id;
    }
    StreamingWorkMode workMode() const
    {
        return m_work_mode;
    }
    qint32 timeToBuffer() const
    {
        return m_time_to_buffer;
    }
    AudioDeviceType inputDeviceType() const
    {
        return m_input_dev_type;
    }
    AudioDeviceType outputDeviceType() const
    {
        return m_output_dev_type;
    }
    QAudioFormat inputAudioFormat() const
    {
        return m_input_format;
    }
    QAudioFormat audioFormat() const
    {
        return m_format;
    }
    QAudioDeviceInfo inputDeviceInfo() const
    {
        return m_input;
    }
    QAudioDeviceInfo outputDeviceInfo() const
    {
        return m_output;
    }
    int OpusBitrate() const
    {
        return m_opus_bitrate;
    }
    bool isCallBackEnabled() const
    {
        return m_callback_enabled;
    }
    bool isGetAudioEnabled() const
    {
        return m_is_get_audio_enabled;
    }
    bool isSslEnabled() const
    {
        return m_ssl_enabled;
    }

    void setNegotiationString(const QByteArray &negotiation_string)
    {
        m_negotiation_string = negotiation_string;
    }
    void setID(const QString &id)
    {
        m_id = id.leftJustified(MAX_ID_LENGTH, (char)0, true);
    }
    void setWorkMode(const StreamingWorkMode &work_mode)
    {
        m_work_mode = work_mode;
    }
    void setTimeToBuffer(qint32 ms_time)
    {
        m_time_to_buffer = ms_time;
        if (m_time_to_buffer < MIN_BUFFERED_TIME)
            m_time_to_buffer = 0;
    }
    void setInputDeviceType(const AudioDeviceType &input_type)
    {
        m_input_dev_type = input_type;
    }
    void setOutputDeviceType(const AudioDeviceType &output_type)
    {
        m_output_dev_type = output_type;
    }
    void setInputAudioFormat(const QAudioFormat &format)
    {
        m_input_format = format;
        m_input_format.setCodec("audio/pcm");
    }
    void setAudioFormat(const QAudioFormat &format)
    {
        m_format = format;
        m_format.setCodec("audio/pcm");
    }
    void setInputDeviceInfo(const QAudioDeviceInfo &dev_info)
    {
        m_input = dev_info;
    }
    void setOutputDeviceInfo(const QAudioDeviceInfo &dev_info)
    {
        m_output = dev_info;
    }
    void setOpusBitrate(int bitrate)
    {
        m_opus_bitrate = bitrate;
    }
    void setCallBackEnabled(bool enable)
    {
        m_callback_enabled = enable;
    }
    void setGetAudioEnabled(bool enable)
    {
        m_is_get_audio_enabled = enable;
    }
    void setSslEnabled(bool enable)
    {
        m_ssl_enabled = enable;
    }

private:
    QByteArray m_negotiation_string;
    QString m_id;
    StreamingWorkMode m_work_mode;
    qint32 m_time_to_buffer;
    AudioDeviceType m_input_dev_type;
    AudioDeviceType m_output_dev_type;
    QAudioFormat m_input_format;
    QAudioFormat m_format;
    QAudioDeviceInfo m_input;
    QAudioDeviceInfo m_output;
    int m_opus_bitrate;
    bool m_callback_enabled;
    bool m_is_get_audio_enabled;
    bool m_ssl_enabled;
};

QDataStream &operator<<(QDataStream &stream, const StreamingInfo &info);
QDataStream &operator>>(QDataStream &stream, StreamingInfo &info);

class AudioStreamingLibCore : public QObject
{
    Q_OBJECT
public:
    explicit AudioStreamingLibCore(QObject *parent = 0);
    ~AudioStreamingLibCore();

    static bool generateAsymmetricKeys(QByteArray *private_key, QByteArray *public_key);

signals:
    void connected(QHostAddress,QString);
    void disconnected(QHostAddress);
    void pending(QHostAddress,QString);
    void inputData(QByteArray);
    void veryInputData(QByteArray);
    void outputData(QByteArray);
    void veryOutputData(QByteArray);
    void extraData(QByteArray);
    void inputLevel(qreal);
    void outputLevel(qreal);
    void adjustSettings();
    void extraDataWritten();
    void finished();
    void error(QString);

public slots:
    bool start(const StreamingInfo &streaming_info);
    void stop();
    bool isRunning() const;
    DiscoverClient *discoverInstance();
    void setKeys(const QByteArray &private_key, const QByteArray &public_key);
    void listen(quint16 port, bool auto_accept = true, const QByteArray &password = QByteArray(), int max_connections = 30);
    void connectToHost(const QString &host, quint16 port, const QByteArray &password = QByteArray());
    void acceptConnection();
    void rejectConnection();
    void writeExtraData(const QByteArray &data);
    void writeExtraDataResult();
    void inputDataBack(const QByteArray &data);
    void outputDataBack(const QByteArray &data);
    bool isInputMuted() const;
    void setInputMuted(bool mute);
    int volume() const;
    void setVolume(int volume);
    StreamingInfo streamingInfo() const;
    QList<QHostAddress> connectionsList() const;
    bool isReadyToWriteExtraData() const;
    QAudioFormat audioFormat() const;
    QAudioFormat inputAudioFormat() const;

private slots:
    void finishedPrivate();

private:
    bool m_input_muted;
    int m_volume;
    Worker *m_worker;
    DiscoverClient *m_client_discover;
    bool m_running;
    bool m_delete_pending;
};

#endif // AUDIOSTREAMINGLIBCORE_H
