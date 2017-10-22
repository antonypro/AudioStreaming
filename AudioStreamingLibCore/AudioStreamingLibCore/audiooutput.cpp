#include "audiooutput.h"

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    m_initialized = false;
    m_audio_output = nullptr;
    m_is_get_very_output_enabled = false;
    m_device = nullptr;
    m_buffer_requested = true;
    m_play_called = false;
    m_volume = 0;
    m_sample_align = 0;
    m_size_to_buffer = 0;
    m_time_to_buffer = 0;
    m_max_size_to_buffer = 0;
    m_level_meter = nullptr;

    START_THREAD
}

void AudioOutput::startPrivate(const QAudioDeviceInfo &devinfo,
                               const QAudioFormat &format,
                               int time_to_buffer,
                               bool is_get_very_output_enabled)
{
    m_format = m_supported_format = format;

    //Check if format is supported by the choosen output device
    if (!devinfo.isFormatSupported(m_format))
    {
        m_supported_format = devinfo.nearestFormat(m_format);

        bool found_format = true;

        if (m_supported_format.sampleRate() != format.sampleRate())
            found_format = false;
        else if (m_supported_format.channelCount() != format.channelCount())
            found_format = false;
        else if (m_supported_format.byteOrder() != QAudioFormat::LittleEndian)
            found_format = false;

        if (!found_format)
        {
            emit error("Format not supported by the output device");
            return;
        }
    }

    int internal_buffer_size;

    //Adjust internal buffer size
    if (format.sampleRate() >= 44100)
        internal_buffer_size = (1024 * 10) * m_format.channelCount();
    else if (format.sampleRate() >= 24000)
        internal_buffer_size = (1024 * 6) * m_format.channelCount();
    else
        internal_buffer_size = (1024 * 4) * m_format.channelCount();

    //Initialize the audio output device
    m_audio_output = new QAudioOutput(devinfo, m_supported_format, this);
    //Increase the buffer size to enable higher sample rates
    m_audio_output->setBufferSize(internal_buffer_size);

    //Factor to compensate differents sample sizes
    m_sample_align = m_supported_format.sampleSize() / (float)m_format.sampleSize();

    m_time_to_buffer = time_to_buffer;
    //Compute the size in bytes to be buffered based on the current format
    m_size_to_buffer = timeToSize(m_time_to_buffer, m_format);
    //Define a highest size that the buffer are allowed to have in the given time
    //This value is used to discard too old buffered data
    m_max_size_to_buffer = m_size_to_buffer + timeToSize(MAX_BUFFERED_TIME, m_format);

    m_device = m_audio_output->start();

    if (!m_device)
    {
        emit error("Failed to open output audio device");
        return;
    }

    //Timer that helps to keep playing data while it's available on the internal buffer
    QTimer *timer_play = new QTimer(this);
    timer_play->setTimerType(Qt::PreciseTimer);
    connect(timer_play, &QTimer::timeout, this, &AudioOutput::preplay);
    timer_play->start(10);

    //Timer that checks for too old data in the buffer
    QTimer *timer_verifier = new QTimer(this);
    connect(timer_verifier, &QTimer::timeout, this, &AudioOutput::verifyBuffer);
    timer_verifier->start(qMax(m_time_to_buffer, 10));

    m_is_get_very_output_enabled = is_get_very_output_enabled;

    m_level_meter = new LevelMeter();
    connect(this, &AudioOutput::destroyed, m_level_meter, &LevelMeter::deleteLater);

    m_level_meter->start(m_format);

    connect(m_level_meter, &LevelMeter::currentlevel, this, &AudioOutput::currentlevel);

    m_initialized = true;
}

void AudioOutput::start(const QAudioDeviceInfo &devinfo,
                        const QAudioFormat &format,
                        int time_to_buffer,
                        bool is_very_output_enabled)
{

    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection,
                              Q_ARG(QAudioDeviceInfo, devinfo),
                              Q_ARG(QAudioFormat, format),
                              Q_ARG(int, time_to_buffer),
                              Q_ARG(bool, is_very_output_enabled));
}

void AudioOutput::setVolumePrivate(int volume)
{
    m_volume = volume;
}

void AudioOutput::setVolume(int volume)
{
    QMetaObject::invokeMethod(this, "setVolumePrivate", Qt::QueuedConnection, Q_ARG(int, volume));
}

void AudioOutput::verifyBuffer()
{
    if (m_buffer.size() >= m_max_size_to_buffer)
        m_buffer.clear();
}

void AudioOutput::writePrivate(const QByteArray &data)
{
    m_buffer.append(data);

    preplay();
}

void AudioOutput::write(const QByteArray &data)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void AudioOutput::preplay()
{
    if (!m_initialized)
        return;

    //Verify if exists a pending call to play function
    //If not, call the play function async
    if (!m_play_called)
    {
        m_play_called = true;
        QMetaObject::invokeMethod(this, "play", Qt::QueuedConnection);
    }
}

void AudioOutput::play()
{
    //Set that last async call was triggered
    m_play_called = false;

    if (m_buffer.isEmpty())
    {
        //If data is empty set that nothing should be played
        //until the buffer has at least the minimum buffered size already set
        m_buffer_requested = true;
        return;
    }
    else if (m_buffer.size() < m_size_to_buffer)
    {
        //If buffer doesn't contains enough data,
        //check if exists a already flag telling that the buffer comes
        //from a empty state and should not play anything until have the minimum data size
        if (m_buffer_requested)
            return;
    }
    else
    {
        //Buffer is ready and data can be played
        m_buffer_requested = false;
    }

    int readlen = m_audio_output->periodSize();

    int chunks = m_audio_output->bytesFree() / readlen;

    //Play data while it's available in the output device
    while (chunks)
    {
        //Get chunk from the buffer
        QByteArray samples = m_buffer.mid(0, timeToSize(sizeToTime(qRound(readlen * m_sample_align), m_supported_format), m_format));
        int len = samples.size();
        m_buffer.remove(0, len);

        if (m_is_get_very_output_enabled)
            emit veryOutputData(samples);

        m_level_meter->write(samples);

        //Apply volume to samples
        if (!samples.isEmpty())
        {
            Eigen::Ref<Eigen::VectorXf> samplesX = Eigen::Map<Eigen::VectorXf>((float*)samples.data(), samples.size() / sizeof(float));
            samplesX *= (m_volume / (float)100);
        }

        if (!samples.isEmpty() && m_format != m_supported_format)
        {
            samples = convertSamplesToInt(samples, m_supported_format);
            len = samples.size();
        }

        //Write data to the output device after the volume was applied
        if (len)
            m_device->write(samples);

        //If chunk is smaller than the output chunk size, exit loop
        if (len != readlen)
            break;

        //Decrease the available number of chunks
        chunks--;
    }
}
