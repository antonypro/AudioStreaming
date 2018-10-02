#include "audiooutput.h"

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    m_audio_output = nullptr;
    m_device = nullptr;
    m_level_meter = nullptr;

    m_initialized = false;
    m_is_get_very_output_enabled = false;
    m_buffer_requested = true;
    m_play_called = false;
    m_sample_align = 0;
    m_size_to_buffer = 0;
    m_time_to_buffer = 0;
    m_last_size_to_buffer = 0;
    m_max_size_to_buffer = 0;

#ifdef IS_TO_DEBUG
    m_index = -1;
#endif

    //Smart buffer
    m_smart_buffer_enabled = false;
    m_smart_buffer_test_active = false;
    m_bytes = 0;
    m_smart_bufer_min_size = 0;
    m_bytes_read = 0;

    START_THREAD
}

void AudioOutput::startPrivate(const QAudioDeviceInfo &devinfo,
                               const QAudioFormat &format,
                               int time_to_buffer,
                               bool is_get_very_output_enabled)
{
    m_format = m_supported_format = format;

    //Adjust to integer
    m_supported_format.setSampleSize(16);
    m_supported_format.setSampleType(QAudioFormat::SignedInt);

    //Check if format is supported by the choosen output device
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
            emit error("Format not supported by the output device");
            return;
        }
    }

    LIB_DEBUG_LEVEL_2("Output format used by device:" << m_supported_format);

    int internal_buffer_size;

    //Adjust internal buffer size
    if (format.sampleRate() >= 44100)
        internal_buffer_size = (1024 * 10) * m_supported_format.channelCount();
    else if (format.sampleRate() >= 24000)
        internal_buffer_size = (1024 * 6) * m_supported_format.channelCount();
    else
        internal_buffer_size = (1024 * 4) * m_supported_format.channelCount();

    //Initialize the audio output device
    m_audio_output = new QAudioOutput(devinfo, m_supported_format, this);
    //Increase the buffer size to enable higher sample rates
    m_audio_output->setBufferSize(internal_buffer_size);

    //Factor to compensate differents sample sizes
    m_sample_align = m_supported_format.sampleSize() / float(m_format.sampleSize());

    m_time_to_buffer = time_to_buffer;
    //Compute the size in bytes to be buffered based on the current format
    m_size_to_buffer = int(timeToSize(m_time_to_buffer, m_format));
    //Define a highest size that the buffer are allowed to have in the given time
    //This value is used to discard too old buffered data
    m_max_size_to_buffer = m_size_to_buffer + int(timeToSize(MAX_BUFFERED_TIME, m_format));

    m_device = m_audio_output->start();

    if (!m_device)
    {
        emit error("Failed to open output audio device");
        return;
    }

    if (m_time_to_buffer == 0)
    {
        m_smart_buffer_enabled = true;
        m_smart_buffer_timer.start();

        m_smart_buffer_test_active = true;
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
    float volumeLevelLinear = float(0.5); //cut amplitude in half
    float volumeLevelDb = float(10 * (qLn(double(volumeLevelLinear)) / qLn(10)));
    float volumeLinear = (volume / float(100));
    m_volume = volumeLinear * float(qPow(10, (qreal(volumeLevelDb)) / 20));

    LIB_DEBUG_LEVEL_2("Volume:" << m_volume);
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
    if (!m_initialized)
        return;

    if (m_smart_buffer_test_active)
    {
        m_bytes_read += data.size();

        //Test of environment finished
        if (m_bytes_read >= timeToSize(MAX_BUFFERED_TIME, m_format))
        {
            m_bytes_read = 0; //Reset it
            m_smart_buffer_test_active = false;
        }
    }

    if (m_smart_buffer_enabled && !m_smart_buffer_test_active)
    {
        if (m_smart_buffer_timer.elapsed() >= 500)
        {
            int time_elapsed = int(m_smart_buffer_timer.restart());

            int speed = qCeil(m_bytes * qreal(500) / time_elapsed); //Nearly bytes per time

            int desired_speed = (int(timeToSize(500, m_format)) * 75 / 100); //75%

            if (speed < desired_speed)
            {
                if (m_time_to_buffer + 100 <= MAX_BUFFERED_TIME)
                {
                    m_time_to_buffer += 100;
                    m_size_to_buffer = int(timeToSize(m_time_to_buffer, m_format));
                    m_size_to_buffer = qMax(m_smart_bufer_min_size, m_size_to_buffer);

                    LIB_DEBUG_LEVEL_1("Speed:" << qRound(speed / float(1024)) << "KB/S" << "-"
                                      << "Desired:" << qRound(desired_speed / float(1024)) << "KB/S" << "-"
                                      << "Buffer adjusted:" << m_time_to_buffer << "ms");
                }
            }
            else
            {
                if (m_time_to_buffer > 0)
                {
                    m_time_to_buffer = 0;
                    m_size_to_buffer = int(timeToSize(m_time_to_buffer, m_format));
                    m_size_to_buffer = qMax(m_smart_bufer_min_size, m_size_to_buffer);

                    m_buffer.clear();

                    LIB_DEBUG_LEVEL_1("Speed:" << qRound(speed / float(1024)) << "KB/S" << "-"
                                      << "Desired:" << qRound(desired_speed / float(1024)) << "KB/S" << "-"
                                      << "Buffer adjusted:" << m_time_to_buffer << "ms");
                }
            }

            m_bytes = 0;
        }
    }

    m_bytes += data.size();

    m_buffer.append(data);

#ifdef IS_TO_DEBUG
#ifdef IS_TO_DEBUG_VERBOSE_1
    if (m_buffer_requested)
#endif
    {
        if (m_time_to_buffer > 0)
        {
            LIB_DEBUG_LEVEL_1("Buffering:" << qPrintable(QString("%0\\%1 ms - %2%").arg(sizeToTime(m_buffer.size(), m_format)).arg(m_time_to_buffer)
                                                         .arg(m_buffer.size() * 100 / m_size_to_buffer)));
        }
    }
#endif

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
    if (!m_initialized)
        return;

    //Set that last async call was triggered
    m_play_called = false;

    if (m_last_size_to_buffer != m_size_to_buffer)
    {
        if (m_buffer.size() < m_size_to_buffer)
            return;

        m_last_size_to_buffer = m_size_to_buffer;
    }

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
#ifdef IS_TO_DEBUG
        record_mutex.lock();

        qint64 index = record_count;

        record_mutex.unlock();

        if (m_index != index)
        {
            m_index = index + 1;
            LIB_DEBUG_LEVEL_1("Playing...");
        }
#endif

        //Buffer is ready and data can be played
        m_buffer_requested = false;
    }

    int readlen = m_audio_output->periodSize();

    int chunks = m_audio_output->bytesFree() / readlen;

    int aligned_len = qRound(readlen / m_sample_align);

    LIB_DEBUG_LEVEL_2("Chunks:" << chunks);

    if (m_smart_buffer_test_active)
    {
        //Test the environment for minimum buffered time required to prevent dropouts
        //Not related to network performance, but the output device performance
        int expected_buffer_size = (aligned_len * chunks * 125 / 100); // 125%

        if (m_buffer.size() < expected_buffer_size)
        {
            if (sizeToTime(m_smart_bufer_min_size, m_format) + 100 <= MAX_BUFFERED_TIME)
            {
                m_smart_bufer_min_size += timeToSize(100, m_format);

                m_size_to_buffer = qMax(m_smart_bufer_min_size, m_size_to_buffer);

                LIB_DEBUG_LEVEL_1("Minimum time to buffer:" << sizeToTime(m_smart_bufer_min_size, m_format) << "ms");
            }
        }
    }

    //Play data while it's available in the output device
    while (chunks)
    {
        //Get chunk from the buffer
        QByteArray samples = m_buffer.mid(0, aligned_len);
        int len = samples.size();
        m_buffer.remove(0, len);

        if (m_is_get_very_output_enabled)
            emit veryOutputData(samples);

        m_level_meter->write(samples);

        //Apply volume to samples
        if (!samples.isEmpty())
        {
            Eigen::Ref<Eigen::VectorXf> samplesX = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(samples.data()), samples.size() / int(sizeof(float)));
            samplesX *= m_volume;
        }

        if (!samples.isEmpty() && m_format != m_supported_format)
        {
            samples = convertSamplesToInt(samples, m_supported_format);
            len = samples.size();
        }

        //Write data to the output device after the volume was applied
        if (len)
        {
            LIB_DEBUG_LEVEL_2("Writing" << sizeToTime(samples.size(), m_supported_format) << "ms to output device.");
            m_device->write(samples);
        }

        //If chunk is smaller than the output chunk size, exit loop
        if (len != readlen)
            break;

        //Decrease the available number of chunks
        chunks--;
    }
}
