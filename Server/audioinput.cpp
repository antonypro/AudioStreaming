#include "audioinput.h"

AudioInput::AudioInput(QAudioDeviceInfo devinfo, QObject *parent) : QObject(parent)
{
    format.setCodec("audio/pcm");
    format.setChannelCount(2);
    format.setSampleRate(48000);
    format.setSampleSize(16);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    audio = new QAudioInput(devinfo, format, this);
    audio->setBufferSize(8192);

    device = audio->start();
    connect(device, SIGNAL(readyRead()), SLOT(readyRead()));
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

    emit dataReady(data);
}

QByteArray AudioInput::header()
{
    QByteArray data;

    QDataStream stream(&data, QIODevice::ReadWrite);
    stream << (quint8)format.channelCount();
    stream << (quint32)format.sampleRate();
    stream << (quint8)format.sampleSize();
    stream << (quint8)format.byteOrder();
    stream << (quint8)format.sampleType();

    return data;
}
