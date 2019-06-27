#include "audiostreamingworker.h"

#if defined (IS_TO_DEBUG_VERBOSE_1) || defined (IS_TO_DEBUG_VERBOSE_2)
QMutex record_mutex;
qint64 record_count = 0;
#endif

#define INVALID_FORMAT\
    (m_streaming_info.inputAudioFormat().isValid() &&\
    (m_streaming_info.inputAudioFormat().sampleType() != QAudioFormat::Float ||\
    m_streaming_info.inputAudioFormat().sampleSize() != 32 ||\
    m_streaming_info.inputAudioFormat().byteOrder() != QAudioFormat::LittleEndian ||\
    m_streaming_info.inputAudioFormat().sampleRate() < 8000 ||\
    m_streaming_info.inputAudioFormat().sampleRate() > 192000))\
    ||\
    (m_streaming_info.audioFormat().isValid() &&\
    (m_streaming_info.audioFormat().sampleType() != QAudioFormat::Float ||\
    m_streaming_info.audioFormat().sampleSize() != 32 ||\
    m_streaming_info.audioFormat().byteOrder() != QAudioFormat::LittleEndian ||\
    m_streaming_info.audioFormat().sampleRate() < 8000 ||\
    m_streaming_info.audioFormat().sampleRate() > 192000))\

AudioStreamingWorker::AudioStreamingWorker(QObject *parent) : QObject(parent)
{
    m_is_walkie_talkie = false;
    m_has_error = false;
    m_input_muted = false;
    m_volume = 0;
    m_ready_to_write_extra_data = false;
    m_extra_data_peers = 0;
    m_callback_enabled = false;
    m_input_device_running = false;
    m_output_device_running = false;
    m_workers_started_later = false;
    m_input_device_changing = false;
    m_output_device_changing = false;

#ifdef OPUS
    m_frame_size = 0;
    m_max_frame_size = 0;
    m_bitrate = 0;
#endif

    START_THREAD
}

AudioStreamingWorker::~AudioStreamingWorker()
{
    STOP_THREAD
}

void AudioStreamingWorker::start(const AudioStreamingLibInfo &streaming_info)
{
    m_streaming_info = streaming_info;

    if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::Undefined)
    {
        errorPrivate("Mode of work not defined!");
        return;
    }

    if (m_streaming_info.workMode() != AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer &&
            m_streaming_info.isListenAudioInputEnabled())
    {
        errorPrivate("Listen input is allowed only on broadcast mode!");
        return;
    }

    if (INVALID_FORMAT)
    {
        errorPrivate("Invalid format!");
        return;
    }

    switch (m_streaming_info.workMode())
    {
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient:
    {
        if (m_streaming_info.isEncryptionEnabled())
            m_client = new EncryptedClient(this);
        else
            m_client = new Client(this);

        connect(m_client, &AbstractClient::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_client, &AbstractClient::connected, this, &AudioStreamingWorker::clientConencted);
        connect(m_client, &AbstractClient::disconnected, this, &AudioStreamingWorker::clientDisconencted);
        connect(m_client, &AbstractClient::readyRead, this, &AudioStreamingWorker::processClientInput);

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

        m_server_discover = new DiscoverServer(this);

        if (m_streaming_info.isEncryptionEnabled())
            m_server = new EncryptedServer(this);
        else
            m_server = new Server(this);

        connect(m_server, &AbstractServer::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_server, &AbstractServer::connected, this, &AudioStreamingWorker::serverClientConencted);
        connect(m_server, &AbstractServer::disconnected, this, &AudioStreamingWorker::serverClientDisconencted);
        connect(m_server, &AbstractServer::pending, this, &AudioStreamingWorker::pending);
        connect(m_server, &AbstractServer::readyRead, this, &AudioStreamingWorker::processServerInput);

        adjustSettingsPrivate(false);

        startAudioWorkers();

        restartActiveWorkers();

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer:
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient:
    {
        m_is_walkie_talkie = true;

        if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient)
        {
            if (m_streaming_info.isEncryptionEnabled())
                m_client = new EncryptedClient(this);
            else
                m_client = new Client(this);

            connect(m_client, &AbstractClient::error, this, &AudioStreamingWorker::errorPrivate);
            connect(m_client, &AbstractClient::connected, this, &AudioStreamingWorker::clientConencted);
            connect(m_client, &AbstractClient::disconnected, this, &AudioStreamingWorker::clientDisconencted);
            connect(m_client, &AbstractClient::readyRead, this, &AudioStreamingWorker::processClientInput);
        }
        else if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer)
        {
            m_server_discover = new DiscoverServer(this);

            if (m_streaming_info.isEncryptionEnabled())
                m_server = new EncryptedServer(this);
            else
                m_server = new Server(this);

            connect(m_server, &AbstractServer::error, this, &AudioStreamingWorker::errorPrivate);
            connect(m_server, &AbstractServer::connected, this, &AudioStreamingWorker::serverClientConencted);
            connect(m_server, &AbstractServer::disconnected, this, &AudioStreamingWorker::serverClientDisconencted);
            connect(m_server, &AbstractServer::pending, this, &AudioStreamingWorker::pending);
            connect(m_server, &AbstractServer::readyRead, this, &AudioStreamingWorker::processServerInput);

            adjustSettingsPrivate(false);

            startAudioWorkers();

            restartActiveWorkers();
        }

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::WebClient:
    {
        if (m_streaming_info.isEncryptionEnabled())
        {
            m_client = new WebClient(this);
        }
        else
        {
            errorPrivate("WebClient requires encryption enabled!");
            return;
        }

        m_is_walkie_talkie = true;

        connect(m_client, &AbstractClient::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_client, &AbstractClient::connectedToServer, this, &AudioStreamingWorker::webClientConencted);
        connect(m_client, &AbstractClient::connectedToPeer, this, &AudioStreamingWorker::webClientConnectedToPeer);
        connect(m_client, &AbstractClient::disconnectedFromPeer, this, &AudioStreamingWorker::webClientDisconnected);
        connect(m_client, &AbstractClient::pending, this, &AudioStreamingWorker::pending);
        connect(m_client, &AbstractClient::webClientLoggedIn, this, &AudioStreamingWorker::webClientLoggedIn);
        connect(m_client, &AbstractClient::webClientWarning, this, &AudioStreamingWorker::webClientWarning);
        connect(m_client, &AbstractClient::readyRead, this, &AudioStreamingWorker::processWebClientInput);
        connect(m_client, &AbstractClient::commandXML, this, &AudioStreamingWorker::commandXML);

        adjustSettingsPrivate(false);

        break;
    }
    default:
        break;
    }
}

void AudioStreamingWorker::startInputAudioWorkers()
{
    switch (m_streaming_info.workMode())
    {
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient:
    {
        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();
#ifdef OPUS
        m_resampler = new r8brain(this);
        m_opus_enc = new OpusEncoderClass(this);

        connect(m_resampler, &r8brain::error, this, &AudioStreamingWorker::warningInputPrivate);
        connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

        if (m_streaming_info.isGetAudioEnabled() && !m_streaming_info.isListenAudioInputEnabled())
            connect(m_resampler, &r8brain::resampled, this, &AudioStreamingWorker::veryInputData);

        connect(m_opus_enc, &OpusEncoderClass::encoded, this, &AudioStreamingWorker::posProcessedInput);

        connect(m_opus_enc, &OpusEncoderClass::error, this, &AudioStreamingWorker::warningInputPrivate);
#endif
        if (m_streaming_info.inputDeviceType() == AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput(this);

            connect(m_audio_input, &AudioInput::error, this, &AudioStreamingWorker::warningInputPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputDataBack);
        }
        else
        {
            QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

            m_flow_control = new FlowControl(this);

            connect(m_flow_control, &FlowControl::readyRead, this, &AudioStreamingWorker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &AudioStreamingWorker::error);
            m_flow_control->start(inputFormat.sampleRate(), inputFormat.channelCount(), inputFormat.sampleSize());
        }

        m_level_meter_input = new LevelMeter(this);

        connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);

        if (m_streaming_info.isListenAudioInputEnabled())
        {
            if (!m_audio_output)
            {
                m_audio_output = new AudioOutput(this);

                connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::warningOutputPrivate);
                connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);
                if (m_streaming_info.isGetAudioEnabled())
                    connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryInputData);
                connect(m_audio_output, &AudioOutput::veryOutputData, m_level_meter_input, &LevelMeter::write);
#ifdef OPUS
                connect(m_resampler, &r8brain::resampled, m_audio_output, &AudioOutput::write);
#endif
                m_audio_output->setVolume(m_volume);

                m_audio_output->start(m_streaming_info.outputDeviceInfo(),
                                      m_streaming_info.audioFormat(),
                                      m_streaming_info.timeToBuffer(),
                                      m_streaming_info.isGetAudioEnabled());
            }
            else
            {
                connect(m_audio_output, &AudioOutput::veryOutputData, m_level_meter_input, &LevelMeter::write);
#ifdef OPUS
                connect(m_resampler, &r8brain::resampled, m_audio_output, &AudioOutput::write);
#endif
            }
        }

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer:
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

#ifdef OPUS
        m_resampler = new r8brain(this);
        m_opus_enc = new OpusEncoderClass(this);

        connect(m_opus_enc, &OpusEncoderClass::error, this, &AudioStreamingWorker::warningInputPrivate);

        connect(m_resampler, &r8brain::error, this, &AudioStreamingWorker::warningInputPrivate);
        connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

        if (m_streaming_info.isGetAudioEnabled())
            connect(m_resampler, &r8brain::resampled, this, &AudioStreamingWorker::veryInputData);

        connect(m_opus_enc, &OpusEncoderClass::encoded, this, &AudioStreamingWorker::posProcessedInput);
#endif
        if (m_streaming_info.inputDeviceType() == AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput(this);

            connect(m_audio_input, &AudioInput::error, this, &AudioStreamingWorker::warningInputPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputDataBack);

            m_level_meter_input = new LevelMeter(this);

            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);
        }
        else //used to compute level if not using the library input device
        {
            m_flow_control = new FlowControl(this);

            connect(m_flow_control, &FlowControl::readyRead, this, &AudioStreamingWorker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &AudioStreamingWorker::error);

            m_level_meter_input = new LevelMeter(this);

            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);

            if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

                m_flow_control->start(inputFormat.sampleRate(),
                                      inputFormat.channelCount(),
                                      inputFormat.sampleSize());
            }
        }

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::WebClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

#ifdef OPUS
        m_resampler = new r8brain(this);
        m_opus_enc = new OpusEncoderClass(this);
        connect(m_opus_enc, &OpusEncoderClass::error, this, &AudioStreamingWorker::warningInputPrivate);

        connect(m_resampler, &r8brain::error, this, &AudioStreamingWorker::warningInputPrivate);
        connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

        if (m_streaming_info.isGetAudioEnabled())
            connect(m_resampler, &r8brain::resampled, this, &AudioStreamingWorker::veryInputData);

        connect(m_opus_enc, &OpusEncoderClass::encoded, this, &AudioStreamingWorker::posProcessedInput);
#endif
        if (m_streaming_info.inputDeviceType() == AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput(this);

            connect(m_audio_input, &AudioInput::error, this, &AudioStreamingWorker::warningInputPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputDataBack);

            m_level_meter_input = new LevelMeter(this);

            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);
        }
        else //used to compute level if not using the library input device
        {
            m_flow_control = new FlowControl(this);

            connect(m_flow_control, &FlowControl::readyRead, this, &AudioStreamingWorker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &AudioStreamingWorker::error);

            QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

            m_level_meter_input = new LevelMeter(this);

            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);

            m_flow_control->start(inputFormat.sampleRate(),
                                  inputFormat.channelCount(),
                                  inputFormat.sampleSize());
        }

        break;
    }
    default:
        break;
    }

    m_input_device_running = true;
}

void AudioStreamingWorker::startOutputAudioWorkers()
{
    switch (m_streaming_info.workMode())
    {
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();
#ifdef OPUS
        m_opus_dec = new OpusDecoderClass(this);

        connect(m_opus_dec, &OpusDecoderClass::decoded, this, &AudioStreamingWorker::posProcessedOutput);

        connect(m_opus_dec, &OpusDecoderClass::error, this, &AudioStreamingWorker::warningOutputPrivate);
#endif
        if (m_streaming_info.outputDeviceType() == AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput(this);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryOutputData);

            connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::warningOutputPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter(this);

            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &AudioStreamingWorker::outputLevel);
        }

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer:
    {
        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer:
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

#ifdef OPUS
        m_opus_dec = new OpusDecoderClass(this);

        connect(m_opus_dec, &OpusDecoderClass::error, this, &AudioStreamingWorker::warningOutputPrivate);

        connect(m_opus_dec, &OpusDecoderClass::decoded, this, &AudioStreamingWorker::posProcessedOutput);
#endif

        if (m_streaming_info.outputDeviceType() == AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput(this);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryOutputData);

            connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::warningOutputPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter(this);

            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &AudioStreamingWorker::outputLevel);
        }

        break;
    }
    case AudioStreamingLibInfo::StreamingWorkMode::WebClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

#ifdef OPUS
        m_opus_dec = new OpusDecoderClass(this);

        connect(m_opus_dec, &OpusDecoderClass::error, this, &AudioStreamingWorker::warningOutputPrivate);

        connect(m_opus_dec, &OpusDecoderClass::decoded, this, &AudioStreamingWorker::posProcessedOutput);
#endif

        if (m_streaming_info.outputDeviceType() == AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput(this);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryOutputData);

            connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::warningOutputPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter(this);

            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &AudioStreamingWorker::outputLevel);
        }

        break;
    }
    default:
        break;
    }

    m_output_device_running = true;
}

void AudioStreamingWorker::startAudioWorkers()
{
    startInputAudioWorkers();
    startOutputAudioWorkers();
}

void AudioStreamingWorker::restartActiveWorkers()
{
    if (m_audio_input)
        m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());

    if (m_level_meter_input)
        m_level_meter_input->start(m_streaming_info.inputAudioFormat());

    if (m_streaming_info.workMode() !=  AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer)
    {
        if (m_audio_output)
            m_audio_output->start(m_streaming_info.outputDeviceInfo(),
                                  m_streaming_info.audioFormat(),
                                  m_streaming_info.timeToBuffer(),
                                  m_streaming_info.isGetAudioEnabled());
    }

    if (m_level_meter_output)
        m_level_meter_output->start(m_streaming_info.audioFormat());

#ifdef OPUS
    startOpusEncoder();
    startOpusDecoder();
#endif

    m_workers_started_later = true;
}

void AudioStreamingWorker::stopInputAudioWorkers()
{
    if (m_audio_input)
        m_audio_input->deleteLater();
    if (m_flow_control)
        m_flow_control->deleteLater();
#ifdef OPUS
    if (m_resampler)
        m_resampler->deleteLater();
    if (m_opus_enc)
        m_opus_enc->deleteLater();
#endif
    if (m_level_meter_input)
        m_level_meter_input->deleteLater();
}

void AudioStreamingWorker::stopOutputAudioWorkers()
{
    if (m_audio_output)
        m_audio_output->deleteLater();
#ifdef OPUS
    if (m_opus_dec)
        m_opus_dec->deleteLater();
#endif
    if (m_level_meter_output)
        m_level_meter_output->deleteLater();
}

void AudioStreamingWorker::stopAudioWorkers()
{
    stopInputAudioWorkers();
    stopOutputAudioWorkers();

    m_input_device_running = false;
    m_output_device_running = false;

    m_workers_started_later = false;
}

void AudioStreamingWorker::changeInputDevice(const QAudioDeviceInfo &dev_info)
{
    if (m_streaming_info.inputDeviceType() != AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
    {
        emit warning("The input device type isn't 'LibraryAudioDevice'");
        return;
    }

    if (!m_input_device_running)
    {
        emit warning("No input device running, nothing to change!'");
        return;
    }

    if (m_input_device_changing)
    {
        QMetaObject::invokeMethod(this, "changeInputDevice", Qt::QueuedConnection, Q_ARG(QAudioDeviceInfo, dev_info));
        return;
    }

    m_input_device_changing = true;

    if (m_audio_input)
        connect(m_audio_input, &QObject::destroyed, this, &AudioStreamingWorker::restartInputLater, Qt::UniqueConnection);
    if (m_flow_control)
        connect(m_flow_control, &QObject::destroyed, this, &AudioStreamingWorker::restartInputLater, Qt::UniqueConnection);
#ifdef OPUS
    if (m_resampler)
        connect(m_resampler, &QObject::destroyed, this, &AudioStreamingWorker::restartInputLater, Qt::UniqueConnection);
    if (m_opus_enc)
        connect(m_opus_enc, &QObject::destroyed, this, &AudioStreamingWorker::restartInputLater, Qt::UniqueConnection);
#endif
    if (m_level_meter_input)
        connect(m_level_meter_input, &QObject::destroyed, this, &AudioStreamingWorker::restartInputLater, Qt::UniqueConnection);

    stopInputAudioWorkers();

    m_streaming_info.setInputDeviceInfo(dev_info);

    restartInputLater();
}

void AudioStreamingWorker::restartInputLater()
{
    if (m_audio_input)
        return;
    if (m_flow_control)
        return;
#ifdef OPUS
    if (m_resampler)
        return;
    if (m_opus_enc)
        return;
#endif
    if (m_level_meter_input)
        return;

    startInputAudioWorkers();

    if (m_workers_started_later)
        restartActiveWorkers();

    m_input_device_changing = false;
}

void AudioStreamingWorker::changeOutputDevice(const QAudioDeviceInfo &dev_info)
{
    if (m_streaming_info.outputDeviceType() != AudioStreamingLibInfo::AudioDeviceType::LibraryAudioDevice)
    {
        emit warning("The output device type isn't 'LibraryAudioDevice'");
        return;
    }

    if (!m_output_device_running)
    {
        emit warning("No output device running, nothing to change!'");
        return;
    }

    if (m_output_device_changing)
    {
        QMetaObject::invokeMethod(this, "changeOutputDevice", Qt::QueuedConnection, Q_ARG(QAudioDeviceInfo, dev_info));
        return;
    }

    m_output_device_changing = true;

    if (m_audio_output)
        connect(m_audio_output, &QObject::destroyed, this, &AudioStreamingWorker::restartOutputLater, Qt::UniqueConnection);
#ifdef OPUS
    if (m_opus_dec)
        connect(m_opus_dec, &QObject::destroyed, this, &AudioStreamingWorker::restartOutputLater, Qt::UniqueConnection);
#endif
    if (m_level_meter_output)
        connect(m_level_meter_output, &QObject::destroyed, this, &AudioStreamingWorker::restartOutputLater, Qt::UniqueConnection);

    stopOutputAudioWorkers();

    m_streaming_info.setOutputDeviceInfo(dev_info);

    restartOutputLater();
}

void AudioStreamingWorker::restartOutputLater()
{
    if (m_audio_output)
        return;
#ifdef OPUS
    if (m_opus_dec)
        return;
#endif
    if (m_level_meter_output)
        return;

    startOutputAudioWorkers();

    if (m_workers_started_later)
        restartActiveWorkers();

    m_output_device_changing = false;
}

void AudioStreamingWorker::listen(quint16 port, bool auto_accept, const QByteArray &password, int max_connections)
{
    if (m_server_discover)
    {
        m_server_discover->listen(port, m_streaming_info.negotiationString(), m_streaming_info.ID());
    }
    else
    {
        errorPrivate("Not started in server mode!");
        return;
    }

    if (m_server)
    {
        m_server->listen(port, auto_accept, !m_is_walkie_talkie ? max_connections : 1,
                         m_streaming_info.negotiationString(), m_streaming_info.ID(), password);
    }
    else
    {
        errorPrivate("Not started in server mode!");
        return;
    }
}

void AudioStreamingWorker::connectToHost(const QString &host, quint16 port, const QByteArray &password)
{
    if (m_client)
    {
        m_client->connectToHost(host, port, m_streaming_info.negotiationString(), m_streaming_info.ID(), password);
    }
    else
    {
        errorPrivate("Not started in client mode!");
        return;
    }
}

void AudioStreamingWorker::writeCommandXML(const QByteArray &XML)
{
    if (m_client)
        m_client->writeCommandXML(XML);
}

void AudioStreamingWorker::connectToPeer(const QString &ID)
{
    if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::WebClient)
    {
        m_client->connectToPeer(ID);
    }
}

void AudioStreamingWorker::disconnectFromPeer()
{
    if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::WebClient)
    {
        m_client->disconnectFromPeer();
    }
}

void AudioStreamingWorker::acceptSslCertificate()
{
    if (m_client)
        m_client->acceptSslCertificate();
}

void AudioStreamingWorker::acceptConnection()
{
    if (m_server)
        m_server->acceptNewConnection();
    else if (m_client)
        m_client->acceptConnection();
}

void AudioStreamingWorker::rejectConnection()
{
    if (m_server)
        m_server->rejectNewConnection();
    else if (m_client)
        m_client->rejectConnection();
}

void AudioStreamingWorker::warningInputPrivate(const QString &warning_description)
{
    stopInputAudioWorkers();

    emit warning(warning_description);
}

void AudioStreamingWorker::warningOutputPrivate(const QString &warning_description)
{
    stopOutputAudioWorkers();

    emit warning(warning_description);
}

void AudioStreamingWorker::errorPrivate(const QString &error_description)
{
    if (m_has_error)
        return;

    m_has_error = true;

    emit error(error_description);
}

void AudioStreamingWorker::startOpusEncoder()
{
#ifdef OPUS
    int application = 0;

    switch (m_streaming_info.workMode())
    {
    case AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer:
        application = OPUS_APPLICATION_AUDIO;
        break;
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient:
    case AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieServer:
    case AudioStreamingLibInfo::StreamingWorkMode::WebClient:
        application = OPUS_APPLICATION_VOIP;
        break;
    default:
        break;
    }

    QAudioFormat inputAudioFormat = m_streaming_info.inputAudioFormat();
    QAudioFormat audioFormat = m_streaming_info.audioFormat();

    if (m_resampler)
        m_resampler->start(inputAudioFormat.sampleRate(),
                           audioFormat.sampleRate(),
                           inputAudioFormat.channelCount(),
                           audioFormat.channelCount(),
                           inputAudioFormat.sampleSize());

    if (m_opus_enc)
        m_opus_enc->start(audioFormat.sampleRate(),
                          audioFormat.channelCount(),
                          m_bitrate,
                          m_frame_size,
                          application);
#endif
}

void AudioStreamingWorker::startOpusDecoder()
{
#ifdef OPUS
    QAudioFormat format = m_streaming_info.audioFormat();

    if (m_opus_dec)
        m_opus_dec->start(format.sampleRate(),
                          format.channelCount(),
                          m_frame_size,
                          m_max_frame_size);
#endif
}

void AudioStreamingWorker::adjustSettingsPrivate(bool client_mode)
{
    QAudioFormat format = m_streaming_info.inputAudioFormat();

#ifdef OPUS
    //Change the settings to Opus compatible settings
    if (!client_mode)
    {
        int input_sample_rate = format.sampleRate();

        int opus_sample_rate = 0;

        int frame_size = 0;

        if (input_sample_rate <= 8000)
        {
            opus_sample_rate = 8000;
            frame_size = 80; //10 ms
        }
        else if (input_sample_rate <= 12000)
        {
            opus_sample_rate = 12000;
            frame_size = 120; //10 ms
        }
        else if (input_sample_rate <= 16000)
        {
            opus_sample_rate = 16000;
            frame_size = 160; //10 ms
        }
        else if (input_sample_rate <= 24000)
        {
            opus_sample_rate = 24000;
            frame_size = 240; //10 ms
        }
        else
        {
            opus_sample_rate = 48000;
            frame_size = 480; //10 ms
        }

        m_frame_size = frame_size;

        m_max_frame_size = 6 * frame_size;

        format.setSampleRate(opus_sample_rate);
        format.setChannelCount(qMin(format.channelCount(), 2));

        m_bitrate = m_streaming_info.OpusBitrate();

        m_streaming_info.setAudioFormat(format);
    }
#else
    Q_UNUSED(client_mode)

    m_streaming_info.setAudioFormat(format);
#endif

    emit adjustSettings();
}

//Client connected to the app, currently a server
void AudioStreamingWorker::serverClientConencted(const PeerData &pd, const QString &id)
{
    //Send the audio format to the peer
    QByteArray data;
    data.append(getBytes<quint8>(Command::AudioHeader));
    data.append(createHeader());

    m_server->writeToHost(data, pd.descriptor);

    m_ready_to_write_extra_data = true;

    m_id_connections_list.append(pd.descriptor);
    m_host_connections_list.append(pd.host);

    m_hash_heart_beat[pd.descriptor] = 0;
    m_hash_heart_beat_time[pd.descriptor].start();

    emit connected(pd.host, id);
}

//Client disconnected from the app, currently a server
void AudioStreamingWorker::serverClientDisconencted(const PeerData &pd)
{
    if (m_is_walkie_talkie)
    {
        m_ready_to_write_extra_data = false;
    }
    else
    {
        m_extra_data_peers--;
        m_extra_data_peers = qMax(m_extra_data_peers, 0);

        if (m_extra_data_peers == 0)
            m_ready_to_write_extra_data = false;
    }

    int index = m_id_connections_list.indexOf(pd.descriptor);

    if (index >= 0)
    {
        m_id_connections_list.removeAt(index);
        m_host_connections_list.removeAt(index);
    }

    m_hash_heart_beat.remove(pd.descriptor);
    m_hash_heart_beat_time.remove(pd.descriptor);

    emit disconnected(pd.host);
}

//The app is currently connected to the server
void AudioStreamingWorker::clientConencted(const PeerData &pd, const QString &id)
{
    m_ready_to_write_extra_data = true;

    m_id_connections_list.append(pd.descriptor);
    m_host_connections_list.append(pd.host);

    m_hash_heart_beat[pd.descriptor] = 0;
    m_hash_heart_beat_time[pd.descriptor].start();

    emit connected(pd.host, id);
}

//The app currently disconnected from the server
void AudioStreamingWorker::clientDisconencted(const PeerData &pd)
{
    if (m_is_walkie_talkie)
    {
        m_ready_to_write_extra_data = false;
    }
    else
    {
        m_extra_data_peers--;
        m_extra_data_peers = qMax(m_extra_data_peers, 0);

        if (m_extra_data_peers == 0)
            m_ready_to_write_extra_data = false;
    }

    int index = m_id_connections_list.indexOf(pd.descriptor);

    if (index >= 0)
    {
        m_id_connections_list.removeAt(index);
        m_host_connections_list.removeAt(index);
    }

    m_hash_heart_beat.remove(pd.descriptor);
    m_hash_heart_beat_time.remove(pd.descriptor);

    emit disconnected(pd.host);
}

void AudioStreamingWorker::webClientConencted(const QByteArray &hash)
{
    emit connectedToServer(hash);
}

//The app is currently connected to another peer
void AudioStreamingWorker::webClientConnectedToPeer(const QString &id)
{
    //Send the audio format to the peer
    QByteArray data;
    data.append(getBytes<quint8>(Command::AudioHeader));
    data.append(createHeader());

    m_client->write(data);

    m_ready_to_write_extra_data = true;

    m_id_connections_list.append(0);

    m_hash_heart_beat[0] = 0;
    m_hash_heart_beat_time[0].start();

    emit connected(QHostAddress(), id);
}

//The app currently disconnected from another peer
void AudioStreamingWorker::webClientDisconnected()
{
    stopAudioWorkers();

    m_ready_to_write_extra_data = false;

    int index = m_id_connections_list.indexOf(0);

    if (index >= 0)
        m_id_connections_list.removeAt(index);

    m_hash_heart_beat.remove(0);
    m_hash_heart_beat_time.remove(0);

    emit disconnected(QHostAddress());
}

//Write data that can be or not relacted to audio streamed
void AudioStreamingWorker::writeExtraData(const QByteArray &data)
{
    QByteArray result;
    result.append(getBytes<quint8>(Command::ExtraData));
    result.append(data);

    if (m_server)
        m_extra_data_peers = m_server->writeToAll(result);
    else if (m_client)
        m_extra_data_peers = m_client->write(result);

    if (m_extra_data_peers > 0)
        m_ready_to_write_extra_data = false;
}

//Tell to the peer that the extra data was sent and now it's ready to more
void AudioStreamingWorker::writeExtraDataResult()
{
    QByteArray result;
    result.append(getBytes<quint8>(Command::ExtraDataReceived));

    if (m_server)
        m_server->writeToAll(result);
    else if (m_client)
        m_client->write(result);
}

//Receive the input callback data back
//and encode audio if using with Opus codec
void AudioStreamingWorker::inputDataBack(const QByteArray &data)
{
    if (data.size() % int(sizeof(float)) != 0)
    {
        errorPrivate("Corrupted audio data!");
        return;
    }

    QByteArray middle = data;

    if (m_input_muted && !middle.isEmpty())
    {
        Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(middle.data()), middle.size() / int(sizeof(float)));
        samples.fill(0);
    }
    else if (!middle.isEmpty())
    {
        int middle_size = middle.size() / int(sizeof(float));

        float *middle_data = reinterpret_cast<float*>(middle.data());

        for (int i = 0; i < middle_size; i++)
            middle_data[i] = qBound(float(-1), middle_data[i], float(1));
    }

    if (!m_streaming_info.isListenAudioInputEnabled() && m_level_meter_input)
        m_level_meter_input->write(middle);

#ifdef OPUS
    m_resampler->write(middle);
#else
    if (m_streaming_info.isGetAudioEnabled() && !m_streaming_info.isListenAudioInputEnabled())
        emit veryInputData(middle);

    if (m_streaming_info.isListenAudioInputEnabled() && m_audio_output)
        m_audio_output->write(middle);

    posProcessedInput(middle);
#endif
}

void AudioStreamingWorker::posProcessedInput(const QByteArray &data)
{
    QByteArray result;
    result.append(getBytes<quint8>(Command::AudioData));
    result.append(data);

    if (m_server)
    {
        for (qintptr descriptor : m_id_connections_list)
        {
            if (!m_hash_heart_beat.contains(descriptor))
                continue;

            qint32 &beats = m_hash_heart_beat[descriptor];
            QElapsedTimer &time = m_hash_heart_beat_time[descriptor];

            if (time.elapsed() < MAX_BUFFERED_TIME || beats <= MAX_PENDING_BEATS)
            {
                LIB_DEBUG_LEVEL_2("Sent Audio Data - Size:" << result.size() << "bytes.");

                beats++;

                m_server->writeToHost(result, descriptor);
            }
            else
            {
                LIB_DEBUG_LEVEL_2("Not Sent Audio Data - Size:" << result.size() << "bytes.");
            }
        }
    }
    else if (m_client)
    {
        for (qintptr descriptor : m_id_connections_list)
        {
            if (!m_hash_heart_beat.contains(descriptor))
                continue;

            qint32 &beats = m_hash_heart_beat[descriptor];
            QElapsedTimer &time = m_hash_heart_beat_time[descriptor];

            if (time.elapsed() < MAX_BUFFERED_TIME || beats <= MAX_PENDING_BEATS)
            {
                LIB_DEBUG_LEVEL_2("Sent Audio Data - Size:" << result.size() << "bytes.");

                beats++;

                m_client->write(result);
            }
            else
            {
                LIB_DEBUG_LEVEL_2("Not Sent Audio Data - Size:" << result.size() << "bytes.");
            }
        }
    }
}

//Decode audio if using Opus codec
void AudioStreamingWorker::preProcessOutput(const QByteArray &data)
{
#ifdef OPUS
    m_opus_dec->write(data);
#else
    posProcessedOutput(data);
#endif
}

void AudioStreamingWorker::posProcessedOutput(const QByteArray &data)
{
    if (m_level_meter_output)
        m_level_meter_output->write(data);

    if (m_callback_enabled)
        emit outputData(data);
    else
        outputDataBack(data);
}

//Send callback output data to the output device
void AudioStreamingWorker::outputDataBack(const QByteArray &data)
{
    if (data.size() % int(sizeof(float)) != 0)
    {
        errorPrivate("Corrupted audio data!");
        return;
    }

    if (m_audio_output)
        m_audio_output->write(data);
}

//Convert flow control bytes to pseudo audio data
void AudioStreamingWorker::flowControl(const QByteArray &data)
{
    if (m_callback_enabled)
        emit inputData(data);
    else
        inputDataBack(data);
}

bool AudioStreamingWorker::isInputMuted() const
{
    return m_input_muted;
}

void AudioStreamingWorker::setInputMuted(bool mute)
{
    m_input_muted = mute;
}

int AudioStreamingWorker::volume() const
{
    return m_volume;
}

void AudioStreamingWorker::setVolume(int volume)
{
    m_volume = volume;

    if (m_audio_output)
        m_audio_output->setVolume(m_volume);
}

AudioStreamingLibInfo AudioStreamingWorker::audioStreamingLibInfo() const
{
    return m_streaming_info;
}

QList<QHostAddress> AudioStreamingWorker::connectionsList() const
{
    return m_host_connections_list;
}

bool AudioStreamingWorker::isReadyToWriteExtraData() const
{
    return m_ready_to_write_extra_data;
}

void AudioStreamingWorker::processServerInput(const PeerData &pd)
{
    QByteArray data = QByteArray(pd.data);

    if (data.isEmpty())
    {
        m_server->abort(pd.descriptor);
        return;
    }

    quint8 command = getValue<quint8>(data.mid(0, 1));
    data.remove(0, 1);

    switch (command)
    {
    case Command::AudioHeader:
    {
        //Server should not receive header
        m_server->abort(pd.descriptor);
        return;
    }
    case Command::AudioData:
    {
        //Audio data comes to the server,
        //if necesary decode it and send it to the callback signal

        LIB_DEBUG_LEVEL_2("Got Audio Data - Size:" << data.size() << "bytes.");

        preProcessOutput(data);

        QByteArray result;
        result.append(getBytes<quint8>(Command::HeartBeat));

        if (m_server)
            m_server->writeToHost(result, pd.descriptor);

        break;
    }
    case Command::HeartBeat:
    {
        if (!data.isEmpty())
        {
            m_server->abort(pd.descriptor);
            return;
        }

        qint32 &beats = m_hash_heart_beat[pd.descriptor];
        QElapsedTimer &time = m_hash_heart_beat_time[pd.descriptor];

        if (beats > 0)
            beats--;
        else
            time.restart();

        break;
    }
    case Command::ExtraData:
    {
        //Extra data received
        emit extraData(data);

        break;
    }
    case Command::ExtraDataReceived:
    {
        if (!data.isEmpty())
        {
            m_server->abort(pd.descriptor);
            return;
        }

        m_extra_data_peers--;

        m_extra_data_peers = qMax(m_extra_data_peers, 0);

        if (m_extra_data_peers == 0)
        {
            m_ready_to_write_extra_data = true;
            emit extraDataWritten();
        }

        break;
    }
    default:
    {
        m_server->abort(pd.descriptor);
        return;
    }
    }
}

void AudioStreamingWorker::processClientInput(const PeerData &pd)
{
    QByteArray data = QByteArray(pd.data);

    if (data.isEmpty())
    {
        m_client->abort();
        return;
    }

    quint8 command = getValue<quint8>(data.mid(0, 1));
    data.remove(0, 1);

    switch (command)
    {
    case Command::AudioHeader:
    {
        //Header got by the server

        if (data.size() != 56)
        {
            m_client->abort();
            return;
        }

        QAudioFormat inputFormat;
        QAudioFormat audioFormat;
        qint32 timeToBuffer;

        header(data, &inputFormat, &audioFormat, &timeToBuffer);

        m_streaming_info.setInputAudioFormat(inputFormat);
        m_streaming_info.setAudioFormat(audioFormat);

        if (m_is_walkie_talkie)
            m_streaming_info.setTimeToBuffer(timeToBuffer);

        if (INVALID_FORMAT)
        {
            errorPrivate("Invalid format!");
            return;
        }

        adjustSettingsPrivate(true);

        startAudioWorkers();

        restartActiveWorkers();

        break;
    }
    case Command::AudioData:
    {
        LIB_DEBUG_LEVEL_2("Got Audio Data - Size:" << data.size() << "bytes.");

        preProcessOutput(data);

        QByteArray result;
        result.append(getBytes<quint8>(Command::HeartBeat));

        if (m_client)
            m_client->write(result);

        break;
    }
    case Command::HeartBeat:
    {
        if (!data.isEmpty())
        {
            m_client->abort();
            return;
        }

        qint32 &beats = m_hash_heart_beat[pd.descriptor];
        QElapsedTimer &time = m_hash_heart_beat_time[pd.descriptor];

        if (beats > 0)
            beats--;

        if (beats <= MAX_PENDING_BEATS)
            time.restart();

        break;
    }
    case Command::ExtraData:
    {
        emit extraData(data);

        break;
    }
    case Command::ExtraDataReceived:
    {
        if (!data.isEmpty())
        {
            m_client->abort();
            return;
        }

        m_extra_data_peers--;

        m_extra_data_peers = qMax(m_extra_data_peers, 0);

        if (m_extra_data_peers == 0)
        {
            m_ready_to_write_extra_data = true;
            emit extraDataWritten();
        }

        break;
    }
    default:
    {
        m_client->abort();
        return;
    }
    }
}

void AudioStreamingWorker::processWebClientInput(const PeerData &pd)
{
    QByteArray data = QByteArray(pd.data);

    if (data.isEmpty())
    {
        m_client->abort();
        return;
    }

    quint8 command = getValue<quint8>(data.mid(0, 1));
    data.remove(0, 1);

    switch (command)
    {
    case Command::AudioHeader:
    {
        //Header got by the server

        if (data.size() != 56)
        {
            m_client->abort();
            return;
        }

        QAudioFormat inputFormat;
        QAudioFormat audioFormat;
        qint32 timeToBuffer;

        header(data, &inputFormat, &audioFormat, &timeToBuffer);

        if (INVALID_FORMAT)
        {
            errorPrivate("Invalid format!");
            return;
        }

        if (m_streaming_info.inputAudioFormat() != inputFormat ||
                m_streaming_info.audioFormat() != audioFormat ||
                m_streaming_info.timeToBuffer() != timeToBuffer)
        {
            errorPrivate("Incompatible audio formats!");
            return;
        }

        startAudioWorkers();

        restartActiveWorkers();

        break;
    }
    case Command::AudioData:
    {
        LIB_DEBUG_LEVEL_2("Got Audio Data - Size:" << data.size() << "bytes.");

        preProcessOutput(data);

        QByteArray result;
        result.append(getBytes<quint8>(Command::HeartBeat));

        if (m_client)
            m_client->write(result);

        break;
    }
    case Command::HeartBeat:
    {
        if (!data.isEmpty())
        {
            m_server->abort(pd.descriptor);
            return;
        }

        qint32 &beats = m_hash_heart_beat[pd.descriptor];
        QElapsedTimer &time = m_hash_heart_beat_time[pd.descriptor];

        if (beats > 0)
            beats--;

        if (beats <= MAX_PENDING_BEATS)
            time.restart();

        break;
    }
    case Command::ExtraData:
    {
        emit extraData(data);

        break;
    }
    case Command::ExtraDataReceived:
    {
        if (!data.isEmpty())
        {
            m_client->abort();
            return;
        }

        m_extra_data_peers--;

        m_extra_data_peers = qMax(m_extra_data_peers, 0);

        if (m_extra_data_peers == 0)
        {
            m_ready_to_write_extra_data = true;
            emit extraDataWritten();
        }

        break;
    }
    default:
    {
        m_client->abort();
        return;
    }
    }
}

QByteArray AudioStreamingWorker::createHeader()
{
    QByteArray data;

    QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();
    QAudioFormat format = m_streaming_info.audioFormat();

    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(format.sampleSize());
    stream << qint32(format.sampleRate());
    stream << qint32(format.channelCount());
    stream << qint32(format.sampleType());
    stream << qint32(format.byteOrder());

    stream << qint32(inputFormat.sampleSize());
    stream << qint32(inputFormat.sampleRate());
    stream << qint32(inputFormat.channelCount());
    stream << qint32(inputFormat.sampleType());
    stream << qint32(inputFormat.byteOrder());

    if (m_is_walkie_talkie)
        stream << qint32(m_streaming_info.timeToBuffer());
    else
        stream << qint32(0);

#ifdef OPUS
    stream << m_frame_size;
    stream << m_max_frame_size;
    stream << m_bitrate;
#else
    stream << qint32(0);
    stream << qint32(0);
    stream << qint32(0);
#endif

    return data;
}

void AudioStreamingWorker::header(QByteArray data,
                    QAudioFormat *refInputAudioFormat,
                    QAudioFormat *refAudioFormat,
                    qint32 *refTimeToBuffer)
{
    qint32 sampleSize;
    qint32 sampleRate;
    qint32 channelCount;
    qint32 sampleType;
    qint32 byteOrder;

    qint32 inputSampleSize;
    qint32 inputSampleRate;
    qint32 inputChannelCount;
    qint32 inputSampleType;
    qint32 inputByteOrder;

    qint32 timeToBuffer;

#ifdef OPUS
    qint32 frame_size;
    qint32 max_frame_size;
    qint32 bitrate;
#endif

    QDataStream stream(&data, QIODevice::ReadOnly);

    stream >> sampleSize;
    stream >> sampleRate;
    stream >> channelCount;
    stream >> sampleType;
    stream >> byteOrder;

    stream >> inputSampleSize;
    stream >> inputSampleRate;
    stream >> inputChannelCount;
    stream >> inputSampleType;
    stream >> inputByteOrder;

    stream >> timeToBuffer;

#ifdef OPUS
    stream >> frame_size;
    stream >> max_frame_size;
    stream >> bitrate;

    if (m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient
            || m_streaming_info.workMode() == AudioStreamingLibInfo::StreamingWorkMode::WalkieTalkieClient)
    {
        m_frame_size = frame_size;
        m_max_frame_size = max_frame_size;
        m_bitrate = bitrate;
    }

    m_streaming_info.setOpusBitrate(m_bitrate);
#endif

    if (refTimeToBuffer)
        *refTimeToBuffer = timeToBuffer;

    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setSampleSize(sampleSize);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
    format.setSampleType(QAudioFormat::SampleType(sampleType));
    format.setByteOrder(QAudioFormat::Endian(byteOrder));

    QAudioFormat inputFormat;
    inputFormat.setCodec("audio/pcm");
    inputFormat.setSampleSize(inputSampleSize);
    inputFormat.setSampleRate(inputSampleRate);
    inputFormat.setChannelCount(inputChannelCount);
    inputFormat.setSampleType(QAudioFormat::SampleType(inputSampleType));
    inputFormat.setByteOrder(QAudioFormat::Endian(inputByteOrder));

    *refInputAudioFormat = inputFormat;
    *refAudioFormat = format;
}
