#ifndef AUDIOCOMMON_H
#define AUDIOCOMMON_H

#include <QtCore>
#include <QtMultimedia>
#include <eigen3/Eigen/Eigen>

static inline qint64 nanoTimeToSize(qint64 nano_time, int channel_count, int sample_size, int sample_rate)
{
    qint64 value = (channel_count * (sample_size / 8) * sample_rate) * nano_time / qint64(qPow(1000, 3));

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
    return nanoTimeToSize(ms_time * qint64(qPow(1000, 2)), channel_count, sample_size, sample_rate);
}

static inline qint64 timeToSize(qint64 ms_time, const QAudioFormat &format)
{
    return timeToSize(ms_time, format.channelCount(), format.sampleSize(), format.sampleRate());
}

static inline qint64 sizeToNanoTime(qint64 bytes, int channel_count, int sample_size, int sample_rate)
{
    int divider = ((sample_size / 8) * channel_count * sample_rate);
    if (divider == 0) return 0;
    return (bytes * qint64(qPow(1000, 3)) / divider);
}

static inline qint64 sizeToTime(qint64 bytes, int channel_count, int sample_size, int sample_rate)
{
    return sizeToNanoTime(bytes, channel_count, sample_size, sample_rate) / qint64(qPow(1000, 2));
}

static inline qint64 sizeToTime(qint64 bytes, const QAudioFormat &format)
{
    return sizeToTime(bytes, format.channelCount(), format.sampleSize(), format.sampleRate());
}

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
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(input.data()), input.size() / int(sizeof(float)));

            Eigen::VectorXf samples_int_tmp = samples_float * float(std::numeric_limits<quint8>::max());

            VectorXuint8 samples_int = samples_int_tmp.cast<quint8>();

            QByteArray raw = QByteArray(reinterpret_cast<char*>(samples_int.data()), int(samples_int.size()) * int(sizeof(quint8)));

            return raw;
        }
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(input.data()), input.size() / int(sizeof(float)));

            Eigen::VectorXf samples_int_tmp = samples_float * float(std::numeric_limits<qint8>::max());

            VectorXint8 samples_int = samples_int_tmp.cast<qint8>();

            QByteArray raw = QByteArray(reinterpret_cast<char*>(samples_int.data()), int(samples_int.size()) * int(sizeof(qint8)));

            return raw;
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
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(input.data()), input.size() / int(sizeof(float)));

            Eigen::VectorXf samples_int_tmp = samples_float * float(std::numeric_limits<qint16>::max());

            VectorXint16 samples_int = samples_int_tmp.cast<qint16>();

            QByteArray raw = QByteArray(reinterpret_cast<char*>(samples_int.data()), int(samples_int.size()) * int(sizeof(qint16)));

            return raw;
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
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(input.data()), input.size() / int(sizeof(float)));

            Eigen::VectorXf samples_int_tmp = samples_float * float(std::numeric_limits<qint32>::max());

            VectorXint32 samples_int = samples_int_tmp.cast<qint32>();

            QByteArray raw = QByteArray(reinterpret_cast<char*>(samples_int.data()), int(samples_int.size()) * int(sizeof(qint32)));

            return raw;
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
            QByteArray raw = input;

            Eigen::Ref<VectorXuint8> samples_int = Eigen::Map<VectorXuint8>(reinterpret_cast<quint8*>(raw.data()), raw.size() / int(sizeof(quint8)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<quint8>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), int(samples_float.size()) * int(sizeof(float)));
        }
        case QAudioFormat::SignedInt:
        {
            QByteArray raw = input;

            Eigen::Ref<VectorXint8> samples_int = Eigen::Map<VectorXint8>(reinterpret_cast<qint8*>(raw.data()), raw.size() / int(sizeof(qint8)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<qint8>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), int(samples_float.size()) * int(sizeof(float)));
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
            QByteArray raw = input;

            Eigen::Ref<VectorXint16> samples_int = Eigen::Map<VectorXint16>(reinterpret_cast<qint16*>(raw.data()), raw.size() / int(sizeof(qint16)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<qint16>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), int(samples_float.size()) * int(sizeof(float)));
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
            QByteArray raw = input;

            Eigen::Ref<VectorXint32> samples_int = Eigen::Map<VectorXint32>(reinterpret_cast<qint32*>(raw.data()), raw.size() / int(sizeof(qint32)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<qint32>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), int(samples_float.size()) * int(sizeof(float)));
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

#endif // AUDIOCOMMON_H
