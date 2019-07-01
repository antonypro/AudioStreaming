#ifndef R8BRAIN_H
#define R8BRAIN_H

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif

#include <QtCore>
#include <limits>
#include <r8bbase.h>
#include <CDSPResampler.h>

//\cond HIDDEN_SYMBOLS
using namespace r8b;
//\endcond

//\cond HIDDEN_SYMBOLS
class r8brain : public QObject
{
    Q_OBJECT
public:
    explicit r8brain(QObject *parent = nullptr);
    ~r8brain();

signals:
    void resampled(QByteArray);
    void error(QString);

public slots:
    void start(int in_sample_rate, int out_sample_rate, int in_channels, int out_channels, int sample_size);
    void write(const QByteArray &input);

private slots:
    void startPrivate(int in_sample_rate, int out_sample_rate, int in_channels, int out_channels, int sample_size);
    void writePrivate(const QByteArray &input);

private:
    int m_bits;
    int m_channels_input;
    int m_channels_output;
    CFixedBuffer<double> *m_in_bufs;
    CPtrKeeper<CDSPResampler24*> *m_resamps;
    double **m_opp;
    bool m_initialized;
};
//\endcond

#endif // R8BRAIN_H
