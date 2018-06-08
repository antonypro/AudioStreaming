#include "worker.h"

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

Worker::Worker(QObject *parent) : QObject(parent)
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

Worker::~Worker()
{

}

void Worker::start(const StreamingInfo &streaming_info)
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
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

#ifdef OPUS
        m_opus_dec = new OpusDecoderClass();
        {
            connect(this, &Worker::destroyed, m_opus_dec, &OpusDecoderClass::deleteLater);

            connect(m_opus_dec, &OpusDecoderClass::decoded, this, &Worker::posProcessedOutput);
        }
        connect(m_opus_dec, &OpusDecoderClass::error, this, &Worker::errorPrivate);
#endif
        if (m_streaming_info.isEncryptionEnabled())
            m_client = new EncryptedClient(this);
        else
            m_client = new Client(this);

        connect(m_client, &AbstractClient::error, this, &Worker::errorPrivate);
        connect(m_client, &AbstractClient::connected, this, &Worker::clientConencted);
        connect(m_client, &AbstractClient::disconnected, this, &Worker::clientDisconencted);
        connect(m_client, &AbstractClient::readyRead, this, &Worker::processClientInput);

        if (m_streaming_info.outputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput();
            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &Worker::veryOutputData);

            connect(this, &Worker::destroyed, m_audio_output, &AudioOutput::deleteLater);

            connect(m_audio_output, &AudioOutput::error, this, &Worker::errorPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &Worker::outputLevel);

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_output, &LevelMeter::deleteLater);
            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &Worker::outputLevel);
        }

        break;
    }
    case StreamingInfo::StreamingWorkMode::BroadcastServer:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

#ifdef OPUS
        m_resampler = new r8brain();
        m_opus_enc = new OpusEncoderClass();
        {
            connect(this, &Worker::destroyed, m_resampler, &r8brain::deleteLater);

            connect(m_resampler, &r8brain::error, this, &Worker::errorPrivate);
            connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_resampler, &r8brain::resampled, this, &Worker::veryInputData);
        }
        {
            connect(this, &Worker::destroyed, m_opus_enc, &OpusEncoderClass::deleteLater);

            connect(m_opus_enc, &OpusEncoderClass::encoded, this, &Worker::posProcessedInput);
        }
        connect(m_opus_enc, &OpusEncoderClass::error, this, &Worker::errorPrivate);
#endif
        m_server_discover = new DiscoverServer(this);

        if (m_streaming_info.isEncryptionEnabled())
            m_server = new EncryptedServer(this);
        else
            m_server = new Server(this);

        connect(m_server, &AbstractServer::error, this, &Worker::errorPrivate);
        connect(m_server, &AbstractServer::connected, this, &Worker::serverClientConencted);
        connect(m_server, &AbstractServer::disconnected, this, &Worker::serverClientDisconencted);
        connect(m_server, &AbstractServer::pending, this, &Worker::pending);
        connect(m_server, &AbstractServer::readyRead, this, &Worker::processServerInput);

        if (m_streaming_info.inputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput();
            connect(this, &Worker::destroyed, m_audio_input, &AudioInput::deleteLater);

            connect(m_audio_input, &AudioInput::error, this, &Worker::errorPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &Worker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &Worker::inputDataBack);

            m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());
        }
        else
        {
            QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

            m_flow_control = new FlowControl(this);
            connect(m_flow_control, &FlowControl::getbytes, this, &Worker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &Worker::errorPrivate);
            m_flow_control->start(inputFormat.sampleRate(), inputFormat.channelCount(), inputFormat.sampleSize());
        }

        adjustSettingsPrivate(true, false, false);

        m_level_meter_input = new LevelMeter();
        connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
        connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
        m_level_meter_input->start(m_streaming_info.inputAudioFormat());

        break;
    }
    case StreamingInfo::StreamingWorkMode::WalkieTalkieServer:
    case StreamingInfo::StreamingWorkMode::WalkieTalkieClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

        m_is_walkie_talkie = true;
#ifdef OPUS
        m_resampler = new r8brain();
        m_opus_enc = new OpusEncoderClass();
        m_opus_dec = new OpusDecoderClass();
        connect(m_opus_enc, &OpusEncoderClass::error, this, &Worker::errorPrivate);
        connect(m_opus_dec, &OpusDecoderClass::error, this, &Worker::errorPrivate);
        {
            connect(this, &Worker::destroyed, m_resampler, &r8brain::deleteLater);

            connect(m_resampler, &r8brain::error, this, &Worker::errorPrivate);
            connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_resampler, &r8brain::resampled, this, &Worker::veryInputData);
        }
        {
            connect(this, &Worker::destroyed, m_opus_enc, &OpusEncoderClass::deleteLater);

            connect(m_opus_enc, &OpusEncoderClass::encoded, this, &Worker::posProcessedInput);
        }
        {
            connect(this, &Worker::destroyed, m_opus_dec, &OpusDecoderClass::deleteLater);

            connect(m_opus_dec, &OpusDecoderClass::decoded, this, &Worker::posProcessedOutput);
        }
#endif
        if (m_streaming_info.inputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput();

            connect(this, &Worker::destroyed, m_audio_input, &AudioInput::deleteLater);

            connect(m_audio_input, &AudioInput::error, this, &Worker::errorPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &Worker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &Worker::inputDataBack);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                adjustSettingsPrivate(true, true, false);

                m_level_meter_input = new LevelMeter();
                connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
                connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
                m_level_meter_input->start(m_streaming_info.inputAudioFormat());

                m_audio_input->start(m_streaming_info.inputDeviceInfo(), m_streaming_info.inputAudioFormat());
            }
        }
        else //used to compute level if not using the library input device
        {
            m_flow_control = new FlowControl(this);
            connect(m_flow_control, &FlowControl::getbytes, this, &Worker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &Worker::errorPrivate);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

                adjustSettingsPrivate(true, true, false);

                m_level_meter_input = new LevelMeter();
                connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
                connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
                m_level_meter_input->start(m_streaming_info.inputAudioFormat());

                m_flow_control->start(inputFormat.sampleRate(),
                                      inputFormat.channelCount(),
                                      inputFormat.sampleSize());
            }
        }

        if (m_streaming_info.outputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput();

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &Worker::veryOutputData);

            connect(this, &Worker::destroyed, m_audio_output, &AudioOutput::deleteLater);

            connect(m_audio_output, &AudioOutput::error, this, &Worker::errorPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &Worker::outputLevel);

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
            m_level_meter_output = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_output, &LevelMeter::deleteLater);
            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &Worker::outputLevel);

            if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
            {
                QAudioFormat format = m_streaming_info.audioFormat();

                m_level_meter_output->start(format);
            }
        }

        if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieClient)
        {
            if (m_streaming_info.isEncryptionEnabled())
                m_client = new EncryptedClient(this);
            else
                m_client = new Client(this);

            connect(m_client, &AbstractClient::error, this, &Worker::errorPrivate);
            connect(m_client, &AbstractClient::connected, this, &Worker::clientConencted);
            connect(m_client, &AbstractClient::disconnected, this, &Worker::clientDisconencted);
            connect(m_client, &AbstractClient::readyRead, this, &Worker::processClientInput);
        }
        else if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WalkieTalkieServer)
        {
            m_server_discover = new DiscoverServer(this);

            if (m_streaming_info.isEncryptionEnabled())
                m_server = new EncryptedServer(this);
            else
                m_server = new Server(this);

            connect(m_server, &AbstractServer::error, this, &Worker::errorPrivate);
            connect(m_server, &AbstractServer::connected, this, &Worker::serverClientConencted);
            connect(m_server, &AbstractServer::disconnected, this, &Worker::serverClientDisconencted);
            connect(m_server, &AbstractServer::pending, this, &Worker::pending);
            connect(m_server, &AbstractServer::readyRead, this, &Worker::processServerInput);
        }

        break;
    }
    case StreamingInfo::StreamingWorkMode::WebClient:
    {
        m_callback_enabled = m_streaming_info.isCallBackEnabled();

        m_is_walkie_talkie = true;
#ifdef OPUS
        m_resampler = new r8brain();
        m_opus_enc = new OpusEncoderClass();
        m_opus_dec = new OpusDecoderClass();
        connect(m_opus_enc, &OpusEncoderClass::error, this, &Worker::errorPrivate);
        connect(m_opus_dec, &OpusDecoderClass::error, this, &Worker::errorPrivate);
        {
            connect(this, &Worker::destroyed, m_resampler, &r8brain::deleteLater);

            connect(m_resampler, &r8brain::error, this, &Worker::errorPrivate);
            connect(m_resampler, &r8brain::resampled, m_opus_enc, &OpusEncoderClass::write);

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_resampler, &r8brain::resampled, this, &Worker::veryInputData);
        }
        {
            connect(this, &Worker::destroyed, m_opus_enc, &OpusEncoderClass::deleteLater);

            connect(m_opus_enc, &OpusEncoderClass::encoded, this, &Worker::posProcessedInput);
        }
        {
            connect(this, &Worker::destroyed, m_opus_dec, &OpusDecoderClass::deleteLater);

            connect(m_opus_dec, &OpusDecoderClass::decoded, this, &Worker::posProcessedOutput);
        }
#endif
        if (m_streaming_info.inputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_input = new AudioInput();

            connect(this, &Worker::destroyed, m_audio_input, &AudioInput::deleteLater);

            connect(m_audio_input, &AudioInput::error, this, &Worker::errorPrivate);

            if (m_callback_enabled)
                connect(m_audio_input, &AudioInput::readyRead, this, &Worker::inputData);
            else
                connect(m_audio_input, &AudioInput::readyRead, this, &Worker::inputDataBack);

            adjustSettingsPrivate(true, true, false);

            m_level_meter_input = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
            m_level_meter_input->start(m_streaming_info.inputAudioFormat());
        }
        else //used to compute level if not using the library input device
        {
            m_flow_control = new FlowControl(this);
            connect(m_flow_control, &FlowControl::getbytes, this, &Worker::flowControl);
            connect(m_flow_control, &FlowControl::error, this, &Worker::errorPrivate);

            QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();

            adjustSettingsPrivate(true, true, false);

            m_level_meter_input = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
            m_level_meter_input->start(m_streaming_info.inputAudioFormat());

            m_flow_control->start(inputFormat.sampleRate(),
                                  inputFormat.channelCount(),
                                  inputFormat.sampleSize());
        }

        if (m_streaming_info.outputDeviceType() == StreamingInfo::AudioDeviceType::LibraryAudioDevice)
        {
            m_audio_output = new AudioOutput();

            if (m_streaming_info.isGetAudioEnabled())
                connect(m_audio_output, &AudioOutput::veryOutputData, this, &Worker::veryOutputData);

            connect(this, &Worker::destroyed, m_audio_output, &AudioOutput::deleteLater);

            connect(m_audio_output, &AudioOutput::error, this, &Worker::errorPrivate);
            connect(m_audio_output, &AudioOutput::currentlevel, this, &Worker::outputLevel);

            QAudioFormat format = m_streaming_info.audioFormat();

            m_audio_output->setVolume(m_volume);
        }
        else //used to compute level if not using the library output device
        {
            m_level_meter_output = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_output, &LevelMeter::deleteLater);
            connect(m_level_meter_output, &LevelMeter::currentlevel, this, &Worker::outputLevel);

            QAudioFormat format = m_streaming_info.audioFormat();

            m_level_meter_output->start(format);
        }

        if (m_streaming_info.isEncryptionEnabled())
        {
            m_client = new WebClient(this);
        }
        else
        {
            errorPrivate("WebClient requires encryption enabled!");
            return;
        }

        connect(m_client, &AbstractClient::error, this, &Worker::errorPrivate);
        connect(m_client, &AbstractClient::connectedToServer, this, &Worker::webClientConencted);
        connect(m_client, &AbstractClient::connectedToPeer, this, &Worker::webClientConnectedToPeer);
        connect(m_client, &AbstractClient::disconnected, this, &Worker::webClientDisconencted);
        connect(m_client, &AbstractClient::pending, this, &Worker::pending);
        connect(m_client, &AbstractClient::readyRead, this, &Worker::processWebClientInput);

        break;
    }
    default:
        break;
    }
}

void Worker::listen(quint16 port, bool auto_accept, const QByteArray &password, int max_connections)
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

void Worker::connectToHost(const QString &host, quint16 port, const QByteArray &password)
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

void Worker::connectToPeer(const QString &ID)
{
    if (m_streaming_info.workMode() == StreamingInfo::StreamingWorkMode::WebClient)
    {
        m_client->connectToPeer(ID);
    }
}

void Worker::acceptSslCertificate()
{
    if (m_client)
        m_client->acceptSslCertificate();
}

void Worker::acceptConnection()
{
    if (m_server)
        m_server->acceptNewConnection();
    else if (m_client)
        m_client->acceptConnection();
}

void Worker::rejectConnection()
{
    if (m_server)
        m_server->rejectNewConnection();
    else if (m_client)
        m_client->rejectConnection();
}

void Worker::errorPrivate(const QString &error_description)
{
    if (m_has_error)
        return;

    m_has_error = true;

    emit error(error_description);
}

void Worker::startOpusEncoder()
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

void Worker::startOpusDecoder()
{
#ifdef OPUS
    QAudioFormat format = m_streaming_info.audioFormat();

    m_opus_dec->start(format.sampleRate(),
                      format.channelCount(),
                      m_frame_size,
                      m_max_frame_size);
#endif
}

void Worker::adjustSettingsPrivate(bool start_opus_encoder, bool start_opus_decoder, bool client_mode)
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
void Worker::serverClientConencted(const PeerData &pd, const QString &id)
{
    //Send the audio format to the peer
    QByteArray data;
    data.append(getBytes<quint8>(Command::AudioHeader));
    data.append(createHeader());

    m_server->writeToHost(data, pd.descriptor);

    if (m_is_walkie_talkie)
    {
        QByteArray result;
        result.append(getBytes<quint8>(Command::RemoteBufferTime));
        result.append(getBytes<qint32>(qMax(qCeil(m_streaming_info.timeToBuffer() / (qreal)10), 5)));

        m_server->writeToHost(result, pd.descriptor);
    }

    m_ready_to_write_extra_data = true;

    m_id_connections_list.append(pd.descriptor);
    m_host_connections_list.append(pd.host);

    m_hash_pkts_pending[pd.descriptor] = 0;
    m_hash_limit_reached[pd.descriptor] = false;
    m_hash_max_pkts_pending[pd.descriptor] = 0;

    emit connected(pd.host, id);
}

//Client disconnected from the app, currently a server
void Worker::serverClientDisconencted(const PeerData &pd)
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

    m_hash_pkts_pending.remove(pd.descriptor);
    m_hash_limit_reached.remove(pd.descriptor);
    m_hash_max_pkts_pending.remove(pd.descriptor);

    emit disconnected(pd.host);
}

//The app is currently connected to the server
void Worker::clientConencted(const PeerData &pd, const QString &id)
{
    m_ready_to_write_extra_data = true;

    m_id_connections_list.append(pd.descriptor);
    m_host_connections_list.append(pd.host);

    m_hash_pkts_pending[pd.descriptor] = 0;
    m_hash_limit_reached[pd.descriptor] = false;
    m_hash_max_pkts_pending[pd.descriptor] = 0;

    emit connected(pd.host, id);
}

//The app currently disconnected from the server
void Worker::clientDisconencted(const PeerData &pd)
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

    m_hash_pkts_pending.remove(pd.descriptor);
    m_hash_limit_reached.remove(pd.descriptor);
    m_hash_max_pkts_pending.remove(pd.descriptor);

    emit disconnected(pd.host);
}

void Worker::webClientConencted(const QByteArray &hash)
{
    emit connectedToServer(hash);
}

//The app is currently connected to the server
void Worker::webClientConnectedToPeer(const PeerData &pd, const QString &id)
{
    //Send the audio format to the peer
    QByteArray data;
    data.append(getBytes<quint8>(Command::AudioHeader));
    data.append(createHeader());

    m_client->write(data);

    QByteArray result;
    result.append(getBytes<quint8>(Command::RemoteBufferTime));
    result.append(getBytes<qint32>(qMax(qCeil(m_streaming_info.timeToBuffer() / (qreal)10), 5)));

    m_client->write(result);

    m_ready_to_write_extra_data = true;

    m_id_connections_list.append(pd.descriptor);
    m_host_connections_list.append(pd.host);

    m_hash_pkts_pending[pd.descriptor] = 0;
    m_hash_limit_reached[pd.descriptor] = false;
    m_hash_max_pkts_pending[pd.descriptor] = 0;

    emit connected(pd.host, id);
}

//The app currently disconnected from the server
void Worker::webClientDisconencted(const PeerData &pd)
{
    m_ready_to_write_extra_data = false;

    int index = m_id_connections_list.indexOf(pd.descriptor);

    if (index >= 0)
    {
        m_id_connections_list.removeAt(index);
        m_host_connections_list.removeAt(index);
    }

    m_hash_pkts_pending.remove(pd.descriptor);
    m_hash_limit_reached.remove(pd.descriptor);
    m_hash_max_pkts_pending.remove(pd.descriptor);

    emit disconnected(pd.host);
}

//Write data that can be or not relacted to audio streamed
void Worker::writeExtraData(const QByteArray &data)
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
void Worker::writeExtraDataResult()
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
void Worker::inputDataBack(const QByteArray &data)
{
    if (data.size() % sizeof(float) != 0)
    {
        errorPrivate("Corrupted audio data!");
        return;
    }

    QByteArray middle = data;

    if (m_input_muted && !middle.isEmpty())
    {
        Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>((float*)middle.data(), middle.size() / sizeof(float));
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

void Worker::posProcessedInput(const QByteArray &data)
{
    QByteArray result;
    result.append(getBytes<quint8>(Command::AudioData));
    result.append(data);

    if (m_server)
    {
        foreach (qintptr descriptor, m_id_connections_list)
        {
            int *pending = &m_hash_pkts_pending[descriptor];

            if (*pending <= m_hash_max_pkts_pending[descriptor] && !m_hash_limit_reached[descriptor])
            {
                LIB_DEBUG_LEVEL_2("Sent Audio Data - Size:" << result.size() << "bytes.");

                (*pending)++;

                m_server->writeToHost(result, descriptor);
            }
            else if (*pending > m_hash_max_pkts_pending[descriptor])
            {
                LIB_DEBUG_LEVEL_2("Not Sent Audio Data - Size:" << result.size() << "bytes.");

                m_hash_limit_reached[descriptor] = true;
            }
        }
    }
    else if (m_client)
    {
        foreach (qintptr descriptor, m_id_connections_list)
        {
            int *pending = &m_hash_pkts_pending[descriptor];

            if (*pending <= m_hash_max_pkts_pending[descriptor] && !m_hash_limit_reached[descriptor])
            {
                (*pending)++;

                m_client->write(result);
            }
            else if (*pending > m_hash_max_pkts_pending[descriptor])
            {
                m_hash_limit_reached[descriptor] = true;
            }
        }
    }
}

//Decode audio if using Opus codec
void Worker::preProcessOutput(const QByteArray &data)
{
#ifdef OPUS
    m_opus_dec->write(data);
#else
    posProcessedOutput(data);
#endif
}

void Worker::posProcessedOutput(const QByteArray &data)
{
    if (m_level_meter_output)
        m_level_meter_output->write(data);

    if (m_callback_enabled)
        emit outputData(data);
    else
        outputDataBack(data);
}

//Send callback output data to the output device
void Worker::outputDataBack(const QByteArray &data)
{
    if (data.size() % sizeof(float) != 0)
    {
        errorPrivate("Corrupted audio data!");
        return;
    }

    if (m_audio_output)
        m_audio_output->write(data);
}

//Convert flow control bytes to pseudo audio data
void Worker::flowControl(int bytes)
{
    QByteArray data = QByteArray(bytes, (char)0);

    if (!data.isEmpty())
    {
        Eigen::Ref<Eigen::VectorXf> samples = Eigen::Map<Eigen::VectorXf>((float*)data.data(), data.size() / sizeof(float));
        samples.fill(0);
    }

    if (m_callback_enabled)
        emit inputData(data);
    else
        inputDataBack(data);
}

bool Worker::isInputMuted() const
{
    return m_input_muted;
}

void Worker::setInputMuted(bool mute)
{
    m_input_muted = mute;
}

int Worker::volume() const
{
    return m_volume;
}

void Worker::setVolume(int volume)
{
    m_volume = volume;

    if (m_audio_output)
        m_audio_output->setVolume(m_volume);
}

StreamingInfo Worker::streamingInfo() const
{
    return m_streaming_info;
}

QList<QHostAddress> Worker::connectionsList() const
{
    return m_host_connections_list;
}

bool Worker::isReadyToWriteExtraData() const
{
    return m_ready_to_write_extra_data;
}

void Worker::processServerInput(const PeerData &pd)
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

        QByteArray result;
        result.append(getBytes<quint8>(Command::AudioDataReceived));

        m_server->writeToAll(result);

        preProcessOutput(data);

        break;
    }
    case Command::AudioDataReceived:
    {
        if (!data.isEmpty())
        {
            m_server->abort(pd.descriptor);
            return;
        }

        int *pending = &m_hash_pkts_pending[pd.descriptor];

        (*pending)--;

        *pending = qMax(*pending, 0);

        if (*pending == 0)
            m_hash_limit_reached[pd.descriptor] = false;

        break;
    }
    case Command::RemoteBufferTime:
    {
        if (data.size() != 4)
        {
            m_server->abort(pd.descriptor);
            return;
        }

        m_hash_max_pkts_pending[pd.descriptor] = getValue<qint32>(data);

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

void Worker::processClientInput(const PeerData &pd)
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

        header(data, &inputFormat, &audioFormat);

        m_streaming_info.setInputAudioFormat(inputFormat);
        m_streaming_info.setAudioFormat(audioFormat);

        if (INVALID_FORMAT)
        {
            errorPrivate("Sample type and/or endiannes not supported!");
            return;
        }

        adjustSettingsPrivate(m_is_walkie_talkie, true, true);

        if (m_is_walkie_talkie)
        {
            m_level_meter_input = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
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

        QByteArray result;
        result.append(getBytes<quint8>(Command::RemoteBufferTime));
        result.append(getBytes<qint32>(qMax(qCeil(m_streaming_info.timeToBuffer() / (qreal)10), 5)));

        m_client->write(result);

        break;
    }
    case Command::AudioData:
    {
        LIB_DEBUG_LEVEL_2("Got Audio Data - Size:" << data.size() << "bytes.");

        QByteArray result;
        result.append(getBytes<quint8>(Command::AudioDataReceived));

        m_client->write(result);

        preProcessOutput(data);

        break;
    }
    case Command::AudioDataReceived:
    {
        if (!data.isEmpty())
        {
            m_client->abort();
            return;
        }

        int *pending = &m_hash_pkts_pending[pd.descriptor];

        (*pending)--;

        *pending = qMax(*pending, 0);

        if (*pending == 0)
            m_hash_limit_reached[pd.descriptor] = false;

        break;
    }
    case Command::RemoteBufferTime:
    {
        if (data.size() != 4)
        {
            m_client->abort();
            return;
        }

        m_hash_max_pkts_pending[pd.descriptor] = getValue<qint32>(data);

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

void Worker::processWebClientInput(const PeerData &pd)
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

        header(data, &inputFormat, &audioFormat);

        if (INVALID_FORMAT)
        {
            errorPrivate("Sample type and/or endiannes not supported!");
            return;
        }

        if (m_streaming_info.inputAudioFormat() != inputFormat ||
                m_streaming_info.audioFormat() != audioFormat)
        {
            errorPrivate("Incompatible audio formats!");
            return;
        }

        adjustSettingsPrivate(true, true, true);

        {
            m_level_meter_input = new LevelMeter();
            connect(this, &Worker::destroyed, m_level_meter_input, &LevelMeter::deleteLater);
            connect(m_level_meter_input, &LevelMeter::currentlevel, this, &Worker::inputLevel);
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

        QByteArray result;
        result.append(getBytes<quint8>(Command::RemoteBufferTime));
        result.append(getBytes<qint32>(qMax(qCeil(m_streaming_info.timeToBuffer() / (qreal)10), 5)));

        m_client->write(result);

        break;
    }
    case Command::AudioData:
    {
        LIB_DEBUG_LEVEL_2("Got Audio Data - Size:" << data.size() << "bytes.");

        QByteArray result;
        result.append(getBytes<quint8>(Command::AudioDataReceived));

        m_client->write(result);

        preProcessOutput(data);

        break;
    }
    case Command::AudioDataReceived:
    {
        if (!data.isEmpty())
        {
            m_client->abort();
            return;
        }

        int *pending = &m_hash_pkts_pending[pd.descriptor];

        (*pending)--;

        *pending = qMax(*pending, 0);

        if (*pending == 0)
            m_hash_limit_reached[pd.descriptor] = false;

        break;
    }
    case Command::RemoteBufferTime:
    {
        if (data.size() != 4)
        {
            m_client->abort();
            return;
        }

        m_hash_max_pkts_pending[pd.descriptor] = getValue<qint32>(data);

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

QByteArray Worker::createHeader()
{
    QByteArray data;

    QAudioFormat inputFormat = m_streaming_info.inputAudioFormat();
    QAudioFormat format = m_streaming_info.audioFormat();

    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << (qint32)format.sampleSize();
    stream << (qint32)format.sampleRate();
    stream << (qint32)format.channelCount();
    stream << (qint32)format.sampleType();
    stream << (qint32)format.byteOrder();

    stream << (qint32)inputFormat.sampleSize();
    stream << (qint32)inputFormat.sampleRate();
    stream << (qint32)inputFormat.channelCount();
    stream << (qint32)inputFormat.sampleType();
    stream << (qint32)inputFormat.byteOrder();

    if (m_is_walkie_talkie)
        stream << (qint32)m_streaming_info.timeToBuffer();
    else
        stream << (qint32)0;

#ifdef OPUS
    stream << m_frame_size;
    stream << m_max_frame_size;
    stream << m_bitrate;
#else
    stream << (qint32)0;
    stream << (qint32)0;
    stream << (qint32)0;
#endif

    return data;
}

void Worker::header(QByteArray data,
                    QAudioFormat *refInputAudioFormat,
                    QAudioFormat *refAudioFormat)
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

    if (m_is_walkie_talkie)
        m_streaming_info.setTimeToBuffer(timeToBuffer);

    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setSampleSize(sampleSize);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
    format.setSampleType((QAudioFormat::SampleType)sampleType);
    format.setByteOrder((QAudioFormat::Endian)byteOrder);

    QAudioFormat inputFormat;
    inputFormat.setCodec("audio/pcm");
    inputFormat.setSampleSize(inputSampleSize);
    inputFormat.setSampleRate(inputSampleRate);
    inputFormat.setChannelCount(inputChannelCount);
    inputFormat.setSampleType((QAudioFormat::SampleType)inputSampleType);
    inputFormat.setByteOrder((QAudioFormat::Endian)inputByteOrder);

    *refInputAudioFormat = inputFormat;
    *refAudioFormat = format;
}
