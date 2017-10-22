#ifndef WORKER_H
#define WORKER_H

#include <QtCore>
#include "common.h"
#include "audiostreaminglibcore.h"
#include "discoverserver.h"
#include "server.h"
#include "client.h"
#include "sslserver.h"
#include "sslclient.h"
#include "audioinput.h"
#include "audiooutput.h"
#include "flowcontrol.h"
#include "levelmeter.h"
#ifdef OPUS
#include "opusdecoderclass.h"
#include "opusencoderclass.h"
#include "r8brain.h"
#endif

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

signals:
    void connected(QHostAddress,QString);
    void disconnected(QHostAddress);
    void pending(QHostAddress,QString);
    void inputData(QByteArray);
    void veryInputData(QByteArray);
    void outputData(QByteArray);
    void veryOutputData(QByteArray);
    void extraData(QByteArray);
    void inputLevel(float);
    void outputLevel(float);
    void adjustSettings();
    void extraDataWritten();
    void error(QString);

public slots:
    void start(const StreamingInfo &streaming_info);
    void setKeys(const QByteArray &private_key, const QByteArray &public_key);
    void listen(quint16 port, bool auto_accept, const QByteArray &password, int max_connections);
    void connectToHost(const QString &host, quint16 port, const QByteArray &password);
    void acceptConnection();
    void rejectConnection();
    void writeExtraData(const QByteArray &data);
    void writeExtraDataResult();
    void inputDataBack(const QByteArray &data);
    void outputDataBack(const QByteArray &data);
    bool isInputMuted() const;
    void setInputMuted(bool mute);
    int volume() const;
    void setVolume(int volume);
    StreamingInfo streamingInfo() const;
    QList<QHostAddress> connectionsList() const;
    bool isReadyToWriteExtraData() const;

private slots:
    void errorPrivate(const QString &error_description);
    void startOpusEncoder();
    void startOpusDecoder();
    void adjustSettingsPrivate(bool start_opus_encoder, bool start_opus_decoder, bool client_mode);
    void serverClientConencted(const PeerData &pd, const QString &id);
    void serverClientDisconencted(const PeerData &pd);
    void clientConencted(const PeerData &pd, const QString &id);
    void clientDisconencted(const PeerData &pd);
    void posProcessedInput(const QByteArray &data);
    void preProcessOutput(const QByteArray &data);
    void posProcessedOutput(const QByteArray &data);
    void flowControl(int bytes);
    void processServerInput(const PeerData &pd);
    void processClientInput(const PeerData &pd);
    QByteArray createHeader();
    void header(QByteArray data,
                QAudioFormat *refInputAudioFormat,
                QAudioFormat *refAudioFormat);

private:
    DiscoverServer *m_server_discover;
    AbstractServer *m_server;
    AbstractClient *m_client;
    AudioInput *m_audio_input;
    AudioOutput *m_audio_output;
    FlowControl *m_flow_control;
    LevelMeter *m_level_meter_input;
    LevelMeter *m_level_meter_output;
#ifdef OPUS
    r8brain *m_resampler;
    OpusEncoderClass *m_opus_enc;
    OpusDecoderClass *m_opus_dec;
#endif
    bool m_input_muted;
    int m_volume;
    bool m_has_error;
    bool m_is_walkie_talkie;
    StreamingInfo m_streaming_info;
    bool m_ready_to_write_extra_data;
    int m_extra_data_peers;
    QList<qintptr> m_id_connections_list;
    QList<QHostAddress> m_host_connections_list;
    bool m_callback_enabled;

    QHash<qintptr, int> m_hash_pkts_pending;
    QHash<qintptr, int> m_hash_max_pkts_pending;

#ifdef OPUS
    qint32 m_frame_size;
    qint32 m_max_frame_size;
    qint32 m_bitrate;
#endif
};

#endif // WORKER_H
