#ifndef FLOWCONTROL_H
#define FLOWCONTROL_H

#include <QtCore>
#include "common.h"

//\cond HIDDEN_SYMBOLS
class FlowControl : public QObject
{
    Q_OBJECT
public:
    explicit FlowControl(QObject *parent = nullptr);

signals:
    void error(QString);
    void readyRead(QByteArray);

public slots:
    void stop();
    void start(int sample_rate, int channels, int bits_per_sample);

private slots:
    void askForBytes();

private:
    QElapsedTimer m_time;
    qint64 m_elapsed_time;

    QPointer<QTimer> m_timer;
    QPointer<r8brain> m_resampler;

    int m_sample_rate;
    int m_channels;
    int m_bits_per_sample;
};
//\endcond

#endif // FLOWCONTROL_H
