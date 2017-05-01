#include "audioinput.h"

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
    m_audio_input = nullptr;
    m_device = nullptr;
}

void AudioInput::start(const QAudioDeviceInfo &devinfo,
                       const QAudioFormat &format)
{
    //Check if format is supported by the choosen input device
    if (!devinfo.isFormatSupported(format))
    {
        emit error("Format not supported by the input device");
        return;
    }

    m_format = format;

    //Initialize the audio input device
    m_audio_input = new QAudioInput(devinfo, m_format, this);

    m_device = m_audio_input->start();

    if (!m_device)
    {
        emit error("Failed to open input audio device");
        return;
    }

    //Call the readyReadPrivate function when data are available in the input device
    connect(m_device, &QIODevice::readyRead, this, &AudioInput::readyReadPrivate);
}

void AudioInput::readyReadPrivate()
{
    //Read sound samples from input device to buffer
    QByteArray data = m_device->readAll();
    //Expose the input data to worker class
    emit readyRead(data);
}
