#ifndef LEVELMETER_H
#define LEVELMETER_H

#include <QtCore>
#include <QtMultimedia>
#include "levelmeterthread.h"

class LevelMeter : public QObject
{
    Q_OBJECT
public:
    explicit LevelMeter(QObject *parent = 0);

signals:
    void currentlevel(qreal);

public slots:
    void start(const QAudioFormat &format);
    void write(const QByteArray &data);

private:
    LevelMeterThread *m_meter;
};

#endif // LEVELMETER_H
