#include "levelmeter.h"

//Compute level of the input data and of the output data when
//choose to not use the audio output device provided in this library

#define INTERVAL 50

LevelMeter::LevelMeter(QObject *parent) : QObject(parent)
{
    START_THREAD
}

void LevelMeter::startPrivate(const QAudioFormat &format)
{
    m_level = -1;

    m_format = format;

    m_size = 0;

    QTimer *timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &LevelMeter::currentlevelPrivate);
    timer->start(INTERVAL);
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
    m_buffer.append(data);
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

    Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>((float*)middle.data(), middle.size() / sizeof(float));

    m_level = samples.maxCoeff();
}
