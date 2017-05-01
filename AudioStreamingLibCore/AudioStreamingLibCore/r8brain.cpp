#include "r8brain.h"

//Resampler class

r8brain::r8brain(QObject *parent) : QObject(parent)
{
    m_bits = 0;
    m_channels_input = 0;
    m_channels_output = 0;
    m_InBufs = nullptr;
    m_Resamps = nullptr;
    m_opp = nullptr;
    m_initalized = false;
}

r8brain::~r8brain()
{
    if (!m_initalized)
        return;

    delete[] m_InBufs;
    delete[] m_Resamps;
    delete[] m_opp;
}

void r8brain::start(int in_sample_rate, int out_sample_rate, int channels, int sample_size)
{
    if (m_initalized)
        return;

    switch (sample_size)
    {
    case 16:
    {
        m_bits = 16;
        break;
    }
    case 32:
    {
        m_bits = 32;
        break;
    }
    default:
        emit error("Only 16 bits or 32 bits per sample are allowed!");
        return;
    }

    m_initalized = true;

    m_channels_input = m_channels_output = channels;

    m_channels_output = qMin(m_channels_output, 2);

    const double InSampleRate = in_sample_rate;
    const double OutSampleRate = out_sample_rate;

    const int InBufCapacity = BUFFER_LEN;
    m_InBufs = new CFixedBuffer<double>[m_channels_output];
    m_Resamps = new CPtrKeeper<CDSPResampler24*>[m_channels_output];

    for (int i = 0; i < m_channels_output; i++)
    {
        m_InBufs[i].alloc(InBufCapacity);
        m_Resamps[i] = new CDSPResampler24(InSampleRate, OutSampleRate, InBufCapacity);
    }

    m_opp = new double*[m_channels_output];
}

//If more than two channels, down to two, and resample to OPUS_SAMPLE_RATE
void r8brain::write(const QByteArray &input)
{
    if (!m_initalized)
        return;

    int readCount = 0;

    switch (m_bits)
    {
    case 16:
    {
        if (input.size() % 2 != 0)
        {
            emit error("Corrupted audio data!");
            return;
        }

        int min = std::numeric_limits<qint16>::min();
        int max = std::numeric_limits<qint16>::max();

        qint16 *samples = (qint16*)input.data();

        int samplescount = input.size() / sizeof(qint16);

        QByteArray downchanneldata;

        switch (m_channels_input)
        {
        case 8:
        {
            for (int i = 0; i < samplescount; i += 8) {
                int FL = samples[i + 0];
                int FR = samples[i + 1];
                int C = samples[i + 2];
                int LFE = samples[i + 3]; // (sub-woofer)
                int RL = samples[i + 4];
                int RR = samples[i + 5];
                int SL = samples[i + 6];
                int SR = samples[i + 7];

                qint16 L = qBound(min, qRound(FL / 5.0 + RL / 5.0 + SL / 5.0 + C / 5.0 + LFE / 5.0), max);
                qint16 R = qBound(min, qRound(FR / 5.0 + RR / 5.0 + SR / 5.0 + C / 5.0 + LFE / 5.0), max);

                downchanneldata.append((char*)&L, sizeof(qint16));
                downchanneldata.append((char*)&R, sizeof(qint16));
            }
            break;
        }
        case 6:
        {
            for (int i = 0; i < samplescount; i += 6) {
                int FL = samples[i + 0];
                int FR = samples[i + 1];
                int C = samples[i + 2];
                int LFE = samples[i + 3]; // (sub-woofer)
                int RL = samples[i + 4];
                int RR = samples[i + 5];

                qint16 L = qBound(min, qRound(FL / 4.0 + RL / 4.0 + C / 4.0 + LFE / 4.0), max);
                qint16 R = qBound(min, qRound(FR / 4.0 + RR / 4.0 + C / 4.0 + LFE / 4.0), max);

                downchanneldata.append((char*)&L, sizeof(qint16));
                downchanneldata.append((char*)&R, sizeof(qint16));
            }
            break;
        }
        case 4:
        {
            for (int i = 0; i < samplescount; i += 4) {
                int FL = samples[i + 0];
                int FR = samples[i + 1];
                int RL = samples[i + 2];
                int RR = samples[i + 3];

                qint16 L = qBound(min, qRound(FL / 2.0 + RL / 2.0), max);
                qint16 R = qBound(min, qRound(FR / 2.0 + RR / 2.0), max);

                downchanneldata.append((char*)&L, sizeof(qint16));
                downchanneldata.append((char*)&R, sizeof(qint16));
            }
            break;
        }
        case 2:
        case 1:
        {
            downchanneldata = input;
            break;
        }
        default:
            break;
        }

        qint16 *outputsamples = (qint16*)downchanneldata.data();

        readCount = downchanneldata.size() / sizeof(qint16) / m_channels_output;

        for (int j = 0; j < readCount; j++)
            for (int i = 0; i < m_channels_output; i++)
                m_InBufs[i][j] = (outputsamples[j * m_channels_output + i] / (double)max);

        break;
    }
    case 32:
    {
        if (input.size() % 4 != 0)
        {
            emit error("Corrupted input!");
            return;
        }

        int min = std::numeric_limits<qint32>::min();
        int max = std::numeric_limits<qint32>::max();

        qint32 *samples = (qint32*)input.data();

        int samplescount = input.size() / sizeof(qint32);

        QByteArray downchanneldata;

        switch (m_channels_input)
        {
        case 8:
        {
            for (int i = 0; i < samplescount; i += 8) {
                int FL = samples[i + 0];
                int FR = samples[i + 1];
                int C = samples[i + 2];
                int LFE = samples[i + 3]; // (sub-woofer)
                int RL = samples[i + 4];
                int RR = samples[i + 5];
                int SL = samples[i + 6];
                int SR = samples[i + 7];

                qint32 L = qBound(min, qRound(FL / 5.0 + RL / 5.0 + SL / 5.0 + C / 5.0 + LFE / 5.0), max);
                qint32 R = qBound(min, qRound(FR / 5.0 + RR / 5.0 + SR / 5.0 + C / 5.0 + LFE / 5.0), max);

                downchanneldata.append((char*)&L, sizeof(qint32));
                downchanneldata.append((char*)&R, sizeof(qint32));
            }
            break;
        }
        case 6:
        {
            for (int i = 0; i < samplescount; i += 6) {
                int FL = samples[i + 0];
                int FR = samples[i + 1];
                int C = samples[i + 2];
                int LFE = samples[i + 3]; // (sub-woofer)
                int RL = samples[i + 4];
                int RR = samples[i + 5];

                qint32 L = qBound(min, qRound(FL / 4.0 + RL / 4.0 + C / 4.0 + LFE / 4.0), max);
                qint32 R = qBound(min, qRound(FR / 4.0 + RR / 4.0 + C / 4.0 + LFE / 4.0), max);

                downchanneldata.append((char*)&L, sizeof(qint32));
                downchanneldata.append((char*)&R, sizeof(qint32));
            }
            break;
        }
        case 4:
        {
            for (int i = 0; i < samplescount; i += 4) {
                int FL = samples[i + 0];
                int FR = samples[i + 1];
                int RL = samples[i + 2];
                int RR = samples[i + 3];

                qint32 L = qBound(min, qRound(FL / 2.0 + RL / 2.0), max);
                qint32 R = qBound(min, qRound(FR / 2.0 + RR / 2.0), max);

                downchanneldata.append((char*)&L, sizeof(qint32));
                downchanneldata.append((char*)&R, sizeof(qint32));
            }
            break;
        }
        case 2:
        case 1:
        {
            downchanneldata = input;
            break;
        }
        default:
            break;
        }

        qint32 *outputsamples = (qint32*)downchanneldata.data();

        readCount = downchanneldata.size() / sizeof(qint32) / m_channels_output;

        for (int j = 0; j < readCount; j++)
            for (int i = 0; i < m_channels_output; i++)
                m_InBufs[i][j] = (outputsamples[j * m_channels_output + i] / (double)max);

        break;
    }
    default:
        break;
    }

    int writeCount = 0; // At initial steps this variable can be equal to 0
    // after resampler. Same number for all channels.

    for (int i = 0; i < m_channels_output; i++)
    {
        writeCount = m_Resamps[i]->process(m_InBufs[i], readCount, m_opp[i]);
    }

    QByteArray data;

    int min = std::numeric_limits<qint16>::min();
    int max = std::numeric_limits<qint16>::max();

    for (int j = 0; j < writeCount; j++)
    {
        for (int i = 0; i < m_channels_output; i++)
        {
            qint16 val = qBound(min, qRound(m_opp[i][j] * max), max);
            data.append((char*)&val, sizeof(qint16));
        }
    }

    emit resampled(data);
}
