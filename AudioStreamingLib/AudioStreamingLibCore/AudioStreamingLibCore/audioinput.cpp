#include "audioinput.h"

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
    START_THREAD
}

AudioInput::~AudioInput()
{
    STOP_THREAD
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
        QAudioFormat format_tmp = m_supported_format;

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
            QString str1;
            str1.append(QString("Sample size: %0 bits\n").arg(format_tmp.sampleSize()));
            str1.append(QString("Sample rate: %0 hz\n").arg(format_tmp.sampleRate()));
            str1.append(QString("Channels: %0\n").arg(format_tmp.channelCount()));
            str1.append(QString("Sample type: %0\n").arg((format_tmp.sampleType()  == QAudioFormat::Float) ? "Float" : "Integer"));
            str1.append(QString("Byte order: %0\n").arg((format_tmp.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

            QString str2;
            str2.append(QString("Sample size: %0 bits\n").arg(m_supported_format.sampleSize()));
            str2.append(QString("Sample rate: %0 hz\n").arg(m_supported_format.sampleRate()));
            str2.append(QString("Channels: %0\n").arg(m_supported_format.channelCount()));
            str2.append(QString("Sample type: %0\n").arg((m_supported_format.sampleType()  == QAudioFormat::Float) ? "Float" : "Integer"));
            str2.append(QString("Byte order: %0\n").arg((m_supported_format.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

            emit error(QString("Format not supported by the input device\n\nFormat used:\n%0\n\nFormat supported:\n%1").arg(str1).arg(str2));

            return;
        }
    }

    LIB_DEBUG_LEVEL_1("Input format used by device:" << m_supported_format);

    //Initialize the audio input device
    m_audio_input = new QAudioInput(devinfo, m_supported_format, this);

    m_device = m_audio_input->start();

    if (!m_device)
    {
        emit error("Failed to open input audio device");
        return;
    }

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
