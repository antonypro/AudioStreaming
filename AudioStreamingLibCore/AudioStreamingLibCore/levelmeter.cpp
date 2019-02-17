#include "levelmeter.h"

//Compute level of the input data and of the output data
#define INTERVAL 50

LevelMeter::LevelMeter(QObject *parent) : QObject(parent)
{
    START_THREAD
}

LevelMeter::~LevelMeter()
{
    STOP_THREAD
}

void LevelMeter::startPrivate(const QAudioFormat &format)
{
    m_level = -1;

    m_format = format;

    m_buffer.clear();

    if (!m_timer)
    {
        m_timer = new QTimer(this);

        m_timer->setTimerType(Qt::PreciseTimer);
        connect(m_timer, &QTimer::timeout, this, &LevelMeter::currentlevelPrivate);
        m_timer->start(INTERVAL);
    }
}

void LevelMeter::start(const QAudioFormat &format)
{
    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection, Q_ARG(QAudioFormat, format));
}

void LevelMeter::currentlevelPrivate()
{
    if (!qFuzzyCompare(m_level, -1))
        emit currentlevel(m_level);

    m_level = -1;
}

void LevelMeter::writePrivate(const QByteArray &data)
{
    Eigen::VectorXf samples = Eigen::VectorXf(data.size() / int(sizeof(float)));

    memcpy(samples.data(), data.data(), uint(data.size()));

    samples = samples.array().abs();

    m_buffer.append(reinterpret_cast<char*>(samples.data()), samples.size() * int(sizeof(float)));

    process();
}

void LevelMeter::write(const QByteArray &data)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void LevelMeter::process()
{
    QByteArray middle = m_buffer;
    m_buffer.clear();

    if (middle.isEmpty())
        return;

    Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(middle.data()), middle.size() / int(sizeof(float)));

    m_level = samples.maxCoeff();
}
