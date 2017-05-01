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
    m_size_to_buffer = 0;
    m_time_to_buffer = 0;
    m_max_size_to_buffer = 0;
    m_level_meter = nullptr;
}

void AudioOutput::start(const QAudioDeviceInfo &devinfo,
                        const QAudioFormat &format,
                        int time_to_buffer,
                        bool is_get_very_output_enabled)
{
    //Check if format is supported by the choosen output device
    if (!devinfo.isFormatSupported(format))
    {
        emit error("Format not supported by the output device");
        return;
    }

    m_format = format;

    int internal_buffer_size = 0;

    //Adjust internal buffer size
    if (format.sampleRate() >= 44100)
        internal_buffer_size = 8192 * m_format.channelCount();
    else if (format.sampleRate() >= 24000)
        internal_buffer_size = 4096 * m_format.channelCount();
    else
        internal_buffer_size = 2048 * m_format.channelCount();

    //Initialize the audio output device
    m_audio_output = new QAudioOutput(devinfo, m_format, this);
    //Increase the buffer size to enable higher sample rates
    m_audio_output->setBufferSize(internal_buffer_size);

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

    m_level_meter = new LevelMeter(this);
    m_level_meter->start(m_format);

    connect(m_level_meter, &LevelMeter::currentlevel, this, &AudioOutput::currentlevel);

    m_initialized = true;
}

void AudioOutput::setVolume(int volume)
{
    m_volume = volume;
}

void AudioOutput::verifyBuffer()
{
    if (m_buffer.size() >= m_max_size_to_buffer)
        m_buffer.clear();
}

void AudioOutput::write(const QByteArray &data)
{
    m_buffer.append(data);
    preplay();
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
        QByteArray middle = m_buffer.mid(0, readlen);
        int len = middle.size();
        m_buffer.remove(0, len);

        if (m_is_get_very_output_enabled)
            emit veryOutputData(middle);

        m_level_meter->write(middle);

        if (m_format.sampleType() != QAudioFormat::SignedInt ||
                (m_format.sampleSize() != 16 && m_format.sampleSize() != 32))
        {
            //Apply volume to the output device when the data is not compatible
            //with the directly apply volume to the samples method
            m_audio_output->setVolume(m_volume / (qreal)100);
        }
        else
        {
            //Apply volume to samples and compute level based on the sample size
            switch (m_format.sampleSize())
            {
            case 16:
            {
                int min = std::numeric_limits<qint16>::min();
                int max = std::numeric_limits<qint16>::max();

                qint16 *samples = (qint16*)middle.data();
                int size = len / sizeof(qint16);

                for (int i = 0; i < size; i++)
                {
                    int sample = (qint16)samples[i];
                    sample *= m_volume / (qreal)100;
                    sample = qBound(min, sample, max);
                    samples[i] = sample;
                }

                break;
            }
            case 32:
            {
                int min = std::numeric_limits<qint32>::min();
                int max = std::numeric_limits<qint32>::max();

                qint32 *samples = (qint32*)middle.data();
                int size = len / sizeof(qint32);

                for (int i = 0; i < size; i++)
                {
                    int sample = (qint32)samples[i];
                    sample *= m_volume / (qreal)100;
                    sample = qBound(min, sample, max);
                    samples[i] = sample;
                }

                break;
            }
            default:
                break;
            }
        }

        //Write data to the output device after the volume was applied
        if (len)
            m_device->write(middle);

        //If chunk is smaller than the output chunk size, exit loop
        if (len != readlen)
            break;

        //Decrease the available number of chunks
        chunks--;
    }
}
