#include "audiooutput.h"

AudioOutput::AudioOutput(QAudioFormat format, QObject *parent) : QObject(parent)
{
    audio = new QAudioOutput(format, this);
    audio->setBufferSize(8192);

    device = audio->start();
}

void AudioOutput::writeData(QByteArray data)
{
    int readlen = audio->periodSize();

    int chunks = audio->bytesFree() / readlen;

    while (chunks)
    {
        QByteArray middle = data.mid(0, readlen);
        int len = middle.size();
        data.remove(0, len);

        if (len)
            device->write(middle);

        chunks--;
    }
}
