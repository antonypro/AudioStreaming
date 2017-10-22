#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtMultimedia>
#include <eigen3/Eigen/Eigen>

#define setTonullptr(obj) QObject::connect(obj, &QObject::destroyed, [=]{obj = nullptr;})

#ifdef OPUS

#define BUFFER_LEN 1024*1024; //Used by resampler

#define MAX_PACKET_SIZE 3*1276

#endif //OPUS

#define MAX_NETWORK_CHUNK_SIZE 10*1024*1024

#define START_THREAD \
{\
QThread *thread = new QThread();\
\
moveToThread(thread);\
\
connect(this, &QObject::destroyed, thread, &QThread::quit);\
connect(thread, &QThread::finished, thread, &QThread::deleteLater);\
\
thread->start();\
}

namespace Command
{
enum
{
    AudioHeader,
    AudioData,
    AudioDataReceived,
    RemoteBufferTime,
    ExtraData,
    ExtraDataReceived
};
}

template <typename T>
static inline QByteArray getBytes(T input)
{
    QByteArray tmp;
    QDataStream data(&tmp, QIODevice::WriteOnly);
    data << input;
    return tmp;
}

template <typename T>
static inline T getValue(QByteArray bytes)
{
    T tmp;
    QDataStream data(&bytes, QIODevice::ReadOnly);
    data >> tmp;
    return tmp;
}

static inline qint64 nanoTimeToSize(qint64 nano_time, int channel_count, int sample_size, int sample_rate)
{
    qint64 value = (channel_count * (sample_size / 8) * sample_rate) * nano_time / (qint64)qPow(1000, 3);

    qint64 numToRound = value;
    qint64 multiple = channel_count * (sample_size / 8);

    if (multiple == 0)
        return numToRound;

    qint64 remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

static inline qint64 timeToSize(qint64 ms_time, int channel_count, int sample_size, int sample_rate)
{
    return nanoTimeToSize(ms_time * (qint64)qPow(1000, 2), channel_count, sample_size, sample_rate);
}

static inline qint64 timeToSize(qint64 ms_time, const QAudioFormat &format)
{
    return timeToSize(ms_time, format.channelCount(), format.sampleSize(), format.sampleRate());
}

static inline qint64 sizeToNanoTime(qint64 bytes, int channel_count, int sample_size, int sample_rate)
{
    return (bytes * (qint64)qPow(1000, 3) / ((sample_size / 8) * channel_count * sample_rate));
}

static inline qint64 sizeToTime(qint64 bytes, int channel_count, int sample_size, int sample_rate)
{
    return sizeToNanoTime(bytes, channel_count, sample_size, sample_rate) / (qint64)qPow(1000, 2);
}

static inline qint64 sizeToTime(qint64 bytes, const QAudioFormat &format)
{
    return sizeToTime(bytes, format.channelCount(), format.sampleSize(), format.sampleRate());
}

typedef struct PeerData {
    QByteArray data;
    QHostAddress host;
    qintptr descriptor;
} PeerData;

typedef Eigen::Matrix<quint8, Eigen::Dynamic, 1> VectorXuint8;
typedef Eigen::Matrix<qint8, Eigen::Dynamic, 1> VectorXint8;
typedef Eigen::Matrix<qint16, Eigen::Dynamic, 1> VectorXint16;
typedef Eigen::Matrix<qint32, Eigen::Dynamic, 1> VectorXint32;

static inline QByteArray convertSamplesToInt(const QByteArray &data, const QAudioFormat &supported_format)
{
    QByteArray input = data;

    switch (supported_format.sampleSize())
    {
    case 8:
    {
        switch (supported_format.sampleType())
        {
        case QAudioFormat::UnSignedInt:
        {
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>((float*)input.data(), input.size() / sizeof(float));

            Eigen::VectorXf samples_int_tmp = samples_float * (float)std::numeric_limits<quint8>::max();

            VectorXuint8 samples_int = samples_int_tmp.cast<quint8>();

            return QByteArray((char*)samples_int.data(), samples_int.size() * sizeof(quint8));
        }
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>((float*)input.data(), input.size() / sizeof(float));

            Eigen::VectorXf samples_int_tmp = samples_float * (float)std::numeric_limits<qint8>::max();

            VectorXint8 samples_int = samples_int_tmp.cast<qint8>();

            return QByteArray((char*)samples_int.data(), samples_int.size() * sizeof(qint8));
        }
        default:
            break;
        }

        break;
    }
    case 16:
    {
        switch (supported_format.sampleType())
        {
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>((float*)input.data(), input.size() / sizeof(float));

            Eigen::VectorXf samples_int_tmp = samples_float * (float)std::numeric_limits<qint16>::max();

            VectorXint16 samples_int = samples_int_tmp.cast<qint16>();

            return QByteArray((char*)samples_int.data(), samples_int.size() * sizeof(qint16));
        }
        default:
            break;
        }

        break;
    }
    case 32:
    {
        switch (supported_format.sampleType())
        {
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>((float*)input.data(), input.size() / sizeof(float));

            Eigen::VectorXf samples_int_tmp = samples_float * (float)std::numeric_limits<qint32>::max();

            VectorXint32 samples_int = samples_int_tmp.cast<qint32>();

            return QByteArray((char*)samples_int.data(), samples_int.size() * sizeof(qint32));
        }
        default:
            break;
        }

        break;
    }
    default:
        break;
    }

    return QByteArray();
}

static inline QByteArray convertSamplesToFloat(const QByteArray &data, const QAudioFormat &supported_format)
{
    QByteArray input = data;

    switch (supported_format.sampleSize())
    {
    case 8:
    {
        switch (supported_format.sampleType())
        {
        case QAudioFormat::UnSignedInt:
        {
            Eigen::Ref<VectorXuint8> samples_int = Eigen::Map<VectorXuint8>((quint8*)input.data(), input.size() / sizeof(quint8));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / (float)std::numeric_limits<quint8>::max();

            return QByteArray((char*)samples_float.data(), samples_float.size() * sizeof(float));
        }
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<VectorXint8> samples_int = Eigen::Map<VectorXint8>((qint8*)input.data(), input.size() / sizeof(qint8));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / (float)std::numeric_limits<qint8>::max();

            return QByteArray((char*)samples_float.data(), samples_float.size() * sizeof(float));
        }
        default:
            break;
        }

        break;
    }
    case 16:
    {
        switch (supported_format.sampleType())
        {
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<VectorXint16> samples_int = Eigen::Map<VectorXint16>((qint16*)input.data(), input.size() / sizeof(qint16));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / (float)std::numeric_limits<qint16>::max();

            return QByteArray((char*)samples_float.data(), samples_float.size() * sizeof(float));
        }
        default:
            break;
        }

        break;
    }
    case 32:
    {
        switch (supported_format.sampleType())
        {
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<VectorXint32> samples_int = Eigen::Map<VectorXint32>((qint32*)input.data(), input.size() / sizeof(qint32));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / (float)std::numeric_limits<qint32>::max();

            return QByteArray((char*)samples_float.data(), samples_float.size() * sizeof(float));
        }
        default:
            break;
        }

        break;
    }
    default:
        break;
    }

    return QByteArray();
}

#endif // COMMON_H
