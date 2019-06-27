#ifndef AUDIOSTREAMINGLIBCORE_H
#define AUDIOSTREAMINGLIBCORE_H

#include <QtCore>
#include <QtMultimedia>
#include <QtNetwork>
#include "discoverclient.h"

class AudioStreamingWorker;

#define MIN_BUFFERED_TIME 10
#define MAX_BUFFERED_TIME 1000 //Time to buffer plus this value

#define MAX_ID_LENGTH 20

/** The AudioStreamingLibInfo class contains most settings of the library. This class replaces ```StreamingInfo```.
 *
 * **Note:** You should not keep a copy of it, when needed request it by calling *audioStreamingLibInfo()*
 * function of the *AudioStreamingLibCore* class.
 */
class AudioStreamingLibInfo
{
public:
    /** Creates the instance and initialize variables. */
    AudioStreamingLibInfo()
    {
        //Default values
        m_id = QString(MAX_ID_LENGTH, char(0));
        m_work_mode = StreamingWorkMode::Undefined;
        m_time_to_buffer = 0;
        m_input_dev_type = AudioDeviceType::LibraryAudioDevice;
        m_output_dev_type = AudioDeviceType::LibraryAudioDevice;

        m_input = QAudioDeviceInfo::defaultInputDevice();

        if (m_input.isNull())
        {
            QList<QAudioDeviceInfo> input_devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

            if (!input_devices.isEmpty())
            {
                m_input = input_devices.first();
            }
        }

        m_output = QAudioDeviceInfo::defaultOutputDevice();

        if (m_output.isNull())
        {
            QList<QAudioDeviceInfo> output_devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

            if (!output_devices.isEmpty())
            {
                m_output = output_devices.first();
            }
        }

        m_opus_bitrate = 128 * 1000;
        m_callback_enabled = false;
        m_is_get_audio_enabled = false;
        m_listen_audio_input_enabled = false;
        m_encryption_enabled = false;
    }

    //\cond HIDDEN_SYMBOLS
    enum class StreamingWorkMode
    {
        Undefined,
        BroadcastClient,
        BroadcastServer,
        WalkieTalkieClient,
        WalkieTalkieServer,
        WebClient
    };

    enum class AudioDeviceType
    {
        LibraryAudioDevice,
        CustomAudioDevice
    };
    //\endcond

    /** \private */
    QByteArray negotiationString()
    {
        return m_negotiation_string;
    }

    /** \private */
    QString ID()
    {
        return m_id;
    }

    /** \private */
    StreamingWorkMode workMode()
    {
        return m_work_mode;
    }

    /** \private */
    qint32 timeToBuffer()
    {
        return m_time_to_buffer;
    }

    /** \private */
    AudioDeviceType inputDeviceType()
    {
        return m_input_dev_type;
    }

    /** \private */
    AudioDeviceType outputDeviceType()
    {
        return m_output_dev_type;
    }

    /** \private */
    QAudioFormat inputAudioFormat()
    {
        return m_input_format;
    }

    /** \private */
    QAudioFormat audioFormat()
    {
        return m_format;
    }

    /** \private */
    QAudioDeviceInfo inputDeviceInfo()
    {
        return m_input;
    }

    /** \private */
    QAudioDeviceInfo outputDeviceInfo()
    {
        return m_output;
    }

    /** \private */
    int OpusBitrate()
    {
        return m_opus_bitrate;
    }

    /** \private */
    bool isCallBackEnabled()
    {
        return m_callback_enabled;
    }

    /** \private */
    bool isGetAudioEnabled()
    {
        return m_is_get_audio_enabled;
    }

    /** \private */
    bool isListenAudioInputEnabled()
    {
        return m_listen_audio_input_enabled;
    }

    /** \private */
    bool isEncryptionEnabled()
    {
        return m_encryption_enabled;
    }

    /** Use this function to prevent conflicts of different applications
     * accessing the library by the same port, search for servers conflicts
     * are also avoided if set the negotiation string.
     *
     * Note that the maximum value is 128 bytes.
     */
    void setNegotiationString(const QByteArray &negotiation_string)
    {
        m_negotiation_string = negotiation_string;
    }

    /** ID is a friendly name that identifies you.
     *
     * Note that the maximum length is 20.
     */
    void setID(const QString &id)
    {
        m_id = id.leftJustified(MAX_ID_LENGTH, char(0), true);
    }

    /** Sets the work mode of the library.
     *
     * The possible values are:
     *
     * ```
     * Undefined
     * BroadcastClient
     * BroadcastServer
     * WalkieTalkieClient
     * WalkieTalkieServer
     * WebClient
     * ```
     *
     * The default value is ```Undefined```.
     */
    void setWorkMode(const StreamingWorkMode &work_mode)
    {
        m_work_mode = work_mode;
    }

    /** Sets the output audio time to buffer. To play with the lowest delay set it to 0.
     *
     * **Note:** If you set a value lower than 10 ms it will be reduced to 0 internally.
     */
    void setTimeToBuffer(qint32 ms_time)
    {
        m_time_to_buffer = ms_time;
        if (m_time_to_buffer < MIN_BUFFERED_TIME)
            m_time_to_buffer = 0;
    }

    /** Define if using the internal audio input device,
     * or ignore it and feed input with other data,
     * most useful with callbacks enabled.
     *
     * The possible values are:
     *
     * ```
     * LibraryAudioDevice
     * CustomAudioDevice
     * ```
     *
     * The default is ```LibraryAudioDevice```.
     */
    void setInputDeviceType(const AudioDeviceType &input_type)
    {
        m_input_dev_type = input_type;
    }

    /**
     * Define if using the internal audio output device,
     * or ignore it and play output with other output device or not play it at all,
     * also useful with callbacks enabled for recording audio without playing it.
     *
     * The possible values are:
     *
     * ```
     * LibraryAudioDevice
     * CustomAudioDevice
     * ```
     *
     * The default is ```LibraryAudioDevice```.
     */
    void setOutputDeviceType(const AudioDeviceType &output_type)
    {
        m_output_dev_type = output_type;
    }

    /** Sets the audio input format.
     *
     * Only use if in server mode, in client mode it will be got by the server.
     */
    void setInputAudioFormat(const QAudioFormat &format)
    {
        m_input_format = format;
        m_input_format.setCodec("audio/pcm");
    }

    /** Set the audio format after resampling if using Opus,
     * otherwise equals to audio format.
     *
     * **DO NOT** change this setting,
     * it will be automatically filled in client/server modes based on input audio format.
     */
    void setAudioFormat(const QAudioFormat &format)
    {
        m_format = format;
        m_format.setCodec("audio/pcm");
    }

    /** If using ```LibraryAudioDevice``` set the specific input device. */
    void setInputDeviceInfo(const QAudioDeviceInfo &dev_info)
    {
        m_input = dev_info;
    }

    /** If using ```LibraryAudioDevice``` set the specific output device. */
    void setOutputDeviceInfo(const QAudioDeviceInfo &dev_info)
    {
        m_output = dev_info;
    }

    /** Sets Opus bitrate, if not using Opus this value is undefined. */
    void setOpusBitrate(int bitrate)
    {
        m_opus_bitrate = bitrate;
    }

    /** Enable callback input and output audio data,
     * you’ll receive the audio through the SIGNALS *inputData()* and *outputData()*
     * and can process the audio and write the audio back
     * with the functions *inputDataBack()* and *outputDataBack()*.
     */
    void setCallBackEnabled(bool enable)
    {
        m_callback_enabled = enable;
    }

    /** Enable get audio input and output,
     * if using Opus returns input audio after resampler in the *veryInputData()* SIGNAL,
     * and output audio immediately before play audio, in the *veryOutputData()* SIGNAL.
     */
    void setGetAudioEnabled(bool enable)
    {
        m_is_get_audio_enabled = enable;
    }

    /** If enabled this route the input audio data to the output device,
     * so you can listen what you are streaming.
     *
     * **Note:** this function have effect only in ```BroadcastServer``` mode!
     */
    void setListenAudioInputEnabled(bool enable)
    {
        m_listen_audio_input_enabled = enable;
    }

    /** Sets if encryption is enabled or not,
     * note that the OpenSsl must be available to successfully start an encrypted server or client.
     */
    void setEncryptionEnabled(bool enable)
    {
        m_encryption_enabled = enable;
    }

private:
    QByteArray m_negotiation_string;
    QString m_id;
    StreamingWorkMode m_work_mode;
    qint32 m_time_to_buffer;
    AudioDeviceType m_input_dev_type;
    AudioDeviceType m_output_dev_type;
    QAudioFormat m_input_format;
    QAudioFormat m_format;
    QAudioDeviceInfo m_input;
    QAudioDeviceInfo m_output;
    int m_opus_bitrate;
    bool m_callback_enabled;
    bool m_is_get_audio_enabled;
    bool m_listen_audio_input_enabled;
    bool m_encryption_enabled;
};

inline QDataStream &operator<<(QDataStream &stream, AudioStreamingLibInfo &info);
inline QDataStream &operator>>(QDataStream &stream, AudioStreamingLibInfo &info);

inline QDebug &operator<<(QDebug &debug, AudioStreamingLibInfo &info);

/** The AudioStreamingLibCore class is the bridge between your app
 * and the internals of the AudioStreamingLib.
 */
class AudioStreamingLibCore : public QObject
{
    Q_OBJECT
public:
    /** Creates the instance. */
    explicit AudioStreamingLibCore(QObject *parent = nullptr);

    /** Destroys the instance. */
    ~AudioStreamingLibCore();

    /** Returns the size in bytes of the audio time. Given channel count, sample size and sample rate. */
    static qint64 timeToSize(qint64 ms_time, int channel_count, int sample_size, int sample_rate);

    /** Returns the size in bytes of the audio time. Given QAudioFormat. */
    static qint64 timeToSize(qint64 ms_time, const QAudioFormat &format);

    /** Returns the time of audio in ms of the audio size in bytes. */
    static qint64 sizeToTime(qint64 bytes, int channel_count, int sample_size, int sample_rate);

    /** Returns the time of audio in ms of the audio size in bytes. */
    static qint64 sizeToTime(qint64 bytes, const QAudioFormat &format);

    /** Helper function to convert float samples to signed integer 16 bits samples. */
    static QByteArray convertFloatToInt16(const QByteArray &data);

    /** Helper function to convert signed integer 16 bits samples to float samples. */
    static QByteArray convertInt16ToFloat(const QByteArray &data);

    /** Helper function to mix the two float audio data into one data. */
    static QByteArray mixFloatAudio(const QByteArray &data1, const QByteArray &data2);

    /** Get the current Eigen instructions set. */
    static QString EigenInstructionsSet();

signals:
    /** Tells that a new connection is done and inform the address and the ID. */
    void connected(QHostAddress,QString);

    /** Tells that a connection with the WebServer is done and return the
     * SHA-256 hash code of the server certificate. */
    void connectedToServer(QByteArray);

    /** Tells that are a client asking for permission to connect.
     *
     * Answer calling *acceptConnection()* to accept or *rejectConnection()* to reject.
     */
    void disconnected(QHostAddress);

    /** Tells some peer was disconnected and the address. */
    void pending(QHostAddress,QString);

    /** Client successfully logged to server. */
    void webClientLoggedIn();

    /** Warning gave by server. */
    void webClientWarning(QString);

    /** Receive the input data if callback mode is enabled.
     *
     * **Note:** Remember that you need to feedback the data by calling *inputDataBack()*.
     */
    void inputData(QByteArray);

    /** Receive the input data if *setGetAudioEnabled()* mode is enabled. No need to feedback. */
    void veryInputData(QByteArray);

    /** Receive the output data if callback mode is enabled.
     *
     * **Note:** Remember that you need to feedback the data by calling *outputDataBack()*
     * if you configured to play audio.
     */
    void outputData(QByteArray);

    /** Receive the output data if *setGetAudioEnabled()* mode is enabled. No need to feedback.
     *
     * This SIGNAL returns audio data
     * immediately before volume is applied to samples and the audio being played.
     */
    void veryOutputData(QByteArray);

    /** Receive the extra data. */
    void extraData(QByteArray);

    /** Input level as float in range 0 to 1.0. */
    void inputLevel(float);

    /** Output level as float in range 0 to 1.0. */
    void outputLevel(float);

    /** The audio settings is ready to being used.
     *
     * To get the settings call *audioFormat()* and *inputAudioFormat()*.
     */
    void adjustSettings();

    /** SIGNAL used to tell that the all peers have called *writeExtraDataResult()*. */
    void extraDataWritten();

    /** The library has stopped. */
    void finished();

    /** Got an XML command from WebServer. */
    void commandXML(QByteArray);

    /** The library has generated a warning, but not stopped.*/
    void warning(QString);

    /** The library has stopped due to an error.
     *
     * Most times a description of the error is available, otherwise the error is empty.
     */
    void error(QString);

public slots:
    /** Initialize the library with the given *AudioStreamingLibInfo* settings. */
    bool start(const AudioStreamingLibInfo &streaming_info);

    /** Stops the library, note that is called by destructor. */
    void stop();

    /** Verify if library is currently running. */
    bool isRunning();

    /** If some input device is already running, change the input device to ```dev_info```. */
    void changeInputDevice(const QAudioDeviceInfo &dev_info);

    /** If some output device is already running, change the output device to ```dev_info```. */
    void changeOutputDevice(const QAudioDeviceInfo &dev_info);

    /** Create an instance of a class for searching peers on
     * local network and returns a pointer to this instance.
     *
     * It will be auto deleted before ```time_to_destroy``` time elapsed(minimum accepted value 1000 ms),
     * also calling the function while discover client instance is running
     * will delete the instance and return a pointer to a new instance.
     */
    DiscoverClient *discoverInstance(int time_to_destroy = 10000);

    /** Start listening state
     *
     * Start listening state with the given port,
     * auto accept connections choice,
     * optional password and maximum connections
     * (Walkie Talkie Server ignore this option and always accepts only one connection).
     *
     * **Note:** this **must** be called **after** calling *start()* with some server work mode option.
     */
    void listen(quint16 port, bool auto_accept = true, const QByteArray &password = QByteArray(), int max_connections = 30);

    /** Starts the connection with the given host, port, and password if in Encrypted mode.
     *
     * Also used to connect to the WebServer.
     *
     * **Note:** this **must** be called **after** calling *start()* with some client work mode option.
     */
    void connectToHost(const QString &host, quint16 port, const QByteArray &password = QByteArray());

    /** Write command to WebServer in XML format. */
    void writeCommandXML(const QByteArray &XML);

    /** Asks the WebServer to start a conversation with the specified ID. */
    void connectToPeer(const QString &ID);

    /** Disconnect from conversation with peer.
     *
     * **Note:** this does not disconnect you from the WebServer.
     * And once time disconnected from some peer you can call or receive calling requests
     * from another peer.
     */
    void disconnectFromPeer();

    /** Accept new pending connection in response to *pending()* SIGNAL. */
    void acceptConnection();

    /** Reject new pending connection in response to *pending()* SIGNAL. */
    void rejectConnection();

    /** Accepts the certificate gave by WebServer. */
    void acceptSslCertificate();

    /** Writes extra data from server to all connections or from client to server. */
    void writeExtraData(const QByteArray &data);

    /** Tells to another side of connection that the extra data was received
     * and the client or server is ready to receive more. */
    void writeExtraDataResult();

    /** If callback mode is enabled this function is used to feed back the input with data. */
    void inputDataBack(const QByteArray &data);

    /** If callback mode is enabled this function is used to feed back the output with data. */
    void outputDataBack(const QByteArray &data);

    /** Tells if input audio is currently muted. */
    bool isInputMuted();

    /** Mute the audio input. */
    void setInputMuted(bool mute);

    /** Returns the current output volume. */
    int volume();

    /** Set the output volume with 0 as mute and 100 the loudest volume with attenuation and 150 with attenuation ignored. */
    void setVolume(int volume);

    /** Returns a copy of current state of the *AudioStreamingLibInfo* class. */
    AudioStreamingLibInfo audioStreamingLibInfo();

    /** Returns a list of connected clients only useful in server mode. */
    QList<QHostAddress> connectionsList();

    /** Tell if writeExtraDataResult was called by all clients or
     * by server and it's ready for subsequent extra data. */
    bool isReadyToWriteExtraData();

    /** Function to query the audio format used after the re sampler and used with Opus codec,
     * if not using Opus, this will be always the same as *inputAudioFormat()*. */
    QAudioFormat audioFormat();

    /** Function to query the audio format used to feed the input data,
     * may be different than *audioFormat()* if using Opus codec. */
    QAudioFormat inputAudioFormat();

private:
    bool m_input_muted;
    int m_volume;
    AudioStreamingWorker *m_worker;
    QPointer<DiscoverClient> m_client_discover;
    bool m_running;
    QAudioFormat m_audio_format;
    QAudioFormat m_input_format;
};

#endif // AUDIOSTREAMINGLIBCORE_H
