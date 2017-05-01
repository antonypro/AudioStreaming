#include "levelmeterthread.h"

#define INTERVAL 50

LevelMeterThread::LevelMeterThread(QObject *parent) : QObject(parent)
{

}

void LevelMeterThread::start(const QAudioFormat &format)
{
    m_level = -1;

    m_format = format;

    m_size = 0;

    QTimer *timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &LevelMeterThread::currentlevelPrivate);
    timer->start(INTERVAL);
}

void LevelMeterThread::currentlevelPrivate()
{
    if (!qFuzzyCompare(m_level, -1))
        emit currentlevel(m_level);

    m_level = -1;
}

void LevelMeterThread::write(const QByteArray &data)
{
    m_buffer.append(data);
    process();
}

void LevelMeterThread::process()
{
    if (!m_buffer.isEmpty())
    {
        QByteArray data = m_buffer;
        m_buffer.clear();

        switch (m_format.sampleSize())
        {
        case 16:
        {
            int max = std::numeric_limits<qint16>::max();

            qint16 *samples = (qint16*)data.data();
            int size = data.size() / sizeof(qint16);

            for (int i = 0; i < size; i++)
            {
                int sample = (qint16)samples[i];
                m_level = qMax(m_level, qAbs(sample) / (qreal)max);
            }

            break;
        }
        case 32:
        {
            int max = std::numeric_limits<qint32>::max();

            qint32 *samples = (qint32*)data.data();
            int size = data.size() / sizeof(qint32);

            for (int i = 0; i < size; i++)
            {
                int sample = (qint32)samples[i];
                m_level = qMax(m_level, qAbs(sample) / (qreal)max);
            }

            break;
        }
        default:
            break;
        }
    }
}
