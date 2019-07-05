#include "r8brain.h"
#include "common.h"

//Resampler class

//Used by resampler
#define RESAMPLER_BUFFER_LEN 1024*1024
#define RESAMPLER_2_CHANNELS 2

r8brain::r8brain(QObject *parent) : QObject(parent)
{
    m_bits = 0;
    m_channels_input = 0;
    m_channels_output = 0;
    m_in_bufs = nullptr;
    m_resamps = nullptr;
    m_opp = nullptr;
    m_initialized = false;

    START_THREAD
}

r8brain::~r8brain()
{
    if (m_initialized)
    {
        m_initialized = false;

        delete[] m_in_bufs;
        delete[] m_resamps;
        delete[] m_opp;
    }

    STOP_THREAD
}

void r8brain::stopPrivate()
{
    deleteLater();
}

void r8brain::stop()
{
    QTimer::singleShot(0, this, &r8brain::stopPrivate);
}

void r8brain::startPrivate(int in_sample_rate, int out_sample_rate, int in_channels, int out_channels, int sample_size)
{
    if (m_initialized)
        return;

    if (sample_size != 32)
    {
        emit error("Only 32 bits per sample are allowed!");
        return;
    }

    if (in_sample_rate <= 0 || out_sample_rate <= 0)
    {
        emit error("Invalid sample rate!");
        return;
    }

    if (in_channels <= 0 || out_channels <= 0)
    {
        emit error("Invalid channel count!");
        return;
    }

    if (in_channels != 1 && in_channels != 2 &&
            in_channels != 4 && in_channels != 6 && in_channels != 8)
    {
        emit error(QString("Input channel count %0 are not allowed!").arg(in_channels));
        return;
    }

    if (out_channels != 1 && out_channels != 2 &&
            out_channels != 4 && out_channels != 6 && out_channels != 8)
    {
        emit error(QString("Output channel count %0 are not allowed!").arg(out_channels));
        return;
    }

    m_initialized = true;

    m_bits = 32;

    m_channels_input = in_channels;
    m_channels_output = out_channels;

    m_in_bufs = new CFixedBuffer<double>[RESAMPLER_2_CHANNELS];
    m_resamps = new CPtrKeeper<CDSPResampler24*>[RESAMPLER_2_CHANNELS];

    for (int i = 0; i < RESAMPLER_2_CHANNELS; i++)
    {
        m_in_bufs[i].alloc(RESAMPLER_BUFFER_LEN);
        m_resamps[i] = new CDSPResampler24(double(in_sample_rate), double(out_sample_rate), RESAMPLER_BUFFER_LEN);
    }

    m_opp = new double*[RESAMPLER_2_CHANNELS];
}

void r8brain::start(int in_sample_rate, int out_sample_rate, int in_channels, int out_channels, int sample_size)
{
    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection,
                              Q_ARG(int, in_sample_rate),
                              Q_ARG(int, out_sample_rate),
                              Q_ARG(int, in_channels),
                              Q_ARG(int, out_channels),
                              Q_ARG(int, sample_size));
}

void r8brain::writePrivate(const QByteArray &input)
{
    if (!m_initialized)
        return;

    QByteArray data;

    switch (m_bits)
    {
    case 32:
    {
        QByteArray downchanneldata;

        //Convert input channels to 2 channels
        {
            if (input.size() % int(sizeof(float)) != 0)
            {
                emit error("Corrupted input!");
                return;
            }

            const float *samples = reinterpret_cast<const float*>(input.data());

            int samplescount = input.size() / int(sizeof(float)) / m_channels_input;

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

                for (int i = 0, j = 0; i < samplescount; i++, j += m_channels_input)
                {
                    FL[i] = samples[j + 0];
                    FR[i] = samples[j + 1];
                    C[i] = samples[j + 2];
                    LFE[i] = samples[j + 3]; // (sub-woofer)
                    RL[i] = samples[j + 4];
                    RR[i] = samples[j + 5];
                    SL[i] = samples[j + 6];
                    SR[i] = samples[j + 7];
                }

                Eigen::VectorXf L = (FL / 5 + RL / 5 + SL / 5 + C / 5 + LFE / 5);
                Eigen::VectorXf R = (FR / 5 + RR / 5 + SR / 5 + C / 5 + LFE / 5);

                for (int i = 0; i < samplescount; i++)
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

                for (int i = 0, j = 0; i < samplescount; i++, j += m_channels_input)
                {
                    FL[i] = samples[j + 0];
                    FR[i] = samples[j + 1];
                    C[i] = samples[j + 2];
                    LFE[i] = samples[j + 3]; // (sub-woofer)
                    RL[i] = samples[j + 4];
                    RR[i] = samples[j + 5];
                }

                Eigen::VectorXf L = (FL / 4 + RL / 4 + C / 4 + LFE / 4);
                Eigen::VectorXf R = (FR / 4 + RR / 4 + C / 4 + LFE / 4);

                for (int i = 0; i < samplescount; i++)
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

                for (int i = 0, j = 0; i < samplescount; i++, j += m_channels_input)
                {
                    FL[i] = samples[j + 0];
                    FR[i] = samples[j + 1];
                    RL[i] = samples[j + 2];
                    RR[i] = samples[j + 3];
                }

                Eigen::VectorXf L = (FL / 2 + RL / 2);
                Eigen::VectorXf R = (FR / 2 + RR / 2);

                for (int i = 0; i < samplescount; i++)
                {
                    downchanneldata.append(reinterpret_cast<char*>(&L[i]), sizeof(float));
                    downchanneldata.append(reinterpret_cast<char*>(&R[i]), sizeof(float));
                }

                break;
            }
            case 2:
            {
                Eigen::VectorXf L(samplescount);
                Eigen::VectorXf R(samplescount);

                for (int i = 0, j = 0; i < samplescount; i++, j += m_channels_input)
                {
                    L[i] = samples[j];
                    R[i] = samples[j + 1];
                }

                for (int i = 0; i < samplescount; i++)
                {
                    downchanneldata.append(reinterpret_cast<char*>(&L[i]), sizeof(float));
                    downchanneldata.append(reinterpret_cast<char*>(&R[i]), sizeof(float));
                }

                break;
            }
            case 1:
            {
                Eigen::VectorXf L(samplescount);
                Eigen::VectorXf R(samplescount);

                for (int i = 0, j = 0; i < samplescount; i++, j += m_channels_input)
                {
                    L[i] = samples[j];
                    R[i] = samples[j];
                }

                for (int i = 0; i < samplescount; i++)
                {
                    downchanneldata.append(reinterpret_cast<char*>(&L[i]), sizeof(float));
                    downchanneldata.append(reinterpret_cast<char*>(&R[i]), sizeof(float));
                }

                break;
            }
            default:
                break;
            }
        }

        //Resample step
        int readCount = 0;

        float *outputsamples = reinterpret_cast<float*>(downchanneldata.data());

        readCount = downchanneldata.size() / int(sizeof(float)) / RESAMPLER_2_CHANNELS;

        for (int j = 0; j < readCount; j++)
            for (int i = 0; i < RESAMPLER_2_CHANNELS; i++)
                m_in_bufs[i][j] = double(outputsamples[j * RESAMPLER_2_CHANNELS + i]);

        int writeCount = 0;

        for (int i = 0; i < RESAMPLER_2_CHANNELS; i++)
            writeCount = m_resamps[i]->process(m_in_bufs[i], readCount, m_opp[i]);

        QByteArray tmpdata;

        for (int j = 0; j < writeCount; j++)
        {
            for (int i = 0; i < RESAMPLER_2_CHANNELS; i++)
            {
                float val = float(m_opp[i][j]);
                tmpdata.append(reinterpret_cast<char*>(&val), sizeof(float));
            }
        }

        QByteArray upchanneldata;

        //Increase output channels
        {
            if (tmpdata.size() % int(sizeof(float)) != 0)
            {
                emit error("Corrupted input!");
                return;
            }

            const float *samples = reinterpret_cast<const float*>(tmpdata.data());

            int samplescount = tmpdata.size() / int(sizeof(float));

            switch (m_channels_output)
            {
            case 8:
            {
                for (int i = 0; i < samplescount; i += RESAMPLER_2_CHANNELS)
                {
                    float L, R, mixLR;

                    L = samples[i];
                    R = samples[i + 1];

                    mixLR = L / 2 + R / 2;

                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //FL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //FR
                    upchanneldata.append(reinterpret_cast<char*>(&mixLR), sizeof(float)); //C
                    upchanneldata.append(reinterpret_cast<char*>(&mixLR), sizeof(float)); //LFE (sub-woofer)
                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //RL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //RR
                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //SL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //SR
                }

                break;
            }
            case 6:
            {
                for (int i = 0; i < samplescount; i += RESAMPLER_2_CHANNELS)
                {
                    float L, R, mixLR;

                    L = samples[i];
                    R = samples[i + 1];

                    mixLR = L / 2 + R / 2;

                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //FL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //FR
                    upchanneldata.append(reinterpret_cast<char*>(&mixLR), sizeof(float)); //C
                    upchanneldata.append(reinterpret_cast<char*>(&mixLR), sizeof(float)); //LFE (sub-woofer)
                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //RL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //RR
                }

                break;
            }
            case 4:
            {
                for (int i = 0; i < samplescount; i += RESAMPLER_2_CHANNELS)
                {
                    float L, R;

                    L = samples[i];
                    R = samples[i + 1];

                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //FL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //FR
                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //RL
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //RR
                }

                break;
            }
            case 2:
            {
                for (int i = 0; i < samplescount; i += RESAMPLER_2_CHANNELS)
                {
                    float L, R;

                    L = samples[i];
                    R = samples[i + 1];

                    upchanneldata.append(reinterpret_cast<char*>(&L), sizeof(float)); //L
                    upchanneldata.append(reinterpret_cast<char*>(&R), sizeof(float)); //R
                }

                break;
            }
            case 1:
            {
                for (int i = 0; i < samplescount; i += RESAMPLER_2_CHANNELS)
                {
                    float mixLR;

                    mixLR = samples[i] / 2 + samples[i + 1] / 2;

                    upchanneldata.append(reinterpret_cast<char*>(&mixLR), sizeof(float)); //LR
                }

                break;
            }
            default:
                break;
            }

            data = upchanneldata;
        }

        break;
    }
    default:
        break;
    }

    //Bound to -1 - +1
    {
        int middle_size = data.size() / int(sizeof(float));

        float *middle_data = reinterpret_cast<float*>(data.data());

        for (int i = 0; i < middle_size; i++)
            middle_data[i] = qBound(float(-1), middle_data[i], float(1));
    }

    emit resampled(data);
}

void r8brain::write(const QByteArray &input)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection,
                              Q_ARG(QByteArray, input));
}
