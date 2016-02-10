#ifndef R8BRAIN_H
#define R8BRAIN_H

#if _MSC_VER && !__INTEL_COMPILER
#define NOMINMAX
#endif

#include <QtCore>
#include <r8brain/r8bbase.h>
#include <r8brain/CDSPResampler.h>
#include <limits>

using namespace r8b;

#define	BUFFER_LEN 1048576

class r8brain : public QObject
{
    Q_OBJECT
public:
    explicit r8brain(QObject *parent = 0);
    ~r8brain();

signals:

public slots:
    void start(int CHANNELS, int SAMPLERATE, int BITSPERSAMPLE);
    QByteArray resample(const QByteArray &input);

private:
    int bits;
    CFixedBuffer<double> *InBufs;
    CPtrKeeper<CDSPResampler24*> *Resamps;
    double **opp;
    int channels;
};

#endif // R8BRAIN_H
