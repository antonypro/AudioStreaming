#include "audioinput.h"

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
    m_audio_input = nullptr;
    m_device = nullptr;

    START_THREAD
}

void AudioInput::startPrivate(const QAudioDeviceInfo &devinfo,
                              const QAudioFormat &format)
{
    m_format = m_supported_format = format;

    //Adjust to integer
    m_supported_format.setSampleSize(16);
    m_supported_format.setSampleType(QAudioFormat::SignedInt);

    //Check if format is supported by the choosen input device
    if (!devinfo.isFormatSupported(m_supported_format))
    {
        m_supported_format = devinfo.nearestFormat(m_supported_format);

        bool found_format = true;

        if (m_supported_format.sampleRate() != format.sampleRate())
            found_format = false;
        else if (m_supported_format.channelCount() != format.channelCount())
            found_format = false;
        else if (m_supported_format.byteOrder() != QAudioFormat::LittleEndian)
            found_format = false;

        if (!found_format)
        {
            emit error("Format not supported by the input device");
            return;
        }
    }

    LIB_DEBUG_LEVEL_1("Input format used by device:" << m_supported_format);

    //Initialize the audio input device
    m_audio_input = new QAudioInput(devinfo, m_supported_format, this);

    SETTONULLPTR(m_audio_input);

    m_device = m_audio_input->start();

    if (!m_device)
    {
        emit error("Failed to open input audio device");
        return;
    }

    SETTONULLPTR(m_device);

    //Call the readyReadPrivate function when data are available in the input device
    connect(m_device, &QIODevice::readyRead, this, &AudioInput::readyReadPrivate);
}

void AudioInput::start(const QAudioDeviceInfo &devinfo, const QAudioFormat &format)
{
    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection,
                              Q_ARG(QAudioDeviceInfo, devinfo),
                              Q_ARG(QAudioFormat, format));
}

void AudioInput::readyReadPrivate()
{
    //Read sound samples from input device to buffer
    QByteArray data = m_device->readAll();

    if (m_format != m_supported_format)
        data = convertSamplesToFloat(data, m_supported_format);

    LIB_DEBUG_LEVEL_2("Got" << sizeToTime(data.size(), m_format) << "ms from input device.");

    //Expose the input data to worker class
    emit readyRead(data);
}
