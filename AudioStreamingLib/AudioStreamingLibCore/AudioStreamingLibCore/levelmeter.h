#ifndef LEVELMETER_H
#define LEVELMETER_H

#include <QtCore>
#include <QtMultimedia>
#include "common.h"
#include "threadcommon.h"

//\cond HIDDEN_SYMBOLS
class LevelMeter : public QObject
{
    Q_OBJECT
public:
    explicit LevelMeter(QObject *parent = nullptr);
    ~LevelMeter();

    CLASS_MEMBER_HEADER

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
    QPointer<QTimer> m_timer;
};
//\endcond

#endif // LEVELMETER_H
