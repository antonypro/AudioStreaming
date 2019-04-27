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
    connect(m_timer, &QTimer::timeout, this, &FlowControl::askforbytes);
    m_timer->setInterval(interval);
}

void FlowControl::start(int sample_rate, int channels, int bits_per_sample)
{
    switch (sample_rate)
    {
    case 8000:
    case 12000:
    case 16000:
    case 24000:
    case 44100:
    case 48000:
    case 96000:
    case 192000:
        break;
    default:
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

    m_sample_rate = sample_rate;
    m_channels = channels;
    m_bits_per_sample = bits_per_sample;

    m_timer->start();
    m_time.start();
}

void FlowControl::askforbytes() //Compute the size in bytes of the elapsed time
{
    qint64 elapsed_time = m_time.nsecsElapsed();

    qint64 elapsed = elapsed_time - m_elapsed_time;
    m_elapsed_time = elapsed_time;

    int bytes = int(nanoTimeToSize(elapsed, m_channels, int(sizeof(float)) * 8, m_sample_rate));

    emit getbytes(bytes);
}
