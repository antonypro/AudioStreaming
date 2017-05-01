#ifndef LEVELMETERTHREAD_H
#define LEVELMETERTHREAD_H

#include <QtCore>
#include <QtMultimedia>
#include "common.h"

class LevelMeterThread : public QObject
{
    Q_OBJECT
public:
    explicit LevelMeterThread(QObject *parent = 0);

signals:
    void currentlevel(qreal);

public slots:
    void start(const QAudioFormat &format);
    void write(const QByteArray &data);

private slots:
    void currentlevelPrivate();
    void process();

private:
    QAudioFormat m_format;
    qreal m_level;
    QByteArray m_buffer;
    int m_size;
};

#endif // LEVELMETERTHREAD_H
