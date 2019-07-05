#include "flowcontrol.h"

//Realtime flow control, used with the CustomInputDevice

#define interval 10

#define errorstr "Format not supported by the fake input device"

FlowControl::FlowControl(QObject *parent) : QObject(parent)
{
    m_elapsed_time = 0;

    m_sample_rate = 0;
    m_channels = 0;
    m_bits_per_sample = 0;

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &FlowControl::askForBytes);
    m_timer->setInterval(interval);
}

void FlowControl::stop()
{
    deleteLater();
}

void FlowControl::start(int sample_rate, int channels, int bits_per_sample)
{
    if (sample_rate < 8000 || sample_rate > 192000)
    {
        emit error(errorstr);
        return;
    }

    switch (channels)
    {
    case 1:
    case 2:
    case 4:
    case 6:
    case 8:
        break;
    default:
        emit error(errorstr);
        return;
    }

    switch (bits_per_sample)
    {
    case 32:
        break;
    default:
        emit error(errorstr);
        return;
    }

    m_sample_rate = 8000; //Generate at 8KHz and up-sample
    m_channels = channels;
    m_bits_per_sample = bits_per_sample;

    m_resampler = new r8brain(this);

    connect(m_resampler, &r8brain::error, this, &FlowControl::error);
    connect(m_resampler, &r8brain::resampled, this, &FlowControl::readyRead);

    m_resampler->start(m_sample_rate, sample_rate, channels, channels, bits_per_sample);

    m_timer->start();
    m_time.start();
}

void FlowControl::askForBytes() //Compute the size in bytes of the elapsed time
{
    qint64 elapsed_time = m_time.elapsed();

    qint64 elapsed = elapsed_time - m_elapsed_time;
    m_elapsed_time = elapsed_time;

    int bytes = int(timeToSize(elapsed, m_channels, m_bits_per_sample, m_sample_rate));

    QByteArray data;
    data.resize(bytes);

    if (!data.isEmpty())
    {
        Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(data.data()), data.size() / int(sizeof(float)));
        samples.fill(0);
    }

    if (m_resampler)
        m_resampler->write(data);
}
