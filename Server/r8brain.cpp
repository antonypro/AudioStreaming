#include "r8brain.h"

r8brain::r8brain(QObject *parent) : QObject(parent)
{

}

r8brain::~r8brain()
{
    delete[] InBufs;
    delete[] Resamps;
    delete[] opp;
}

void r8brain::start(int CHANNELS, int SAMPLERATE, int BITSPERSAMPLE)
{
    switch (BITSPERSAMPLE)
    {
    case 16:
    {
        bits = 16;
        break;
    }
    case 32:
    {
        bits = 32;
        break;
    }
    default:
        return;
    }

    channels = CHANNELS;

    double InSampleRate = (double)SAMPLERATE;
    const double OutSampleRate = 48000.0;

    const int InBufCapacity = BUFFER_LEN;
    InBufs = new CFixedBuffer<double>[channels];
    Resamps = new CPtrKeeper<CDSPResampler24*>[channels];

    for (int i = 0; i < channels; i++)
    {
        InBufs[i].alloc(InBufCapacity);
        Resamps[i] = new CDSPResampler24(InSampleRate, OutSampleRate, InBufCapacity);
    }

    opp = new double*[channels];
}

QByteArray r8brain::resample(const QByteArray &input)
{
    int ReadCount = 0;
    int bytes = input.size();

    switch (bits)
    {
    case 16:
    {
        ReadCount = bytes / sizeof(qint16) / channels;

        qint16 *samples = (qint16*)input.data();

        for (int j = 0; j < ReadCount; j++)
            for (int i = 0; i < channels; i++)
                InBufs[i][j] = (samples[j * channels + i] / (1.0 * 0x8000));

        break;
    }
    case 32:
    {
        ReadCount = bytes / sizeof(qint32) / channels;

        qint32 *samples = (qint32*)input.data();

        for (int j = 0; j < ReadCount; j++)
            for (int i = 0; i < channels; i++)
                InBufs[i][j] = (samples[j * channels + i] / (8.0 * 0x10000000));

        break;
    }
    default:
        break;
    }

    int WriteCount = 0; // At initial steps this variable can be equal to 0
    // after resampler. Same number for all channels.

    for (int i = 0; i < channels; i++)
    {
        WriteCount = Resamps[i]->process(InBufs[i], ReadCount, opp[i]);
    }

    QByteArray data;

    int min = std::numeric_limits<qint16>::min();
    int max = std::numeric_limits<qint16>::max();

    for (int j = 0; j < WriteCount; j++)
    {
        for (int i = 0; i < channels; i++)
        {
            qint16 val = qBound(min, (qRound(opp[i][j] * (8.0 * 0x10000000)) >> 16), max);
            data.append((char*)&val, sizeof(qint16));
        }
    }

    return data;
}
