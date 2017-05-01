#include "spectrumanalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(QObject *parent) : QObject(parent)
{
    m_initialized = false;
    m_fft = NULL;
    m_sample_size = 0;
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

    m_fft = kiss_fft_alloc(numSamples, 0, 0, 0);
    m_spectrum.resize(numSamples);
    m_window.resize(numSamples);

    // Initialize window
    for (int i = 0; i < numSamples; i++)
    {
        float window = 0.5 * (1 - qCos((2 * M_PI * i) / (numSamples - 1)));
        m_window[i] = window;
    }

    switch (format.sampleSize())
    {
    case 16:
        m_sample_size = sizeof(qint16);
        break;
    case 32:
        m_sample_size = sizeof(qint32);
        break;
    default:
        break;
    }
}

void SpectrumAnalyzer::calculateSpectrum(const QByteArray &data)
{
    if (!m_initialized)
        return;

    m_spectrum_buffer.append(data);

    while (m_spectrum_buffer.size() >= numSamples * m_sample_size)
    {
        QByteArray middle = m_spectrum_buffer.mid(0, numSamples * m_sample_size);
        int len = middle.size();
        m_spectrum_buffer.remove(0, len);

        qint16 *pcmSamples16 = (qint16*)middle.data();

        qint32 *pcmSamples32 = (qint32*)middle.data();

        kiss_fft_cpx inbuf[numSamples];
        kiss_fft_cpx outbuf[numSamples];

        // Initialize data array
        for (int i = 0; i < numSamples; i++)
        {
            float realSample = 0;

            // Scale down to range [-1.0, 1.0]
            switch (m_sample_size)
            {
            case 2:
                realSample = pcmSamples16[i] / float(std::numeric_limits<qint16>::max());
                break;
            case 4:
                realSample = pcmSamples32[i] / float(std::numeric_limits<qint32>::max());
                break;
            default:
                break;
            }

            float window = m_window[i];
            float windowedSample = realSample * window;
            inbuf[i].r = windowedSample;
            inbuf[i].i = 0.0;
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
            float imag = 0.0;

            if (i > 0 && i < numSamples / 2)
            {
                kiss_fft_cpx cpx = outbuf[numSamples / 2 + i];
                imag = cpx.r;
            }

            const float magnitude = qSqrt(real * real + imag * imag);
            float amplitude = SpectrumAnalyserMultiplier * qLn(magnitude);

            // Bound amplitude to [0.0, 1.0]
            amplitude = qBound(float(0.0), amplitude, float(1.0));
            m_spectrum[i].amplitude = amplitude;
        }

        emit spectrumChanged(m_spectrum);
    }
}
