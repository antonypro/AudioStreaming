#ifndef FLOWCONTROL_H
#define FLOWCONTROL_H

#include <QtCore>
#include "common.h"

class FlowControl : public QObject
{
    Q_OBJECT
public:
    explicit FlowControl(QObject *parent = 0);

signals:
    void error(QString);
    void getbytes(int);

public slots:
    void start(int sample_rate, int channels, int bits_per_sample);

private slots:
    void askforbytes();

private:
    QElapsedTimer m_time;
    qint64 m_elapsed_time;

    QTimer *m_timer;

    int m_sample_rate;
    int m_channels;
    int m_bits_per_sample;
};

#endif // FLOWCONTROL_H
