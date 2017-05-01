#ifndef R8BRAIN_H
#define R8BRAIN_H

#include <QtCore>
#include <limits>
#include <r8bbase.h>
#include <CDSPResampler.h>
#include "common.h"

using namespace r8b;

class r8brain : public QObject
{
    Q_OBJECT
public:
    explicit r8brain(QObject *parent = 0);
    ~r8brain();

signals:
    void resampled(QByteArray);
    void error(QString);

public slots:
    void start(int in_sample_rate, int out_sample_rate, int channels, int sample_size);
    void write(const QByteArray &input);

private:
    int m_bits;
    int m_channels_input;
    int m_channels_output;
    CFixedBuffer<double> *m_InBufs;
    CPtrKeeper<CDSPResampler24*> *m_Resamps;
    double **m_opp;
    bool m_initalized;
};

#endif // R8BRAIN_H
