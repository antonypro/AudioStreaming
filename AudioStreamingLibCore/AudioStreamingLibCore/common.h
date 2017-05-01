#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtMultimedia>

#define setTonullptr(obj) connect(obj, &QObject::destroyed, this, [this]{obj = nullptr;})

#ifdef OPUS

#define BUFFER_LEN 1024*1024; //Used by resampler

#define MAX_PACKET_SIZE 3*1276

#endif //OPUS

#define MAX_NETWORK_CHUNK_SIZE 10*1024*1024

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

static inline qint64 nanoTimeToSize(qint64 nano_time, int sample_size, int channel_count, int sample_rate)
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

static inline qint64 timeToSize(qint64 ms_time, int sample_size, int channel_count, int sample_rate)
{
    return nanoTimeToSize(ms_time * (qint64)qPow(1000, 2), sample_size, channel_count, sample_rate);
}

static inline qint64 timeToSize(qint64 ms_time, const QAudioFormat &format)
{
    return timeToSize(ms_time, format.sampleSize(), format.channelCount(), format.sampleRate());
}

static inline qint64 sizeToNanoTime(qint64 bytes, int sample_size, int channel_count, int sample_rate)
{
    return (bytes * (qint64)qPow(1000, 3) / ((sample_size / 8) * channel_count * sample_rate));
}

static inline qint64 sizeToTime(qint64 bytes, int sample_size, int channel_count, int sample_rate)
{
    return sizeToNanoTime(bytes, sample_size, channel_count, sample_rate) / (qint64)qPow(1000, 2);
}

static inline qint64 sizeToTime(qint64 bytes, const QAudioFormat &format)
{
    return sizeToTime(bytes, format.sampleSize(), format.channelCount(), format.sampleRate());
}

typedef struct PeerData {
    QByteArray data;
    QHostAddress host;
    qintptr descriptor;
} PeerData;

#endif // COMMON_H
