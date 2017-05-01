#include "audiostreaminglibcore.h"
#include "worker.h"

#define RUNNING (m_running && m_worker)

AudioStreamingLibCore::AudioStreamingLibCore(QObject *parent) : QObject(parent)
{
    m_worker = nullptr;
    m_client_discover = nullptr;

    m_input_muted = false;
    m_volume = 0;

    m_running = false;
    m_delete_pending = false;

    qRegisterMetaType<StreamingInfo>("StreamingInfo");
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

AudioStreamingLibCore::~AudioStreamingLibCore()
{
    if (!RUNNING)
        return;

    stop();

    finishedPrivate();
}

bool AudioStreamingLibCore::start(const StreamingInfo &streaming_info)
{
    if (m_running)
        return false;

    m_worker = new Worker();
    QThread *m_thread = new QThread();
    m_worker->moveToThread(m_thread);

    connect(m_worker, &Worker::destroyed, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

    setTonullptr(m_worker);

    connect(m_worker, &Worker::connected, this, &AudioStreamingLibCore::connected);
    connect(m_worker, &Worker::disconnected, this, &AudioStreamingLibCore::disconnected);
    connect(m_worker, &Worker::pending, this, &AudioStreamingLibCore::pending);
    connect(m_worker, &Worker::inputData, this, &AudioStreamingLibCore::inputData);
    connect(m_worker, &Worker::veryInputData, this, &AudioStreamingLibCore::veryInputData);
    connect(m_worker, &Worker::outputData, this, &AudioStreamingLibCore::outputData);
    connect(m_worker, &Worker::veryOutputData, this, &AudioStreamingLibCore::veryOutputData);
    connect(m_worker, &Worker::extraData, this, &AudioStreamingLibCore::extraData);
    connect(m_worker, &Worker::inputLevel, this, &AudioStreamingLibCore::inputLevel);
    connect(m_worker, &Worker::outputLevel, this, &AudioStreamingLibCore::outputLevel);
    connect(m_worker, &Worker::adjustSettings, this, &AudioStreamingLibCore::adjustSettings);
    connect(m_worker, &Worker::extraDataWritten, this, &AudioStreamingLibCore::extraDataWritten);
    connect(m_worker, &Worker::error, this, &AudioStreamingLibCore::error);
    connect(m_worker, &Worker::error, this, &AudioStreamingLibCore::stop);

    QMetaObject::invokeMethod(m_worker, "start", Qt::QueuedConnection,
                              Q_ARG(StreamingInfo, streaming_info));

    QMetaObject::invokeMethod(m_worker, "setVolume", Qt::QueuedConnection, Q_ARG(int, m_volume));
    QMetaObject::invokeMethod(m_worker, "setInputMuted", Qt::QueuedConnection, Q_ARG(bool, m_input_muted));

    m_thread->start();

    m_running = true;

    return true;
}

void AudioStreamingLibCore::stop()
{
    if (RUNNING && !m_delete_pending)
    {
        m_running = false;

        m_delete_pending = true;

        m_worker->deleteLater();

        QThread::msleep(100);

        finishedPrivate();
    }
}

void AudioStreamingLibCore::finishedPrivate()
{
    if (!m_delete_pending)
        return;

    m_delete_pending = false;

    emit finished();
}

bool AudioStreamingLibCore::generateAsymmetricKeys(QByteArray *private_key, QByteArray *public_key)
{
    return OpenSslLib::generateKeys((SecureByteArray*)private_key, (SecureByteArray*)public_key);
}

bool AudioStreamingLibCore::isRunning() const
{
    return RUNNING;
}

DiscoverClient *AudioStreamingLibCore::discoverInstance()
{
    if (m_client_discover)
        m_client_discover->deleteLater();

    m_client_discover = new DiscoverClient(this);
    setTonullptr(m_client_discover);
    QTimer::singleShot(1000, m_client_discover, &DiscoverClient::deleteLater);

    return m_client_discover;
}

void AudioStreamingLibCore::setKeys(const QByteArray &private_key, const QByteArray &public_key)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "setKeys", Qt::QueuedConnection,
                              Q_ARG(QByteArray, private_key), Q_ARG(QByteArray, public_key));
}

void AudioStreamingLibCore::listen(quint16 port, bool auto_accept, const QByteArray &password, int max_connections)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "listen", Qt::QueuedConnection,
                              Q_ARG(quint16, port), Q_ARG(bool, auto_accept), Q_ARG(QByteArray, password), Q_ARG(int, qMax(max_connections, 1)));
}

void AudioStreamingLibCore::connectToHost(const QString &host, quint16 port, const QByteArray &password)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "connectToHost", Qt::QueuedConnection,
                              Q_ARG(QString, host), Q_ARG(quint16, port), Q_ARG(QByteArray, password));
}

void AudioStreamingLibCore::acceptConnection()
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "acceptConnection", Qt::QueuedConnection);
}

void AudioStreamingLibCore::rejectConnection()
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "rejectConnection", Qt::QueuedConnection);
}

void AudioStreamingLibCore::writeExtraData(const QByteArray &data)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "writeExtraData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void AudioStreamingLibCore::writeExtraDataResult()
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "writeExtraDataResult", Qt::QueuedConnection);
}

void AudioStreamingLibCore::inputDataBack(const QByteArray &data)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "inputDataBack", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void AudioStreamingLibCore::outputDataBack(const QByteArray &data)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "outputDataBack", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

bool AudioStreamingLibCore::isInputMuted() const
{
    if (!RUNNING)
        return false;

    bool m_input_muted;
    QMetaObject::invokeMethod(m_worker, "isInputMuted", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, m_input_muted));
    return m_input_muted;
}

void AudioStreamingLibCore::setInputMuted(bool mute)
{
    m_input_muted = mute;

    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "setInputMuted", Qt::QueuedConnection, Q_ARG(bool, m_input_muted));
}

int AudioStreamingLibCore::volume() const
{
    if (!RUNNING)
        return 0;

    int m_volume;
    QMetaObject::invokeMethod(m_worker, "volume", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, m_volume));
    return m_volume;
}

void AudioStreamingLibCore::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);

    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "setVolume", Qt::QueuedConnection, Q_ARG(int, m_volume));
}

StreamingInfo AudioStreamingLibCore::streamingInfo() const
{
    if (!RUNNING)
        return StreamingInfo();

    StreamingInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "streamingInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(StreamingInfo, streaming_info));
    return streaming_info;
}

QList<QHostAddress> AudioStreamingLibCore::connectionsList() const
{
    if (!RUNNING)
        return QList<QHostAddress>();

    QList<QHostAddress> connections;
    QMetaObject::invokeMethod(m_worker, "connectionsList", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QList<QHostAddress>, connections));
    return connections;
}

bool AudioStreamingLibCore::isReadyToWriteExtraData() const
{
    if (!RUNNING)
        return false;

    bool isReady = false;
    QMetaObject::invokeMethod(m_worker, "isReadyToWriteExtraData", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isReady));
    return isReady;
}

QAudioFormat AudioStreamingLibCore::audioFormat() const
{
    if (!RUNNING)
        return QAudioFormat();

    StreamingInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "streamingInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(StreamingInfo, streaming_info));
    return streaming_info.audioFormat();
}

QAudioFormat AudioStreamingLibCore::inputAudioFormat() const
{
    if (!RUNNING)
        return QAudioFormat();

    StreamingInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "streamingInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(StreamingInfo, streaming_info));
    return streaming_info.inputAudioFormat();
}

QDataStream &operator<<(QDataStream &stream, const StreamingInfo &info)
{
    stream << info.negotiationString();

    //Serialize helpers
    stream << (qint32)info.workMode();
    stream << (qint32)info.timeToBuffer();
    stream << (qint32)info.inputDeviceType();
    stream << (qint32)info.outputDeviceType();
    stream << info.isCallBackEnabled();
    stream << info.isSslEnabled();

    //Serialize audio format
    stream << (qint32)info.audioFormat().sampleSize();
    stream << (qint32)info.audioFormat().sampleRate();
    stream << (qint32)info.audioFormat().channelCount();
    stream << (qint32)info.audioFormat().sampleType();
    stream << (qint32)info.audioFormat().byteOrder();

    //Serialize input audio format
    stream << (qint32)info.inputAudioFormat().sampleSize();
    stream << (qint32)info.inputAudioFormat().sampleRate();
    stream << (qint32)info.inputAudioFormat().channelCount();
    stream << (qint32)info.inputAudioFormat().sampleType();
    stream << (qint32)info.inputAudioFormat().byteOrder();

    //Serialize input device info
    stream << info.inputDeviceInfo().deviceName();

    //Serialize output device info
    stream << info.outputDeviceInfo().deviceName();

    return stream;
}

QDataStream &operator>>(QDataStream &stream, StreamingInfo &info)
{
    QByteArray negotiationString;
    stream >> negotiationString;

    //Serialize helpers
    qint32 workMode;
    stream >> workMode;

    qint32 timeToBuffer;
    stream >> timeToBuffer;

    qint32 inputDeviceType;
    stream >> inputDeviceType;

    qint32 outputDeviceType;
    stream >> outputDeviceType;

    bool isCallBackEnabled;
    stream >> isCallBackEnabled;

    bool isSslEnabled;
    stream >> isSslEnabled;

    //Serialize audio format
    qint32 sampleSize;
    stream >> sampleSize;
    qint32 sampleRate;
    stream >> sampleRate;
    qint32 channelCount;
    stream >> channelCount;
    qint32 sampleType;
    stream >> sampleType;
    qint32 byteOrder;
    stream >> byteOrder;

    QAudioFormat format;
    format.setSampleSize(sampleSize);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
    format.setSampleType((QAudioFormat::SampleType)sampleType);
    format.setByteOrder((QAudioFormat::Endian)byteOrder);

    //Serialize input audio format
    qint32 inputsampleSize;
    stream >> inputsampleSize;
    qint32 inputsampleRate;
    stream >> inputsampleRate;
    qint32 inputchannelCount;
    stream >> inputchannelCount;
    qint32 inputsampleType;
    stream >> inputsampleType;
    qint32 inputbyteOrder;
    stream >> inputbyteOrder;

    QAudioFormat inputformat;
    inputformat.setSampleSize(inputsampleSize);
    inputformat.setSampleRate(inputsampleRate);
    inputformat.setChannelCount(inputchannelCount);
    inputformat.setSampleType((QAudioFormat::SampleType)inputsampleType);
    inputformat.setByteOrder((QAudioFormat::Endian)inputbyteOrder);

    //Serialize input device info
    QString inputdeviceName;
    stream >> inputdeviceName;

    QAudioDeviceInfo inputinfo;

    foreach (const QAudioDeviceInfo &audio_info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        if (audio_info.deviceName() == inputdeviceName)
        {
            inputinfo = audio_info;
            break;
        }
    }

    //Serialize output device info
    QString outputdeviceName;
    stream >> outputdeviceName;

    QAudioDeviceInfo outputinfo;

    foreach (const QAudioDeviceInfo &audio_info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if (audio_info.deviceName() == outputdeviceName)
        {
            outputinfo = audio_info;
            break;
        }
    }

    info.setNegotiationString(negotiationString);
    info.setWorkMode((StreamingInfo::StreamingWorkMode)workMode);
    info.setTimeToBuffer(timeToBuffer);
    info.setInputDeviceType((StreamingInfo::AudioDeviceType)inputDeviceType);
    info.setOutputDeviceType((StreamingInfo::AudioDeviceType)outputDeviceType);
    info.setInputAudioFormat(inputformat);
    info.setAudioFormat(format);
    info.setInputDeviceInfo(inputinfo);
    info.setOutputDeviceInfo(outputinfo);
    info.setCallBackEnabled(isCallBackEnabled);
    info.setSslEnabled(isSslEnabled);

    return stream;
}
