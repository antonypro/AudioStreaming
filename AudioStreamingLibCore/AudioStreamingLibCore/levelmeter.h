#ifndef LEVELMETER_H
#define LEVELMETER_H

#include <QtCore>
#include <QtMultimedia>
#include "common.h"

class LevelMeter : public QObject
{
    Q_OBJECT
public:
    explicit LevelMeter(QObject *parent = nullptr);

signals:
    void currentlevel(float);

public slots:
    void start(const QAudioFormat &format);
    void write(const QByteArray &data);

private slots:
    void startPrivate(const QAudioFormat &format);
    void writePrivate(const QByteArray &data);
    void currentlevelPrivate();
    void process();

private:
    QAudioFormat m_format;
    float m_level;
    QByteArray m_buffer;
    QTimer *m_timer;
};

#endif // LEVELMETER_H
