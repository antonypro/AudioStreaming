#include "audiostreaminglibcore.h"
#include "audiostreamingworker.h"

#define RUNNING (m_running && m_worker)

int m_worker_count = 0;
QMutex m_worker_mutex;
QSemaphore m_worker_semaphore;

AudioStreamingLibCore::AudioStreamingLibCore(QObject *parent) : QObject(parent)
{
    m_worker = nullptr;

    m_input_muted = false;
    m_volume = 0;

    m_running = false;

    qRegisterMetaType<AudioStreamingLibInfo>("AudioStreamingLibInfo");
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

AudioStreamingLibCore::~AudioStreamingLibCore()
{
    stop();
}

bool AudioStreamingLibCore::start(const AudioStreamingLibInfo &streaming_info)
{
    if (m_running)
        return false;

    m_worker = new AudioStreamingWorker(this);

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
    connect(m_worker, &AudioStreamingWorker::commandXML, this, &AudioStreamingLibCore::commandXML);
    connect(m_worker, &AudioStreamingWorker::inputLevel, this, &AudioStreamingLibCore::inputLevel);
    connect(m_worker, &AudioStreamingWorker::outputLevel, this, &AudioStreamingLibCore::outputLevel);
    connect(m_worker, &AudioStreamingWorker::adjustSettings, this, &AudioStreamingLibCore::adjustSettings);
    connect(m_worker, &AudioStreamingWorker::extraDataWritten, this, &AudioStreamingLibCore::extraDataWritten);
    connect(m_worker, &AudioStreamingWorker::warning, this, &AudioStreamingLibCore::warning);
    connect(m_worker, &AudioStreamingWorker::error, this, &AudioStreamingLibCore::error);
    connect(m_worker, &AudioStreamingWorker::error, this, &AudioStreamingLibCore::stop);

    QMetaObject::invokeMethod(m_worker, "start", Qt::QueuedConnection, Q_ARG(AudioStreamingLibInfo, streaming_info));

    QMetaObject::invokeMethod(m_worker, "setVolume", Qt::QueuedConnection, Q_ARG(int, m_volume));
    QMetaObject::invokeMethod(m_worker, "setInputMuted", Qt::QueuedConnection, Q_ARG(bool, m_input_muted));

    m_running = true;

    return true;
}

void AudioStreamingLibCore::stop()
{
    if (!RUNNING)
        return;

    m_running = false;

    m_audio_format = QAudioFormat();
    m_input_format = QAudioFormat();

    m_worker->deleteLater();

    m_worker_semaphore.acquire();

    m_worker = nullptr;

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

    QByteArray new_data;
    new_data.resize(data.size());

    const float *datafloat = reinterpret_cast<const float*>(data.constData());

    float *newfloat = reinterpret_cast<float*>(new_data.data());

    int size = data.size() / int(sizeof(float));

    for (int i = 0; i < size; i++)
    {
        newfloat[i] = qBound(float(-1), datafloat[i], float(1));
    }

    return convertSamplesToInt(new_data, format);
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

    QByteArray mixed;
    mixed.resize(data1.size());

    const float *data1float = reinterpret_cast<const float*>(data1.constData());
    const float *data2float = reinterpret_cast<const float*>(data2.constData());

    float *mixedfloat = reinterpret_cast<float*>(mixed.data());

    int size = data1.size() / int(sizeof(float));

    for (int i = 0; i < size; i++)
    {
        if ((data1float[i] < 0 && data2float[i] < 0) || (data1float[i] > 0 && data2float[i] > 0))
        {
            mixedfloat[i] = (data1float[i] + data2float[i]) - (data1float[i] * data2float[i]);
        }
        else
        {
            mixedfloat[i] = data1float[i] + data2float[i];
        }

        mixedfloat[i] = qBound(float(-1), mixedfloat[i], float(1));
    }

    return mixed;
}

QString AudioStreamingLibCore::EigenInstructionsSet()
{
    return QString(Eigen::SimdInstructionSetsInUse());
}

bool AudioStreamingLibCore::isRunning()
{
    return RUNNING;
}

void AudioStreamingLibCore::changeInputDevice(const QAudioDeviceInfo &dev_info)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "changeInputDevice", Qt::QueuedConnection, Q_ARG(QAudioDeviceInfo, dev_info));
}

void AudioStreamingLibCore::changeOutputDevice(const QAudioDeviceInfo &dev_info)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "changeOutputDevice", Qt::QueuedConnection, Q_ARG(QAudioDeviceInfo, dev_info));
}

DiscoverClient *AudioStreamingLibCore::discoverInstance(int time_to_destroy)
{
    if (m_client_discover)
        m_client_discover->deleteLater();

    m_client_discover = new DiscoverClient(this);

    time_to_destroy = qMax(time_to_destroy, 1000);

    QTimer::singleShot(time_to_destroy, m_client_discover, &DiscoverClient::deleteLater);

    return m_client_discover;
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

void AudioStreamingLibCore::writeCommandXML(const QByteArray &XML)
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "writeCommandXML", Qt::QueuedConnection, Q_ARG(QByteArray, XML));
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

void AudioStreamingLibCore::acceptSslCertificate()
{
    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "acceptSslCertificate", Qt::QueuedConnection);
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
    m_volume = qBound(0, volume, 150);

    if (!RUNNING)
        return;

    QMetaObject::invokeMethod(m_worker, "setVolume", Qt::QueuedConnection, Q_ARG(int, m_volume));
}

AudioStreamingLibInfo AudioStreamingLibCore::audioStreamingLibInfo()
{
    if (!RUNNING)
        return AudioStreamingLibInfo();

    AudioStreamingLibInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "audioStreamingLibInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(AudioStreamingLibInfo, streaming_info));
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

    AudioStreamingLibInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "audioStreamingLibInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(AudioStreamingLibInfo, streaming_info));
    m_audio_format = streaming_info.audioFormat();

    return m_audio_format;
}

QAudioFormat AudioStreamingLibCore::inputAudioFormat()
{
    if (!RUNNING)
        return QAudioFormat();

    if (m_input_format.isValid())
        return m_input_format;

    AudioStreamingLibInfo streaming_info;
    QMetaObject::invokeMethod(m_worker, "audioStreamingLibInfo", Qt::BlockingQueuedConnection, Q_RETURN_ARG(AudioStreamingLibInfo, streaming_info));
    m_input_format = streaming_info.inputAudioFormat();

    return m_input_format;
}

QDataStream &operator<<(QDataStream &stream, AudioStreamingLibInfo &info)
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

QDataStream &operator>>(QDataStream &stream, AudioStreamingLibInfo &info)
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

    for (const QAudioDeviceInfo &audio_info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
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

    for (const QAudioDeviceInfo &audio_info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if (audio_info.deviceName() == outputdeviceName)
        {
            outputinfo = audio_info;
            break;
        }
    }

    info.setNegotiationString(negotiationString);
    info.setWorkMode(AudioStreamingLibInfo::StreamingWorkMode(workMode));
    info.setTimeToBuffer(timeToBuffer);
    info.setInputDeviceType(AudioStreamingLibInfo::AudioDeviceType(inputDeviceType));
    info.setOutputDeviceType(AudioStreamingLibInfo::AudioDeviceType(outputDeviceType));
    info.setInputAudioFormat(inputformat);
    info.setAudioFormat(format);
    info.setInputDeviceInfo(inputinfo);
    info.setOutputDeviceInfo(outputinfo);
    info.setCallBackEnabled(isCallBackEnabled);
    info.setEncryptionEnabled(isEncryptionEnabled);

    return stream;
}

QDebug &operator<<(QDebug &debug, AudioStreamingLibInfo &info)
{
    debug << "Negotiation string:" << qPrintable(info.negotiationString().trimmed()) << endl;

    debug << "ID:" << qPrintable(info.ID().trimmed()) << endl;

    debug << "Streaming work mode:";

    switch (info.workMode())
    {
    case AudioStreamingLibInfo::StreamingWorkMode::Undefined:
        debug << "Undefined" << endl;
        break;
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient:
        debug << "BroadcastClient" << endl;
        break;
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer:
        debug << "BroadcastServer" << endl;
        break;
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient:
        debug << "WalkieTalkieClient" << endl;
        break;
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer:
        debug << "WalkieTalkieServer" << endl;
        break;
    default:
        break;
    }

    debug << "Time to buffer:" << info.timeToBuffer() << "ms" << endl;

    debug << "Input device type:";

    switch (info.inputDeviceType())
    {
    case AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice:
        debug << "LibraryAudioDevice" << endl;
        break;
    case AudioStreamingLibInfo::AudioDeviceType::CustomAudioDevice:
        debug << "CustomAudioDevice" << endl;
        break;
    }

    debug << "Output device type:";

    switch (info.outputDeviceType())
    {
    case AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice:
        debug << "LibraryAudioDevice" << endl;
        break;
    case AudioStreamingLibInfo::AudioDeviceType::CustomAudioDevice:
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
