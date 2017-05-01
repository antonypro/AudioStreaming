#ifndef SPECTRUMANALYZER_H
#define SPECTRUMANALYZER_H

#include <QtCore>
#include <QtMultimedia>
#include <kiss_fft.h>

// Fudge factor used to calculate the spectrum bar heights
#define SpectrumAnalyserMultiplier float(0.15)

// Needs to be a power of two
#define numSamples 1024

struct SpectrumStruct
{
    float frequency;
    float amplitude;
};

class SpectrumAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit SpectrumAnalyzer(QObject *parent = 0);
    ~SpectrumAnalyzer();

signals:
    void spectrumChanged(QVector<SpectrumStruct>);

public slots:
    void start(const QAudioFormat &format);
    void calculateSpectrum(const QByteArray &data);

private:
    bool m_initialized;
    kiss_fft_cfg m_fft;
    QVector<SpectrumStruct> m_spectrum;
    QVector<float> m_window;
    QByteArray m_spectrum_buffer;
    QAudioFormat m_format;
    int m_sample_size;
};

#endif // SPECTRUMANALYZER_H
