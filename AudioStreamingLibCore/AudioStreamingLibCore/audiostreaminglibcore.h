#ifndef AUDIOSTREAMINGLIBCORE_H
#define AUDIOSTREAMINGLIBCORE_H

#include <QtCore>
#include <QtMultimedia>
#include <QtNetwork>
#include "discoverclient.h"

class AudioStreamingWorker;

#define MIN_BUFFERED_TIME 10
#define MAX_BUFFERED_TIME 1000 //Time to buffer plus this value

#define MAX_ID_LENGTH 20

class StreamingInfo
{
public:
    StreamingInfo()
    {
        //Default values
        m_id = QString(MAX_ID_LENGTH, char(0));
        m_work_mode = StreamingWorkMode::Undefined;
        m_time_to_buffer = 0;
        m_input_dev_type = AudioDeviceType::LibraryAudioDevice;
        m_output_dev_type = AudioDeviceType::LibraryAudioDevice;
        m_input = QAudioDeviceInfo::defaultInputDevice();
        m_output = QAudioDeviceInfo::defaultOutputDevice();
        m_opus_bitrate = 128 * 1000;
        m_callback_enabled = false;
        m_is_get_audio_enabled = false;
        m_encryption_enabled = false;
    }

    enum class StreamingWorkMode
    {
        Undefined,
        BroadcastClient,
        BroadcastServer,
        WalkieTalkieClient,
        WalkieTalkieServer,
        WebClient
    };

    enum class AudioDeviceType
    {
        LibraryAudioDevice,
        CustomAudioDevice
    };

    QByteArray negotiationString()
    {
        return m_negotiation_string;
    }
    QString ID()
    {
        return m_id;
    }
    StreamingWorkMode workMode()
    {
        return m_work_mode;
    }
    qint32 timeToBuffer()
    {
        return m_time_to_buffer;
    }
    AudioDeviceType inputDeviceType()
    {
        return m_input_dev_type;
    }
    AudioDeviceType outputDeviceType()
    {
        return m_output_dev_type;
    }
    QAudioFormat inputAudioFormat()
    {
        return m_input_format;
    }
    QAudioFormat audioFormat()
    {
        return m_format;
    }
    QAudioDeviceInfo inputDeviceInfo()
    {
        return m_input;
    }
    QAudioDeviceInfo outputDeviceInfo()
    {
        return m_output;
    }
    int OpusBitrate()
    {
        return m_opus_bitrate;
    }
    bool isCallBackEnabled()
    {
        return m_callback_enabled;
    }
    bool isGetAudioEnabled()
    {
        return m_is_get_audio_enabled;
    }
    bool isEncryptionEnabled()
    {
        return m_encryption_enabled;
    }

    void setNegotiationString(const QByteArray &negotiation_string)
    {
        m_negotiation_string = negotiation_string;
    }
    void setID(const QString &id)
    {
        m_id = id.leftJustified(MAX_ID_LENGTH, char(0), true);
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
    void setEncryptionEnabled(bool enable)
    {
        m_encryption_enabled = enable;
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
    bool m_encryption_enabled;
};

QDataStream &operator<<(QDataStream &stream, StreamingInfo &info);
QDataStream &operator>>(QDataStream &stream, StreamingInfo &info);

QDebug operator<<(QDebug debug, StreamingInfo info);

class AudioStreamingLibCore : public QObject
{
    Q_OBJECT
public:
    explicit AudioStreamingLibCore(QObject *parent = nullptr);
    ~AudioStreamingLibCore();

    static qint64 timeToSize(qint64 ms_time, int channel_count, int sample_size, int sample_rate);
    static qint64 timeToSize(qint64 ms_time, const QAudioFormat &format);
    static qint64 sizeToTime(qint64 bytes, int channel_count, int sample_size, int sample_rate);
    static qint64 sizeToTime(qint64 bytes, const QAudioFormat &format);

    static QByteArray convertFloatToInt16(const QByteArray &data);
    static QByteArray convertInt16ToFloat(const QByteArray &data);

    static QByteArray mixFloatAudio(const QByteArray &data1, const QByteArray &data2);

signals:
    void connected(QHostAddress,QString);
    void connectedToServer(QByteArray);
    void disconnected(QHostAddress);
    void pending(QHostAddress,QString);
    void webClientLoggedIn();
    void webClientWarning(QString);
    void inputData(QByteArray);
    void veryInputData(QByteArray);
    void outputData(QByteArray);
    void veryOutputData(QByteArray);
    void extraData(QByteArray);
    void inputLevel(float);
    void outputLevel(float);
    void adjustSettings();
    void extraDataWritten();
    void finished();
    void error(QString);

public slots:
    bool start(const StreamingInfo &streaming_info);
    void stop();
    bool isRunning();
    DiscoverClient *discoverInstance();
    void listen(quint16 port, bool auto_accept = true, const QByteArray &password = QByteArray(), int max_connections = 30);
    void connectToHost(const QString &host, quint16 port, const QByteArray &password = QByteArray(), bool new_user = false);
    void connectToPeer(const QString &ID);
    void disconnectFromPeer();
    void acceptSslCertificate();
    void acceptConnection();
    void rejectConnection();
    void writeExtraData(const QByteArray &data);
    void writeExtraDataResult();
    void inputDataBack(const QByteArray &data);
    void outputDataBack(const QByteArray &data);
    bool isInputMuted();
    void setInputMuted(bool mute);
    int volume();
    void setVolume(int volume);
    StreamingInfo streamingInfo();
    QList<QHostAddress> connectionsList();
    bool isReadyToWriteExtraData();
    QAudioFormat audioFormat();
    QAudioFormat inputAudioFormat();

private slots:
    void finishedPrivate();

private:
    bool m_input_muted;
    int m_volume;
    AudioStreamingWorker *m_worker;
    DiscoverClient *m_client_discover;
    bool m_running;
    bool m_delete_pending;
    QAudioFormat m_audio_format;
    QAudioFormat m_input_format;
};

#endif // AUDIOSTREAMINGLIBCORE_H
