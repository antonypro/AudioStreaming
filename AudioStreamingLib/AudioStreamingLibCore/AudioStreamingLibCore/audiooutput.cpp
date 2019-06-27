#include "audiooutput.h"

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    m_initialized = false;
    m_is_get_very_output_enabled = false;
    m_buffer_requested = true;
    m_play_called = false;
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

AudioOutput::~AudioOutput()
{
    STOP_THREAD
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

    m_supported_format.setChannelCount(qMin(m_supported_format.channelCount(), 2));

    //Check if format is supported by the choosen output device
    if (!devinfo.isFormatSupported(m_supported_format))
    {
        QAudioFormat format_tmp = m_supported_format;

        m_supported_format = devinfo.nearestFormat(m_supported_format);

        bool found_format = true;

        if (m_supported_format.sampleType() != QAudioFormat::Float && m_supported_format.sampleType() != QAudioFormat::SignedInt)
            found_format = false;
        else if (m_supported_format.channelCount() > 8)
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

            emit error(QString("Format not supported by the output device\n\nFormat used:\n%0\n\nFormat supported:\n%1").arg(str1).arg(str2));

            return;
        }
    }

    m_resampler = new r8brain(this);

    connect(m_resampler, &r8brain::error, this, &AudioOutput::error);
    connect(m_resampler, &r8brain::resampled, this, &AudioOutput::resampledData);

    m_resampler->start(m_format.sampleRate(), m_supported_format.sampleRate(),
                       m_format.channelCount(), m_supported_format.channelCount(),
                       m_format.sampleSize());

    LIB_DEBUG_LEVEL_1("Format got from library:" << m_format);
    LIB_DEBUG_LEVEL_1("Output resampled format used by device:" << m_supported_format);

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

    m_time_to_buffer = time_to_buffer;
    //Compute the size in bytes to be buffered based on the current format
    m_size_to_buffer = int(timeToSize(m_time_to_buffer, m_supported_format));
    //Define a highest size that the buffer are allowed to have in the given time
    //This value is used to discard too old buffered data
    m_max_size_to_buffer = m_size_to_buffer + int(timeToSize(MAX_BUFFERED_TIME, m_supported_format));

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
    connect(timer_play, &QTimer::timeout, this, &AudioOutput::prePlay);
    timer_play->start(10);

    //Timer that checks for too old data in the buffer
    QTimer *timer_verifier = new QTimer(this);
    connect(timer_verifier, &QTimer::timeout, this, &AudioOutput::verifyBuffer);
    timer_verifier->start(qMax(m_time_to_buffer, 10));

    m_is_get_very_output_enabled = is_get_very_output_enabled;

    m_level_meter = new LevelMeter(this);

    m_level_meter->start(m_supported_format);

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

    m_volume = qMin(m_volume, float(1));

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
    if (m_resampler)
        m_resampler->write(data);
}

void AudioOutput::write(const QByteArray &data)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void AudioOutput::resampledData(const QByteArray &data)
{
    if (!m_initialized)
        return;

    QByteArray samples = data;

    if (!samples.isEmpty() && m_supported_format.sampleType() == QAudioFormat::SignedInt)
        samples = convertSamplesToInt(samples, m_supported_format);

    if (m_smart_buffer_test_active)
    {
        m_bytes_read += samples.size();

        //Test of environment finished
        if (m_bytes_read >= timeToSize(MAX_BUFFERED_TIME, m_supported_format))
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

            int desired_speed = (int(timeToSize(500, m_supported_format)) * 75 / 100); //75%

            if (speed < desired_speed)
            {
                if (m_time_to_buffer + 100 <= MAX_BUFFERED_TIME)
                {
                    m_time_to_buffer += 100;
                    m_size_to_buffer = int(timeToSize(m_time_to_buffer, m_supported_format));
                    m_size_to_buffer = qMax(m_smart_bufer_min_size, m_size_to_buffer);

                    LIB_DEBUG_LEVEL_1("Speed:" << qRound(speed / float(1024)) << "KB/S" << "-"
                                      << "Desired:" << qRound(desired_speed / float(1024)) << "KB/S" << "-"
                                      << "Buffer adjusted:" << sizeToTime(m_size_to_buffer, m_supported_format) << "ms");
                }
            }
            else
            {
                if (m_time_to_buffer > 0)
                {
                    m_time_to_buffer = 0;
                    m_size_to_buffer = int(timeToSize(m_time_to_buffer, m_supported_format));
                    m_size_to_buffer = qMax(m_smart_bufer_min_size, m_size_to_buffer);

                    m_buffer.clear();

                    LIB_DEBUG_LEVEL_1("Speed:" << qRound(speed / float(1024)) << "KB/S" << "-"
                                      << "Desired:" << qRound(desired_speed / float(1024)) << "KB/S" << "-"
                                      << "Buffer adjusted:" << sizeToTime(m_size_to_buffer, m_supported_format) << "ms");
                }
            }

            m_bytes = 0;
        }
    }

    m_bytes += samples.size();

    m_buffer.append(samples);

#ifdef IS_TO_DEBUG
#ifdef IS_TO_DEBUG_VERBOSE_1
    if (m_buffer_requested)
#endif
    {
        if (m_time_to_buffer > 0)
        {
            LIB_DEBUG_LEVEL_1("Buffering:" << qPrintable(QString("%0\\%1 ms - %2%").arg(sizeToTime(m_buffer.size(), m_supported_format))
                                                         .arg(sizeToTime(m_size_to_buffer, m_supported_format))
                                                         .arg(m_buffer.size() * 100 / m_size_to_buffer)));
        }
    }
#endif

    prePlay();
}

void AudioOutput::prePlay()
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

    LIB_DEBUG_LEVEL_2("Chunks:" << chunks);

    if (m_smart_buffer_test_active)
    {
        //Test the environment for minimum buffered time required to prevent dropouts
        //Not related to network performance, but the output device performance
        int expected_buffer_size = (readlen * chunks * 125 / 100); // 125%

        if (m_buffer.size() < expected_buffer_size)
        {
            if (sizeToTime(m_smart_bufer_min_size, m_supported_format) + 100 <= MAX_BUFFERED_TIME)
            {
                m_smart_bufer_min_size += timeToSize(100, m_supported_format);

                m_size_to_buffer = qMax(m_smart_bufer_min_size, m_size_to_buffer);

                LIB_DEBUG_LEVEL_1("Minimum time to buffer:" << sizeToTime(m_smart_bufer_min_size, m_supported_format) << "ms");
            }
        }
    }

    //Play data while it's available in the output device
    while (chunks)
    {
        //Get chunk from the buffer
        QByteArray samples = m_buffer.mid(0, readlen);
        int len = samples.size();
        m_buffer.remove(0, len);

        QByteArray samples_float = samples;

        if (m_supported_format.sampleType() == QAudioFormat::SignedInt)
            samples_float = convertSamplesToFloat(samples_float, m_supported_format);

        if (m_is_get_very_output_enabled)
            emit veryOutputData(samples_float);

        m_level_meter->write(samples_float);

        //Apply volume to samples
        if (!samples.isEmpty())
        {
            Eigen::VectorXf samplesF;

            if (m_supported_format.sampleType() == QAudioFormat::SignedInt)
            {
                VectorXint16 samplesI = Eigen::Map<VectorXint16>(reinterpret_cast<qint16*>(samples.data()), samples.size() / int(sizeof(qint16)));
                samplesF = samplesI.cast<float>();
            }
            else
            {
                samplesF = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(samples_float.data()), samples_float.size() / int(sizeof(float)));
            }

            samplesF *= m_volume;

            if (m_supported_format.sampleType() == QAudioFormat::SignedInt)
            {
                VectorXint16 samplesI = samplesF.cast<qint16>();
                samples = QByteArray(reinterpret_cast<char*>(samplesI.data()), int(samplesI.size()) * int(sizeof(qint16)));
            }
            else
            {
                samples = QByteArray(reinterpret_cast<char*>(samplesF.data()), int(samplesF.size()) * int(sizeof(float)));
            }
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
