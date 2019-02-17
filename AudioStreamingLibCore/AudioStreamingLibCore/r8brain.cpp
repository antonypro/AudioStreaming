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
    m_initialized = false;

    START_THREAD
}

r8brain::~r8brain()
{
    if (m_initialized)
    {
        m_initialized = false;

        delete[] m_InBufs;
        delete[] m_Resamps;
        delete[] m_opp;
    }

    STOP_THREAD
}

void r8brain::startPrivate(int in_sample_rate, int out_sample_rate, int channels, int sample_size)
{
    if (m_initialized)
        return;

    switch (sample_size)
    {
    case 32:
    {
        m_bits = 32;
        break;
    }
    default:
        emit error("Only 32 bits per sample are allowed!");
        return;
    }

    m_initialized = true;

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

void r8brain::start(int in_sample_rate, int out_sample_rate, int channels, int sample_size)
{
    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection,
                              Q_ARG(int, in_sample_rate),
                              Q_ARG(int, out_sample_rate),
                              Q_ARG(int, channels),
                              Q_ARG(int, sample_size));
}

//If more than two channels, down to two, and resample to OPUS_SAMPLE_RATE
void r8brain::writePrivate(const QByteArray &input)
{
    if (!m_initialized)
        return;

    int readCount = 0;

    switch (m_bits)
    {
    case 32:
    {
        if (input.size() % int(sizeof(float)) != 0)
        {
            emit error("Corrupted input!");
            return;
        }

        const float *samples = reinterpret_cast<const float*>(input.data());

        int samplescount = input.size() / int(sizeof(float));

        QByteArray downchanneldata;

        switch (m_channels_input)
        {
        case 8:
        {
            Eigen::VectorXf FL(samplescount);
            Eigen::VectorXf FR(samplescount);
            Eigen::VectorXf C(samplescount);
            Eigen::VectorXf LFE(samplescount); // (sub-woofer)
            Eigen::VectorXf RL(samplescount);
            Eigen::VectorXf RR(samplescount);
            Eigen::VectorXf SL(samplescount);
            Eigen::VectorXf SR(samplescount);

            for (int i = 0; i < samplescount; i += m_channels_input)
            {
                FL[i] = samples[i + 0];
                FR[i] = samples[i + 1];
                C[i] = samples[i + 2];
                LFE[i] = samples[i + 3]; // (sub-woofer)
                RL[i] = samples[i + 4];
                RR[i] = samples[i + 5];
                SL[i] = samples[i + 6];
                SR[i] = samples[i + 7];
            }

            Eigen::VectorXf L = (FL / 5 + RL / 5 + SL / 5 + C / 5 + LFE / 5);
            Eigen::VectorXf R = (FR / 5 + RR / 5 + SR / 5 + C / 5 + LFE / 5);

            for (int i = 0; i < samplescount; i ++)
            {
                downchanneldata.append(reinterpret_cast<char*>(&L[i]), sizeof(float));
                downchanneldata.append(reinterpret_cast<char*>(&R[i]), sizeof(float));
            }

            break;
        }
        case 6:
        {
            Eigen::VectorXf FL(samplescount);
            Eigen::VectorXf FR(samplescount);
            Eigen::VectorXf C(samplescount);
            Eigen::VectorXf LFE(samplescount); // (sub-woofer)
            Eigen::VectorXf RL(samplescount);
            Eigen::VectorXf RR(samplescount);

            for (int i = 0; i < samplescount; i += m_channels_input)
            {
                FL[i] = samples[i + 0];
                FR[i] = samples[i + 1];
                C[i] = samples[i + 2];
                LFE[i] = samples[i + 3]; // (sub-woofer)
                RL[i] = samples[i + 4];
                RR[i] = samples[i + 5];
            }

            Eigen::VectorXf L = (FL / 4 + RL / 4 + C / 4 + LFE / 4);
            Eigen::VectorXf R = (FR / 4 + RR / 4 + C / 4 + LFE / 4);

            downchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float));
            downchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float));

            for (int i = 0; i < samplescount; i ++)
            {
                downchanneldata.append(reinterpret_cast<char*>(&L[i]), sizeof(float));
                downchanneldata.append(reinterpret_cast<char*>(&R[i]), sizeof(float));
            }

            break;
        }
        case 4:
        {
            Eigen::VectorXf FL(samplescount);
            Eigen::VectorXf FR(samplescount);
            Eigen::VectorXf RL(samplescount);
            Eigen::VectorXf RR(samplescount);

            for (int i = 0; i < samplescount; i += m_channels_input)
            {
                FL[i] = samples[i + 0];
                FR[i] = samples[i + 1];
                RL[i] = samples[i + 2];
                RR[i] = samples[i + 3];
            }

            Eigen::VectorXf L = (FL / 2 + RL / 2);
            Eigen::VectorXf R = (FR / 2 + RR / 2);

            for (int i = 0; i < samplescount; i ++)
            {
                downchanneldata.append(reinterpret_cast<char*>(&L[i]), sizeof(float));
                downchanneldata.append(reinterpret_cast<char*>(&R[i]), sizeof(float));
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

        float *outputsamples = reinterpret_cast<float*>(downchanneldata.data());

        readCount = downchanneldata.size() / int(sizeof(float)) / m_channels_output;

        for (int j = 0; j < readCount; j++)
            for (int i = 0; i < m_channels_output; i++)
                m_InBufs[i][j] = double(outputsamples[j * m_channels_output + i]);

        break;
    }
    default:
        break;
    }

    int writeCount = 0; // At initial steps this variable can be equal to 0
    // after resampler. Same number for all channels.

    for (int i = 0; i < m_channels_output; i++)
        writeCount = m_Resamps[i]->process(m_InBufs[i], readCount, m_opp[i]);

    QByteArray data;

    for (int j = 0; j < writeCount; j++)
    {
        for (int i = 0; i < m_channels_output; i++)
        {
            float val = float(m_opp[i][j]);
            data.append(reinterpret_cast<char*>(&val), sizeof(float));
        }
    }

    emit resampled(data);
}

void r8brain::write(const QByteArray &input)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection,
                              Q_ARG(QByteArray, input));
}
