#include "audioinput.h"

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
}

void AudioInput::start(const QAudioDeviceInfo &devinfo,
                       int samplesize,
                       int samplerate,
                       int channels,
                       int sampletype,
                       int byteorder)
{
    format.setCodec("audio/pcm");
    format.setSampleSize(samplesize);
    format.setSampleRate(samplerate);
    format.setChannelCount(channels);
    format.setSampleType((QAudioFormat::SampleType)sampletype);
    format.setByteOrder((QAudioFormat::Endian)byteorder);

    if (!devinfo.isFormatSupported(format))
    {
        format = devinfo.nearestFormat(format);
        emit adjustSettings(format);
    }

    audio = new QAudioInput(devinfo, format, this);
    audio->setBufferSize(16384);

    device = audio->start();

    if (!device)
    {
        emit error("Failed to open audio device");
        return;
    }

#ifdef OPUS
    if (format.channelCount() > 2 || (format.sampleSize() != 16 && format.sampleSize() != 32) ||
            format.sampleType() != QAudioFormat::SignedInt || format.byteOrder() != QAudioFormat::LittleEndian)
    {
        emit error("This demo with opus only support 2 or 1 channels, 16 or 32 bits per sample,\n"
                   "signed integer as sample type and little endian byte order!");
        return;
    }

    res = new r8brain(this);
    res->start(format.channelCount(), format.sampleRate(), format.sampleSize());
    format.setSampleRate(48000);
    format.setSampleSize(16);
    opus = new OpusEncode(this);
    opus->start(format.channelCount(), format.sampleRate());
#endif

    connect(device, &QIODevice::readyRead, this, &AudioInput::readyRead);
}

void AudioInput::readyRead()
{
    QByteArray data;

    //Check the number of samples in input buffer
    int len = audio->bytesReady();

    //Read sound samples from input device to buffer
    if (len)
    {
        data.resize(len);
        device->read(data.data(), len);
    }

#ifdef OPUS
    QByteArray resampleddata = res->resample(data);
    int size = sizeof(qint16)*format.channelCount()*FRAME_SIZE;
    buffer.append(resampleddata);
    while (buffer.size() >= size)
    {
        QByteArray encodeddata = opus->encode(buffer.mid(0, size));
        buffer.remove(0, size);
        emit dataReady(encodeddata);
    }
#else
    emit dataReady(data);
#endif
}

QByteArray AudioInput::header()
{
    QByteArray data;

    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << (quint8)format.sampleSize();
    stream << (quint32)format.sampleRate();
    stream << (quint8)format.channelCount();
    stream << (quint8)format.sampleType();
    stream << (quint8)format.byteOrder();

    return data;
}
