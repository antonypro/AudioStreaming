#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtMultimedia>
#include <eigen3/Eigen/Eigen>

#define TIMEOUT 30*1000

#define AES_PADDING 32

#if defined(IS_TO_DEBUG_VERBOSE_1) || defined(IS_TO_DEBUG_VERBOSE_2)
#define IS_TO_DEBUG

extern QMutex record_mutex;
extern qint64 record_count;

#define DEBUG_FUNCTION(message)\
{\
    record_mutex.lock();\
    \
    qDebug()\
    << "Date and time(UTC):" << qPrintable(QDateTime::currentDateTimeUtc().toString("yyyy/MM/dd hh:mm:ss"))\
    << "\nFile:" << __FILE__\
    << "\nLine:" << __LINE__\
    << "\nFunction:" << __FUNCTION__\
    << "\nIndex:" << ++record_count;\
    \
    qDebug()\
    << "Message:"\
    << message\
    << "\n";\
    \
    record_mutex.unlock();\
}

#ifdef IS_TO_DEBUG_VERBOSE_1

#define LIB_DEBUG_LEVEL_1(message)\
    DEBUG_FUNCTION(message)
#define LIB_DEBUG_LEVEL_2(message) //Does nothing

#elif defined(IS_TO_DEBUG_VERBOSE_2) // IS_TO_DEBUG_VERBOSE_1

#define LIB_DEBUG_LEVEL_1(message)\
    DEBUG_FUNCTION(message)
#define LIB_DEBUG_LEVEL_2(message)\
    DEBUG_FUNCTION(message)

#endif // IS_TO_DEBUG_VERBOSE_1

#else //IS_TO_DEBUG

#define LIB_DEBUG_LEVEL_1(message) //Does nothing
#define LIB_DEBUG_LEVEL_2(message) //Does nothing

#endif //IS_TO_DEBUG

#define SETTONULLPTR(obj) QObject::connect(obj, &QObject::destroyed, [&]{obj = nullptr;})

#ifdef OPUS

#define BUFFER_LEN 1024*1024 //Used by resampler

#define MAX_PACKET_SIZE 3*1276

#endif //OPUS

#define MAX_PENDING_BEATS 50

#define MAX_NETWORK_CHUNK_SIZE 10*1024*1024

#define START_THREAD \
{\
    setParent(nullptr);\
    QThread *thread = new QThread();\
    moveToThread(thread);\
    \
    if (parent) connect(parent, &QObject::destroyed, this, &QObject::deleteLater);\
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
    HeartBeat,
    ExtraData,
    ExtraDataReceived
};
}

namespace ServerCommand
{
enum
{
    PeerInfo,
    PeerTryConnect,
    ConnectionRequested,
    ConnectionAnswer,
    LoggedIn,
    ConnectedToPeer,
    DisconnectedFromPeer,
    P2PData,
    Alive,
    Warning
};
}

static inline QString cleanString(const QString &s)
{
    QString diacriticLetters;
    QStringList noDiacriticLetters;
    QStringList acceptedCharacters;

    diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
    noDiacriticLetters << "S"<<"OE"<<"Z"<<"s"<<"oe"<<"z"<<"Y"<<"Y"<<"u"<<"A"<<"A"<<"A"<<"A"<<"A"<<"A"<<"AE"<<"C"
                       <<"E"<<"E"<<"E"<<"E"<<"I"<<"I"<<"I"<<"I"<<"D"<<"N"<<"O"<<"O"<<"O"<<"O"<<"O"<<"O"<<"U"<<"U"
                      <<"U"<<"U"<<"Y"<<"s"<<"a"<<"a"<<"a"<<"a"<<"a"<<"a"<<"ae"<<"c"<<"e"<<"e"<<"e"<<"e"<<"i"<<"i"
                     <<"i"<<"i"<<"o"<<"n"<<"o"<<"o"<<"o"<<"o"<<"o"<<"o"<<"u"<<"u"<<"u"<<"u"<<"y"<<"y";

    acceptedCharacters << "0" << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9"
                       << "a" << "b" << "c" << "d" << "e" << "f" << "g" << "h" << "i" << "j"
                       << "k" << "l" << "m" << "n" << "o" << "p" << "q" << "r" << "s" << "t"
                       << "u" << "v" << "w" << "x" << "y" << "z";

    QString output_tmp;

    for (int i = 0; i < s.length(); i++)
    {
        QChar c = s[i];
        int dIndex = diacriticLetters.indexOf(c);
        if (dIndex < 0)
        {
            output_tmp.append(c);
        }
        else
        {
            QString replacement = noDiacriticLetters[dIndex];
            output_tmp.append(replacement);
        }
    }

    output_tmp = output_tmp.toLower();

    QString output;

    for (int i = 0; i < output_tmp.length(); i++)
    {
        QChar c = output_tmp[i];

        if (acceptedCharacters.contains(c))
            output.append(c);
    }

    return output;
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
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(input.data()), input.size() / int(sizeof(float)));

            Eigen::VectorXf samples_int_tmp = samples_float * float(std::numeric_limits<quint8>::max());

            VectorXuint8 samples_int = samples_int_tmp.cast<quint8>();

            return QByteArray(reinterpret_cast<char*>(samples_int.data()), samples_int.size() * int(sizeof(quint8)));
        }
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<Eigen::VectorXf> samples_float = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(input.data()), input.size() / int(sizeof(float)));

            Eigen::VectorXf samples_int_tmp = samples_float * float(std::numeric_limits<qint8>::max());

            VectorXint8 samples_int = samples_int_tmp.cast<qint8>();

            return QByteArray(reinterpret_cast<char*>(samples_int.data()), samples_int.size() * int(sizeof(qint8)));
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

            return QByteArray(reinterpret_cast<char*>(samples_int.data()), samples_int.size() * int(sizeof(qint16)));
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

            return QByteArray(reinterpret_cast<char*>(samples_int.data()), samples_int.size() * int(sizeof(qint32)));
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
            Eigen::Ref<VectorXuint8> samples_int = Eigen::Map<VectorXuint8>(reinterpret_cast<quint8*>(input.data()), input.size() / int(sizeof(quint8)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<quint8>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), samples_float.size() * int(sizeof(float)));
        }
        case QAudioFormat::SignedInt:
        {
            Eigen::Ref<VectorXint8> samples_int = Eigen::Map<VectorXint8>(reinterpret_cast<qint8*>(input.data()), input.size() / int(sizeof(qint8)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<qint8>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), samples_float.size() * int(sizeof(float)));
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
            Eigen::Ref<VectorXint16> samples_int = Eigen::Map<VectorXint16>(reinterpret_cast<qint16*>(input.data()), input.size() / int(sizeof(qint16)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<qint16>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), samples_float.size() * int(sizeof(float)));
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
            Eigen::Ref<VectorXint32> samples_int = Eigen::Map<VectorXint32>(reinterpret_cast<qint32*>(input.data()), input.size() / int(sizeof(qint32)));

            Eigen::VectorXf samples_float = samples_int.cast<float>() / float(std::numeric_limits<qint32>::max());

            return QByteArray(reinterpret_cast<char*>(samples_float.data()), samples_float.size() * int(sizeof(float)));
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
