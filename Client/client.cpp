#include "client.h"

Client::Client(QObject *parent) : QObject(parent)
{

}

void Client::start(const QString &host, quint16 port, uint timetobuffer)
{
    ttb = timetobuffer;
    output = NULL;
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &Client::readyRead);
    connect(socket, &QTcpSocket::stateChanged, this, &Client::stateChange);
    socket->connectToHost(host, port);
}

void Client::stateChange(QTcpSocket::SocketState socketState)
{
    if (socketState == QTcpSocket::UnconnectedState)
        finishedfunc();
}

QAudioFormat Client::currentAudioFormat()
{
    return format;
}

void Client::finishedfunc()
{
    emit error(socket->errorString());
}

void Client::readyRead()
{
    if (!format.isValid())
    {
        if (socket->bytesAvailable() < 8)
            return;

        QByteArray data = socket->read(8);
        readHeader(data);

        output = new AudioOutput(currentAudioFormat(), ttb, this);
        connect(output, &AudioOutput::currentlevel, this, &Client::currentlevel);
        output->setVolume(volume);

#ifdef OPUS
        opus = new OpusDecode(this);
        opus->start(format.channelCount(), format.sampleRate());
        size = 0;
#endif
    }

#ifdef OPUS
    while (socket->bytesAvailable() > 0)
    {
        buffer.append(socket->readAll());
        while ((size == 0 && buffer.size() >= 4) || (size > 0 && buffer.size() >= size))
        {
            if (size == 0 && buffer.size() >= 4)
            {
                size = ArrayToInt(buffer.mid(0, 4));
                buffer.remove(0, 4);
            }
            if (size > 0 && buffer.size() >= size)
            {
                QByteArray data = buffer.mid(0, size);
                buffer.remove(0, size);
                size = 0;
                processInput(data);
            }
        }
    }
#else
    while (socket->bytesAvailable() > 0)
    {
        QByteArray data = socket->readAll();
        emit audioReady(data);
        output->writeData(data);
    }
#endif
}

#ifdef OPUS
void Client::processInput(const QByteArray &data)
{
    QByteArray decoded = opus->decode(data);
    emit audioReady(decoded);
    output->writeData(decoded);
}
#endif

void Client::readHeader(const QByteArray &data)
{
    quint8 sampleSize;
    quint32 sampleRate;
    quint8 channelCount;
    quint8 sampleType;
    quint8 byteOrder;

    QDataStream stream(data);
    stream >> sampleSize;
    stream >> sampleRate;
    stream >> channelCount;
    stream >> sampleType;
    stream >> byteOrder;

    format.setCodec("audio/pcm");
    format.setSampleSize(sampleSize);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
    format.setSampleType((QAudioFormat::SampleType)sampleType);
    format.setByteOrder((QAudioFormat::Endian)byteOrder);
}

void Client::setVolume(int vol)
{
    volume = vol;
    if (output)
        output->setVolume(volume);
}

qint32 Client::ArrayToInt(const QByteArray &data)
{
    qint32 value;
    QDataStream stream(data);
    stream >> value;
    return value;
}
