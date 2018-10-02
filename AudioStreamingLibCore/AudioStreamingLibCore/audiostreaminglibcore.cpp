#include "audiostreaminglibcore.h"
#include "audiostreamingworker.h"

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

    m_worker = new AudioStreamingWorker();
    QThread *m_thread = new QThread();
    m_worker->moveToThread(m_thread);

    connect(m_worker, &AudioStreamingWorker::destroyed, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

    SETTONULLPTR(m_worker);

    connect(m_worker, &AudioStreamingWorker::connected, this, &AudioStreamingLibCore::connected);
    connect(m_worker, &AudioStreamingWorker::connectedToServer, this, &AudioStreamingLibCore::connectedToServer);
    connect(m_worker, &AudioStreamingWorker::disconnected, this, &AudioStreamingLibCore::disconnected);
    connect(m_worker, &AudioStreamingWorker::pending, this, &AudioStreamingLibCore::pending);
    connect(m_worker, &AudioStreamingWorker::webClientLoggedIn, this, &AudioStreamingLibCore::webClientLoggedIn);
    connect(m_worker, &AudioStreamingWorker::webClientWarning, this, &AudioStreamingLibCore::webClientWarning);
    connect(m_worker, &AudioStreamingWorker::inputData, this, &AudioStreamingLibCore::inputData);
    connect(m_worker, &AudioStreamingWorker::veryInputData, this, &AudioStreamingLibCore::veryInputData);
    connect(m_worker, &AudioStreamingWorker::outputData, this, &AudioStreamingLibCore::outputData);
    connect(m_worker, &AudioStreamingWorker::veryOutputData, this, &AudioStreamingLibCore::veryOutputData);
    connect(m_worker, &AudioStreamingWorker::extraData, this, &AudioStreamingLibCore::extraData);
    connect(m_worker, &AudioStreamingWorker::inputLevel, this, &AudioStreamingLibCore::inputLevel);
    connect(m_worker, &AudioStreamingWorker::outputLevel, this, &AudioStreamingLibCore::outputLevel);
    connect(m_worker, &AudioStreamingWorker::adjustSettings, this, &AudioStreamingLibCore::adjustSettings);
    connect(m_worker, &AudioStreamingWorker::extraDataWritten, this, &AudioStreamingLibCore::extraDataWritten);
    connect(m_worker, &AudioStreamingWorker::error, this, &AudioStreamingLibCore::error);
    connect(m_worker, &AudioStreamingWorker::error, this, &AudioStreamingLibCore::stop);

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

        m_audio_format = QAudioFormat();
        m_input_format = QAudioFormat();

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

qint64 AudioStreamingLibCore::timeToSize(qint64 ms_time, int channel_count, int sample_size, int sample_rate)
{
    return ::timeToSize(ms_time, channel_count, sample_size, sample_rate);
}

qint64 AudioStreamingLibCore::timeToSize(qint64 ms_time, const QAudioFormat &format)
{
    return ::timeToSize(ms_time, format);
}

qint64 AudioStreamingLibCore::sizeToTime(qint64 bytes, int channel_count, int sample_size, int sample_rate)
{
    return ::sizeToTime(bytes, channel_count, sample_size, sample_rate);
}

qint64 AudioStreamingLibCore::sizeToTime(qint64 bytes, const QAudioFormat &format)
{
    return ::sizeToTime(bytes, format);
}

QByteArray AudioStreamingLibCore::convertFloatToInt16(const QByteArray &data)
{
    QAudioFormat format;
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);

    return convertSamplesToInt(data, format);
}

QByteArray AudioStreamingLibCore::convertInt16ToFloat(const QByteArray &data)
{
    QAudioFormat format;
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);

    return convertSamplesToFloat(data, format);
}

QByteArray AudioStreamingLibCore::mixFloatAudio(const QByteArray &data1, const QByteArray &data2)
{
    if (data1.isEmpty() || data2.isEmpty())
        return QByteArray();

    if (data1.size() != data2.size())
        return QByteArray();

    QByteArray data1cpy = data1;
    QByteArray data2cpy = data2;

    Eigen::Ref<Eigen::VectorXf> samples_float_1 = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(data1cpy.data()), data1cpy.size() / int(sizeof(float)));
    Eigen::Ref<Eigen::VectorXf> samples_float_2 = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(data2cpy.data()), data2cpy.size() / int(sizeof(float)));

    Eigen::VectorXf samples_mixed = samples_float_1 * 0.5f + samples_float_2 * 0.5f;

    return QByteArray(reinterpret_cast<char*>(samples_mixed.data()), samples_mixed.size() * int(sizeof(float)));
}

bool AudioStreamingLibCore::isRunning()
{
    return RUNNING;
}

DiscoverClient *AudioStreamingLibCore::discoverInstance()
{
    if (m_client_discover)
        m_client_discover->deleteLater();

    m_client_discover = new DiscoverClient(this);
    SETTONULLPTR(m_client_discover);
    QTimer::singleShot(1000, m_client_discover, &DiscoverClient::deleteLater);

    return m_client_discover;
}

void AudioStreamingLibCore::listen(quint16 port, bool auto_accept, const QByteArray &password, int max_connections)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "listen", Qt::QueuedConnection,
                              Q_ARG(quint16, port), Q_ARG(bool, auto_accept), Q_ARG(QByteArray, password), Q_ARG(int, qMax(max_connections, 1)));
}

void AudioStreamingLibCore::connectToHost(const QString &host, quint16 port, const QByteArray &password, bool new_user)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "connectToHost", Qt::QueuedConnection,
                              Q_ARG(QString, host), Q_ARG(quint16, port), Q_ARG(QByteArray, password), Q_ARG(bool, new_user));
}

void AudioStreamingLibCore::connectToPeer(const QString &ID)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "connectToPeer", Qt::QueuedConnection, Q_ARG(QString, ID));
}

void AudioStreamingLibCore::disconnectFromPeer()
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "disconnectFromPeer", Qt::QueuedConnection);
}

void AudioStreamingLibCore::acceptSslCertificate()
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "acceptSslCertificate", Qt::QueuedConnection);
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

bool AudioStreamingLibCore::isInputMuted()
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

int AudioStreamingLibCore::volume()
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

StreamingInfo AudioStreamingLibCore::streamingInfo()
{
    if (!RUNNING)
        return StreamingInfo();

    StreamingInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "streamingInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(StreamingInfo, streaming_info));
    return streaming_info;
}

QList<QHostAddress> AudioStreamingLibCore::connectionsList()
{
    if (!RUNNING)
        return QList<QHostAddress>();

    QList<QHostAddress> connections;
    QMetaObject::invokeMethod(m_worker, "connectionsList", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QList<QHostAddress>, connections));
    return connections;
}

bool AudioStreamingLibCore::isReadyToWriteExtraData()
{
    if (!RUNNING)
        return false;

    bool isReady = false;
    QMetaObject::invokeMethod(m_worker, "isReadyToWriteExtraData", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isReady));
    return isReady;
}

QAudioFormat AudioStreamingLibCore::audioFormat()
{
    if (!RUNNING)
        return QAudioFormat();

    if (m_audio_format.isValid())
        return m_audio_format;

    StreamingInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "streamingInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(StreamingInfo, streaming_info));
    m_audio_format = streaming_info.audioFormat();

    return m_audio_format;
}

QAudioFormat AudioStreamingLibCore::inputAudioFormat()
{
    if (!RUNNING)
        return QAudioFormat();

    if (m_input_format.isValid())
        return m_input_format;

    StreamingInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "streamingInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(StreamingInfo, streaming_info));
    m_input_format = streaming_info.inputAudioFormat();

    return m_input_format;
}

QDataStream &operator<<(QDataStream &stream, StreamingInfo &info)
{
    stream << info.negotiationString();

    //Serialize helpers
    stream << qint32(info.workMode());
    stream << qint32(info.timeToBuffer());
    stream << qint32(info.inputDeviceType());
    stream << qint32(info.outputDeviceType());
    stream << info.isCallBackEnabled();
    stream << info.isEncryptionEnabled();

    //Serialize audio format
    stream << qint32(info.audioFormat().sampleSize());
    stream << qint32(info.audioFormat().sampleRate());
    stream << qint32(info.audioFormat().channelCount());
    stream << qint32(info.audioFormat().sampleType());
    stream << qint32(info.audioFormat().byteOrder());

    //Serialize input audio format
    stream << qint32(info.inputAudioFormat().sampleSize());
    stream << qint32(info.inputAudioFormat().sampleRate());
    stream << qint32(info.inputAudioFormat().channelCount());
    stream << qint32(info.inputAudioFormat().sampleType());
    stream << qint32(info.inputAudioFormat().byteOrder());

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

    bool isEncryptionEnabled;
    stream >> isEncryptionEnabled;

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
    format.setSampleType(QAudioFormat::SampleType(sampleType));
    format.setByteOrder(QAudioFormat::Endian(byteOrder));

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
    inputformat.setSampleType(QAudioFormat::SampleType(inputsampleType));
    inputformat.setByteOrder(QAudioFormat::Endian(inputbyteOrder));

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
    info.setWorkMode(StreamingInfo::StreamingWorkMode(workMode));
    info.setTimeToBuffer(timeToBuffer);
    info.setInputDeviceType(StreamingInfo::AudioDeviceType(inputDeviceType));
    info.setOutputDeviceType(StreamingInfo::AudioDeviceType(outputDeviceType));
    info.setInputAudioFormat(inputformat);
    info.setAudioFormat(format);
    info.setInputDeviceInfo(inputinfo);
    info.setOutputDeviceInfo(outputinfo);
    info.setCallBackEnabled(isCallBackEnabled);
    info.setEncryptionEnabled(isEncryptionEnabled);

    return stream;
}

QDebug operator<<(QDebug debug, StreamingInfo info)
{
    debug << "Negotiation string:" << qPrintable(info.negotiationString().trimmed()) << endl;

    debug << "ID:" << qPrintable(info.ID().trimmed()) << endl;

    debug << "Streaming work mode:";

    switch (info.workMode())
    {
    case StreamingInfo::StreamingWorkMode::Undefined:
        debug << "Undefined" << endl;
        break;
    case StreamingInfo::StreamingWorkMode::BroadcastClient:
        debug << "BroadcastClient" << endl;
        break;
    case StreamingInfo::StreamingWorkMode::BroadcastServer:
        debug << "BroadcastServer" << endl;
        break;
    case StreamingInfo::StreamingWorkMode::WalkieTalkieClient:
        debug << "WalkieTalkieClient" << endl;
        break;
    case StreamingInfo::StreamingWorkMode::WalkieTalkieServer:
        debug << "WalkieTalkieServer" << endl;
        break;
    default:
        break;
    }

    debug << "Time to buffer:" << info.timeToBuffer() << "ms" << endl;

    debug << "Input device type:";

    switch (info.inputDeviceType())
    {
    case StreamingInfo::AudioDeviceType::LibraryAudioDevice:
        debug << "LibraryAudioDevice" << endl;
        break;
    case StreamingInfo::AudioDeviceType::CustomAudioDevice:
        debug << "CustomAudioDevice" << endl;
        break;
    }

    debug << "Output device type:";

    switch (info.outputDeviceType())
    {
    case StreamingInfo::AudioDeviceType::LibraryAudioDevice:
        debug << "LibraryAudioDevice" << endl;
        break;
    case StreamingInfo::AudioDeviceType::CustomAudioDevice:
        debug << "CustomAudioDevice" << endl;
        break;
    }

    debug << "Input audio format:" << info.inputAudioFormat() << endl;

    debug << "Resampled audio format:" << info.audioFormat() << endl;

    debug << "Input device name:" << qPrintable(info.inputDeviceInfo().deviceName()) << endl;

    debug << "Output device name:" << qPrintable(info.outputDeviceInfo().deviceName()) << endl;

#ifdef OPUS
    debug << "Opus bitrate:" << info.OpusBitrate() << endl;
#else
    debug << "Opus bitrate:" << "N/A" << endl;
#endif

    debug << "Call back enabled:" << info.isCallBackEnabled() << endl;

    debug << "Get audio enabled:" << info.isGetAudioEnabled() << endl;

    debug << "Encryption enabled:" << info.isEncryptionEnabled() << endl;

    return debug;
}
