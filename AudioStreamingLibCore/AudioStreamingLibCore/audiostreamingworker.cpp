#include "audiostreamingworker.h"

#if defined (IS_TO_DEBUG_VERBOSE_1) || defined (IS_TO_DEBUG_VERBOSE_2)
QMutex record_mutex;
qint64 record_count = 0;
#endif

#define INVALID_FORMAT\
    (m_streaming_info.inputAudioFormat().isValid() &&\
    (m_streaming_info.inputAudioFormat().sampleType() != QAudioFormat::Float ||\
    m_streaming_info.inputAudioFormat().sampleSize() != 32 ||\
    m_streaming_info.inputAudioFormat().byteOrder() != QAudioFormat::LittleEndian)) ||\
    (m_streaming_info.audioFormat().isValid() &&\
    (m_streaming_info.audioFormat().sampleType() != QAudioFormat::Float ||\
    m_streaming_info.audioFormat().sampleSize() != 32 ||\
    m_streaming_info.audioFormat().byteOrder() != QAudioFormat::LittleEndian))

AudioStreamingWorker::AudioStreamingWorker(QObject *parent) : QObject(parent)
{
    m_server_discover = nullptr;
    m_server = nullptr;
    m_client = nullptr;
    m_audio_input = nullptr;
    m_audio_output = nullptr;
    m_flow_control = nullptr;
    m_level_meter_input = nullptr;
    m_level_meter_output = nullptr;
#ifdef OPUS
    m_resampler = nullptr;
    m_opus_enc = nullptr;
    m_opus_dec = nullptr;
#endif
    m_is_walkie_talkie = false;
    m_has_error = false;
    m_input_muted = false;
    m_volume = 0;
    m_ready_to_write_extra_data = false;
    m_extra_data_peers = 0;
    m_callback_enabled = false;
#ifdef OPUS
    m_frame_size = 0;
    m_max_frame_size = 0;
    m_bitrate = 0;
#endif
}

AudioStreamingWorker::~AudioStreamingWorker()
{

}

void AudioStreamingWorker::start(const StreamingInfo &streaming_info)
{
    m_streaming_info = streaming_info;

    if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::Undefined)
    {
        errorPrivate("Mode of work not defined!");
        return;
    }

    if (INVALID_FORMAT)
    {
        errorPrivate("Sample type and/or endiannes not supported!");
        return;
    }

    switch (m_streaming_info.workMode())
    {
    case StreamingInfo::StreamingWorkMode::BroadcastClient:
    {
        if (m_streaming_info.isEncryptionEnabled())
            m_client = new EncryptedClient(this);
        else
            m_client = new Client(this);

        connect(m_client, &AbstractClient::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_client, &AbstractClient::connected, this, &AudioStreamingWorker::clientConencted);
        connect(m_client, &AbstractClient::disconnected, this, &AudioStreamingWorker::clientDisconencted);
        connect(m_client, &AbstractClient::readyRead, this, &AudioStreamingWorker::processClientInput);

        startAudioWorkers();

        break;
    }
    case StreamingInfo::StreamingWorkMode::BroadcastServer:
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

        startAudioWorkers();

        break;
    }
    case StreamingInfo::StreamingWorkMode::WalkieTalkieServer:
    case StreamingInfo::StreamingWorkMode::WalkieTalkieClient:
    {
        if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieClient)
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
        else if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
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
        }

        startAudioWorkers();

        break;
    }
    case StreamingInfo::StreamingWorkMode::WebClient:
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

        connect(m_client, &AbstractClient::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_client, &AbstractClient::connectedToServer, this, &AudioStreamingWorker::webClientConencted);
        connect(m_client, &AbstractClient::connectedToPeer, this, &AudioStreamingWorker::webClientConnectedToPeer);
        connect(m_client, &AbstractClient::disconnectedFromPeer, this, &AudioStreamingWorker::webClientDisconnected);
        connect(m_client, &AbstractClient::disconnected, this, &AudioStreamingWorker::webClientDisconnected);
        connect(m_client, &AbstractClient::pending, this, &AudioStreamingWorker::pending);
        connect(m_client, &AbstractClient::webClientLoggedIn, this, &AudioStreamingWorker::webClientLoggedIn);
        connect(m_client, &AbstractClient::webClientWarning, this, &AudioStreamingWorker::webClientWarning);
        connect(m_client, &AbstractClient::readyRead, this, &AudioStreamingWorker::processWebClientInput);

        startAudioWorkers();

        break;
    }
    default:
        break;
    }
}

void AudioStreamingWorker::startAudioWorkers()
{
    switch (m_streaming_info.workMode())
    {
    case StreamingInfo::StreamingWorkMode::BroadcastClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();
#ifdef OPUS
        m_opus_dec = new OpusDecoderClass(this);
        SETTONULLPTR(m_opus_dec);
        connect(m_opus_dec, &OpusDecoderClass::decoded, this, &AudioStreamingWorker::posProcessedOutput);
        connect(m_opus_dec, &OpusDecoderClass::error, this, &AudioStreamingWorker::errorPrivate);
#endif
        if (m_streaming_info.outputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput(this);
            SETTONULLPTR(m_audio_output);
            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryOutputData);

            connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::errorPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_output);
            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &AudioStreamingWorker::outputLevel);
        }

        break;
    }
    case StreamingInfo::StreamingWorkMode::BroadcastServer:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();
#ifdef OPUS
        m_resampler = new r8brain(this);
        m_opus_enc = new OpusEncoderClass(this);
        SETTONULLPTR(m_resampler);
        SETTONULLPTR(m_opus_enc);
        connect(m_resampler, &r8brain::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

        if (m_streaming_info.isGetAudioEnabled())
            connect(m_resampler, &r8brain::resampled, this, &AudioStreamingWorker::veryInputData);

        connect(m_opus_enc, &OpusEncoderClass::encoded, this, &AudioStreamingWorker::posProcessedInput);

        connect(m_opus_enc, &OpusEncoderClass::error, this, &AudioStreamingWorker::errorPrivate);
#endif
        if (m_streaming_info.inputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput(this);
            SETTONULLPTR(m_audio_input);

            connect(m_audio_input, &AudioInput::error, this, &AudioStreamingWorker::errorPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputDataBack);

            m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());
        }
        else
        {
            QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

            m_flow_control = new FlowControl(this);
            SETTONULLPTR(m_flow_control);
            connect(m_flow_control, &FlowControl::getbytes, this, &AudioStreamingWorker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &AudioStreamingWorker::errorPrivate);
            m_flow_control->start(inputFormat.sampleRate(), inputFormat.channelCount(), inputFormat.sampleSize());
        }

        adjustSettingsPrivate(true, false, false);

        m_level_meter_input = new LevelMeter(this);
        SETTONULLPTR(m_level_meter_input);
        connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);
        m_level_meter_input->start(m_streaming_info.inputAudioFormat());

        break;
    }
    case StreamingInfo::StreamingWorkMode::WalkieTalkieServer:
    case StreamingInfo::StreamingWorkMode::WalkieTalkieClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

        m_is_walkie_talkie = true;
#ifdef OPUS
        m_resampler = new r8brain(this);
        m_opus_enc = new OpusEncoderClass(this);
        m_opus_dec = new OpusDecoderClass(this);
        SETTONULLPTR(m_resampler);
        SETTONULLPTR(m_opus_enc);
        SETTONULLPTR(m_opus_dec);
        connect(m_opus_enc, &OpusEncoderClass::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_opus_dec, &OpusDecoderClass::error, this, &AudioStreamingWorker::errorPrivate);

        connect(m_resampler, &r8brain::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

        if (m_streaming_info.isGetAudioEnabled())
            connect(m_resampler, &r8brain::resampled, this, &AudioStreamingWorker::veryInputData);

        connect(m_opus_enc, &OpusEncoderClass::encoded, this, &AudioStreamingWorker::posProcessedInput);

        connect(m_opus_dec, &OpusDecoderClass::decoded, this, &AudioStreamingWorker::posProcessedOutput);
#endif
        if (m_streaming_info.inputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput(this);
            SETTONULLPTR(m_audio_input);

            connect(m_audio_input, &AudioInput::error, this, &AudioStreamingWorker::errorPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputDataBack);

            m_level_meter_input = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_input);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                adjustSettingsPrivate(true, true, false);

                m_level_meter_input->start(m_streaming_info.inputAudioFormat());

                m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());
            }
        }
        else //used to compute level if not using the library input device
        {
            m_flow_control = new FlowControl(this);
            SETTONULLPTR(m_flow_control);
            connect(m_flow_control, &FlowControl::getbytes, this, &AudioStreamingWorker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &AudioStreamingWorker::errorPrivate);

            m_level_meter_input = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_input);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

                adjustSettingsPrivate(true, true, false);

                m_level_meter_input->start(m_streaming_info.inputAudioFormat());

                m_flow_control->start(inputFormat.sampleRate(),
                                      inputFormat.channelCount(),
                                      inputFormat.sampleSize());
            }
        }

        if (m_streaming_info.outputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput(this);
            SETTONULLPTR(m_audio_output);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryOutputData);

            connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::errorPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                QAudioFormat format = m_streaming_info.audioFormat();

                m_audio_output->start(m_streaming_info.outputDeviceInfo(),
                                      format, m_streaming_info.timeToBuffer(),
                                      m_streaming_info.isGetAudioEnabled());
            }

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_output);
            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &AudioStreamingWorker::outputLevel);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                QAudioFormat format = m_streaming_info.audioFormat();

                m_level_meter_output->start(format);
            }
        }

        break;
    }
    case StreamingInfo::StreamingWorkMode::WebClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

        m_is_walkie_talkie = true;
#ifdef OPUS
        m_resampler = new r8brain(this);
        m_opus_enc = new OpusEncoderClass(this);
        m_opus_dec = new OpusDecoderClass(this);
        SETTONULLPTR(m_resampler);
        SETTONULLPTR(m_opus_enc);
        SETTONULLPTR(m_opus_dec);
        connect(m_opus_enc, &OpusEncoderClass::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_opus_dec, &OpusDecoderClass::error, this, &AudioStreamingWorker::errorPrivate);

        connect(m_resampler, &r8brain::error, this, &AudioStreamingWorker::errorPrivate);
        connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

        if (m_streaming_info.isGetAudioEnabled())
            connect(m_resampler, &r8brain::resampled, this, &AudioStreamingWorker::veryInputData);

        connect(m_opus_enc, &OpusEncoderClass::encoded, this, &AudioStreamingWorker::posProcessedInput);

        connect(m_opus_dec, &OpusDecoderClass::decoded, this, &AudioStreamingWorker::posProcessedOutput);
#endif
        if (m_streaming_info.inputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput(this);
            SETTONULLPTR(m_audio_input);

            connect(m_audio_input, &AudioInput::error, this, &AudioStreamingWorker::errorPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &AudioStreamingWorker::inputDataBack);

            adjustSettingsPrivate(true, true, false);

            m_level_meter_input = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_input);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);
        }
        else //used to compute level if not using the library input device
        {
            m_flow_control = new FlowControl(this);
            SETTONULLPTR(m_flow_control);
            connect(m_flow_control, &FlowControl::getbytes, this, &AudioStreamingWorker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &AudioStreamingWorker::errorPrivate);

            QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

            adjustSettingsPrivate(true, true, false);

            m_level_meter_input = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_input);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &AudioStreamingWorker::inputLevel);

            m_flow_control->start(inputFormat.sampleRate(),
                                  inputFormat.channelCount(),
                                  inputFormat.sampleSize());
        }

        if (m_streaming_info.outputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput(this);
            SETTONULLPTR(m_audio_output);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &AudioStreamingWorker::veryOutputData);

            connect(m_audio_output, &AudioOutput::error, this, &AudioStreamingWorker::errorPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &AudioStreamingWorker::outputLevel);

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter(this);
            SETTONULLPTR(m_level_meter_output);
            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &AudioStreamingWorker::outputLevel);
        }

        break;
    }
    default:
        break;
    }
}

void AudioStreamingWorker::stopAudioWorkers()
{
    if (m_audio_input)
        m_audio_input->deleteLater();
    if (m_audio_output)
        m_audio_output->deleteLater();
    if (m_flow_control)
        m_flow_control->deleteLater();
#ifdef OPUS
    if (m_resampler)
        m_resampler->deleteLater();
    if (m_opus_enc)
        m_opus_enc->deleteLater();
    if (m_opus_dec)
        m_opus_dec->deleteLater();
#endif
    if (m_level_meter_input)
        m_level_meter_input->deleteLater();
    if (m_level_meter_output)
        m_level_meter_output->deleteLater();
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

void AudioStreamingWorker::connectToHost(const QString &host, quint16 port, const QByteArray &password, bool new_user)
{
    if (m_client)
    {
        m_client->connectToHost(host, port, m_streaming_info.negotiationString(), m_streaming_info.ID(), password, new_user);
    }
    else
    {
        errorPrivate("Not started in client mode!");
        return;
    }
}

void AudioStreamingWorker::connectToPeer(const QString &ID)
{
    if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WebClient)
    {
        m_client->connectToPeer(ID);
    }
}

void AudioStreamingWorker::disconnectFromPeer()
{
    if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WebClient)
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
    case StreamingInfo::StreamingWorkMode::BroadcastServer:
        application = OPUS_APPLICATION_AUDIO;
        break;
    case StreamingInfo::StreamingWorkMode::WalkieTalkieClient:
    case StreamingInfo::StreamingWorkMode::WalkieTalkieServer:
    case StreamingInfo::StreamingWorkMode::WebClient:
        application = OPUS_APPLICATION_VOIP;
        break;
    default:
        break;
    }

    QAudioFormat inputAudioFormat = m_streaming_info.inputAudioFormat();
    QAudioFormat audioFormat = m_streaming_info.audioFormat();

    m_resampler->start(inputAudioFormat.sampleRate(),
                       audioFormat.sampleRate(),
                       inputAudioFormat.channelCount(),
                       inputAudioFormat.sampleSize());

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

    m_opus_dec->start(format.sampleRate(),
                      format.channelCount(),
                      m_frame_size,
                      m_max_frame_size);
#endif
}

void AudioStreamingWorker::adjustSettingsPrivate(bool start_opus_encoder, bool start_opus_decoder, bool client_mode)
{
    QAudioFormat format = m_streaming_info.inputAudioFormat();
#ifdef OPUS //Change the settings to Opus compatible settings

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

    if (start_opus_encoder)
        startOpusEncoder();

    if (start_opus_decoder)
        startOpusDecoder();
#else
    Q_UNUSED(start_opus_encoder)
    Q_UNUSED(start_opus_decoder)
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

//The app is currently connected to the server
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

//The app currently disconnected from the server
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

    if (m_level_meter_input)
        m_level_meter_input->write(middle);

#ifdef OPUS
    m_resampler->write(middle);
#else
    if (m_streaming_info.isGetAudioEnabled())
        emit veryInputData(middle);
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
        foreach (qintptr descriptor, m_id_connections_list)
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
        foreach (qintptr descriptor, m_id_connections_list)
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
void AudioStreamingWorker::flowControl(int bytes)
{
    QByteArray data = QByteArray(bytes, char(0));

    if (!data.isEmpty())
    {
        Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>(reinterpret_cast<float*>(data.data()), data.size() / int(sizeof(float)));
        samples.fill(0);
    }

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

StreamingInfo AudioStreamingWorker::streamingInfo() const
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
            errorPrivate("Sample type and/or endiannes not supported!");
            return;
        }

        startAudioWorkers();

        adjustSettingsPrivate(m_is_walkie_talkie, true, true);

        if (m_is_walkie_talkie)
        {
            m_level_meter_input->start(m_streaming_info.inputAudioFormat());

            if (m_flow_control)
                m_flow_control->start(inputFormat.sampleRate(), inputFormat.channelCount(), inputFormat.sampleSize());
            else
                m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());
        }

        m_audio_output->start(m_streaming_info.outputDeviceInfo(),
                              m_streaming_info.audioFormat(),
                              m_streaming_info.timeToBuffer(),
                              m_streaming_info.isGetAudioEnabled());

        if (m_level_meter_output)
            m_level_meter_output->start(m_streaming_info.audioFormat());

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
            errorPrivate("Sample type and/or endiannes not supported!");
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

        adjustSettingsPrivate(true, true, true);

        m_level_meter_input->start(m_streaming_info.inputAudioFormat());

        if (m_flow_control)
            m_flow_control->start(inputFormat.sampleRate(), inputFormat.channelCount(), inputFormat.sampleSize());
        else
            m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());

        m_audio_output->start(m_streaming_info.outputDeviceInfo(),
                              m_streaming_info.audioFormat(),
                              m_streaming_info.timeToBuffer(),
                              m_streaming_info.isGetAudioEnabled());

        if (m_level_meter_output)
            m_level_meter_output->start(m_streaming_info.audioFormat());

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
    stream >> m_frame_size;
    stream >> m_max_frame_size;
    stream >> m_bitrate;

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
