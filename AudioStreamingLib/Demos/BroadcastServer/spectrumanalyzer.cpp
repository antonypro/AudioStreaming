#include "spectrumanalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(QObject *parent) : QObject(parent)
{
    m_initialized = false;
    m_fft = nullptr;
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    m_initialized = false;

    if (m_fft)
        free(m_fft);
}

void SpectrumAnalyzer::start(const QAudioFormat &format)
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_format = format;

    m_fft = kiss_fft_alloc(numSamples, 0, nullptr, nullptr);
    m_spectrum.resize(numSamples);
    m_window.resize(numSamples);

    // Initialize window
    for (int i = 0; i < numSamples; i++)
    {
        float window = 0.5f * float(1 - qCos((2 * M_PI * i) / (numSamples - 1)));
        m_window[i] = window;
    }
}

void SpectrumAnalyzer::calculateSpectrum(const QByteArray &data)
{
    if (!m_initialized)
        return;

    m_spectrum_buffer.append(data);

    while (m_spectrum_buffer.size() >= int(numSamples * sizeof(float)))
    {
        QByteArray middle = m_spectrum_buffer.mid(0, numSamples * sizeof(float));
        int len = middle.size();
        m_spectrum_buffer.remove(0, len);

        const float *samples = reinterpret_cast<const float*>(middle.constData());

        kiss_fft_cpx inbuf[numSamples];
        kiss_fft_cpx outbuf[numSamples];

        // Initialize data array
        for (int i = 0; i < numSamples; i++)
        {
            float realSample = samples[i];
            float window = m_window[i];
            float windowedSample = realSample * window;
            inbuf[i].r = windowedSample;
            inbuf[i].i = 0;
        }

        // Calculate the FFT
        kiss_fft(m_fft, inbuf, outbuf);

        // Analyze output to obtain amplitude and phase for each frequency
        for (int i = 2; i <= numSamples / 2; i++)
        {
            // Calculate frequency of this complex sample
            m_spectrum[i].frequency = float(i * m_format.sampleRate() / numSamples);

            kiss_fft_cpx cpx = outbuf[i];

            float real = cpx.r;
            float imag = 0;

            if (i > 0 && i < numSamples / 2)
            {
                kiss_fft_cpx cpx = outbuf[numSamples / 2 + i];
                imag = cpx.r;
            }

            const float magnitude = float(qSqrt(qreal(real * real + imag * imag)));
            float amplitude = SpectrumAnalyserMultiplier * float(qLn(qreal(magnitude)));

            // Bound amplitude to [0, 1]
            amplitude = qBound(float(0), amplitude, float(1));
            m_spectrum[i].amplitude = amplitude;
        }

        emit spectrumChanged(m_spectrum);
    }
}
