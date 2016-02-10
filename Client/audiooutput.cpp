#include "audiooutput.h"

AudioOutput::AudioOutput(const QAudioFormat &format, uint timetobuffer, QObject *parent) : QObject(parent)
{
    f = format;

    bufferrequested = true;

    readcalled = false;

    issigned = (f.sampleType() == QAudioFormat::SignedInt);
    samplesize = f.sampleSize();

    savedlevel = -1;
    level = 0;

    sizetobuffer = f.channelCount() * (f.sampleSize() / 8) * f.sampleRate() / (double)1000 * timetobuffer;

    audio = new QAudioOutput(f, this);
    audio->setBufferSize(16384);

    device = audio->start();

    time.start();
}

void AudioOutput::writeData(const QByteArray &data)
{
    buffer.append(data);

    if (!readcalled)
    {
        readcalled = true;
        QMetaObject::invokeMethod(this, "read", Qt::QueuedConnection);
    }
}

void AudioOutput::read()
{
    readcalled = false;

    if (buffer.isEmpty())
    {
        bufferrequested = true;
        return;
    }
    else if (buffer.size() < sizetobuffer)
    {
        if (bufferrequested)
            return;
    }
    else
    {
        bufferrequested = false;
    }

    while (!buffer.isEmpty())
    {
        int readlen = audio->periodSize();

        int chunks = audio->bytesFree() / readlen;

        while (chunks)
        {
            QByteArray middle = buffer.mid(0, readlen);
            int len = middle.size();
            buffer.remove(0, len);

            switch (samplesize)
            {
            case 16:
            {
                int min = std::numeric_limits<qint16>::min();
                int max = std::numeric_limits<qint16>::max();

                if (issigned)
                {
                    qint16 *samples = (qint16*)middle.data();
                    int size = len / sizeof(qint16);

                    for (int i = 0; i < size; i++)
                    {
                        int sample = (qint16)samples[i];
                        level = qMax(level, qAbs(sample) / (double)max);
                        sample = sample * (volume / (double)100);
                        sample = qBound(min, sample, max);
                        samples[i] = sample;
                    }
                }
                else
                {
                    quint16 *samples = (quint16*)middle.data();
                    int size = len / sizeof(quint16);

                    for (int i = 0; i < size; i++)
                    {
                        int sample = (qint16)samples[i];
                        level = qMax(level, qAbs(sample) / (double)max);
                        sample = sample * (volume / (double)100);
                        sample = qBound(min, sample, max);
                        samples[i] = sample;
                    }
                }
                break;
            }
            case 32:
            {
                int min = std::numeric_limits<qint32>::min();
                int max = std::numeric_limits<qint32>::max();

                if (issigned)
                {
                    qint32 *samples = (qint32*)middle.data();
                    int size = len / sizeof(qint32);

                    for (int i = 0; i < size; i++)
                    {
                        int sample = (qint32)samples[i];
                        level = qMax(level, qAbs(sample) / (double)max);
                        sample = sample * (volume / (double)100);
                        sample = qBound(min, sample, max);
                        samples[i] = sample;
                    }
                }
                else
                {
                    quint32 *samples = (quint32*)middle.data();
                    int size = len / sizeof(quint32);

                    for (int i = 0; i < size; i++)
                    {
                        int sample = (qint32)samples[i];
                        level = qMax(level, qAbs(sample) / (double)max);
                        sample = sample * (volume / (double)100);
                        sample = qBound(min, sample, max);
                        samples[i] = sample;
                    }
                }
                break;
            }
            default:
            {
                audio->setVolume(volume / (double)100);
                break;
            }
            }

            if (len)
                device->write(middle);

            if (time.elapsed() >= 50)
            {
                if (!qFuzzyCompare(savedlevel, (double)-1))
                    emit currentlevel((float)savedlevel);
                savedlevel = level;
                level = 0;
                time.restart();
                if (!readcalled)
                {
                    readcalled = true;
                    QMetaObject::invokeMethod(this, "read", Qt::QueuedConnection);
                }
                return;
            }

            chunks--;
        }
    }
}

void AudioOutput::setVolume(int vol)
{
    volume = vol;
}
