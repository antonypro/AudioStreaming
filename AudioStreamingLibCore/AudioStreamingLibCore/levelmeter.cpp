#include "levelmeter.h"

//Compute level of the input data and of the output data when
//choose to not use the audio output device provided in this library

LevelMeter::LevelMeter(QObject *parent) : QObject(parent)
{

}

void LevelMeter::start(const QAudioFormat &format)
{
    QThread *thread = new QThread();
    m_meter = new LevelMeterThread();
    m_meter->moveToThread(thread);

    connect(this, &LevelMeter::destroyed, m_meter, &LevelMeterThread::deleteLater);
    connect(m_meter, &LevelMeterThread::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    connect(m_meter, &LevelMeterThread::currentlevel, this, &LevelMeter::currentlevel);

    QMetaObject::invokeMethod(m_meter, "start", Qt::QueuedConnection, Q_ARG(QAudioFormat, format));

    thread->start();
}

void LevelMeter::write(const QByteArray &data)
{
    QMetaObject::invokeMethod(m_meter, "write", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}
