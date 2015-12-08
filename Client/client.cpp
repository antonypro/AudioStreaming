#include "client.h"

Client::Client(QString host, quint16 port, QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);
    socket->connectToHost(host, port);
    connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
}

QAudioFormat Client::currentAudioFormat()
{
    return format;
}

void Client::readyRead()
{
    if (!format.isValid())
    {
        if (socket->bytesAvailable() < 8)
            return;

        QByteArray data = socket->read(8);
        readHeader(data);

        output = new AudioOutput(currentAudioFormat(), this);
    }

    QByteArray data;

    while (socket->bytesAvailable() > 0)
        data.append(socket->readAll());

    emit audioReady(data);
    output->writeData(data);
}

void Client::readHeader(QByteArray data)
{
    quint8 channelCount;
    quint32 sampleRate;
    quint8 sampleSize;
    quint8 byteOrder;
    quint8 sampleType;

    QDataStream stream(&data, QIODevice::ReadWrite);
    stream >> channelCount;
    stream >> sampleRate;
    stream >> sampleSize;
    stream >> byteOrder;
    stream >> sampleType;

    format.setCodec("audio/pcm");
    format.setChannelCount(channelCount);
    format.setSampleRate(sampleRate);
    format.setSampleSize(sampleSize);
    format.setByteOrder((QAudioFormat::Endian)byteOrder);
    format.setSampleType((QAudioFormat::SampleType)sampleType);
}
