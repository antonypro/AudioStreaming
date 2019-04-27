#include "mp3recorder.h"

MP3Recorder::MP3Recorder(QObject *parent) : QObject(parent)
{
    m_lame = nullptr;
}

MP3Recorder::~MP3Recorder()
{
    stop();
}

bool MP3Recorder::loadFunction()
{
#ifdef Q_OS_WIN
    QLibrary lame_enc("LIBMP3LAME-0");
#else
    QLibrary lame_enc("mp3lame");
#endif

    if (!lame_enc.load())
        return false;

    if (!(plame_init = tlame_init(lame_enc.resolve("lame_init"))))
        return false;

    if (!(plame_set_in_samplerate = tlame_set_in_samplerate(lame_enc.resolve("lame_set_in_samplerate"))))
        return false;

    if (!(plame_set_VBR = tlame_set_VBR(lame_enc.resolve("lame_set_VBR"))))
        return false;

    if (!(plame_set_brate = tlame_set_brate(lame_enc.resolve("lame_set_brate"))))
        return false;

    if (!(plame_set_num_channels = tlame_set_num_channels(lame_enc.resolve("lame_set_num_channels"))))
        return false;

    if (!(plame_init_params = tlame_init_params(lame_enc.resolve("lame_init_params"))))
        return false;

    if (!(plame_encode_flush = tlame_encode_flush(lame_enc.resolve("lame_encode_flush"))))
        return false;

    if (!(plame_encode_buffer_interleaved = tlame_encode_buffer_interleaved(lame_enc.resolve("lame_encode_buffer_interleaved"))))
        return false;

    if (!(plame_encode_buffer = tlame_encode_buffer(lame_enc.resolve("lame_encode_buffer"))))
        return false;

    if (!(plame_close = tlame_close(lame_enc.resolve("lame_close"))))
        return false;

    if (!(pid3tag_init = tid3tag_init(lame_enc.resolve("id3tag_init"))))
        return false;

    if (!(pid3tag_v2_only = tid3tag_v2_only(lame_enc.resolve("id3tag_v2_only"))))
        return false;

    if (!(pid3tag_set_title = tid3tag_set_title(lame_enc.resolve("id3tag_set_title"))))
        return false;

    return true;
}

bool MP3Recorder::start(const QString &mp3_path, const QAudioFormat &mp3_format, int mp3_bitrate, const QString &title)
{
    if (m_lame)
        return false;

    if (!loadFunction())
        return false;

    m_mp3_file = new QFile(mp3_path, this);

    if (!m_mp3_file->open(QFile::WriteOnly | QFile::Truncate))
        return false;

    CHANNELS = mp3_format.channelCount();
    SAMPLE_RATE = mp3_format.sampleRate();
    BITRATE = mp3_bitrate;

    m_lame = plame_init();
    plame_set_in_samplerate(m_lame, SAMPLE_RATE);
    plame_set_VBR(m_lame, vbr_off); //force CBR mode
    plame_set_brate(m_lame, BITRATE);
    plame_set_num_channels(m_lame, CHANNELS);
    pid3tag_init(m_lame);
    pid3tag_v2_only(m_lame);
    pid3tag_set_title(m_lame, qPrintable(title));
    plame_init_params(m_lame);

    return true;
}

void MP3Recorder::encode(const QByteArray &data)
{
    if (!m_lame)
        return;

    qint16 *pcm_buffer = reinterpret_cast<qint16*>(const_cast<char*>(data.data()));
    int read = data.size();
    int write = 0;

    if (read == 0)
        write = plame_encode_flush(m_lame, mp3_buffer, MP3_SIZE);
    else if (CHANNELS == 2)
        write = plame_encode_buffer_interleaved(m_lame, pcm_buffer, read / (CHANNELS * int(sizeof(qint16))), mp3_buffer, MP3_SIZE);
    else
        write = plame_encode_buffer(m_lame, pcm_buffer, nullptr, read / int(sizeof(qint16)), mp3_buffer, MP3_SIZE);

    if (write > 0)
        m_mp3_file->write(reinterpret_cast<char*>(mp3_buffer), write);
}

void MP3Recorder::stop()
{
    if (!m_lame)
        return;

    m_mp3_file->close();
    m_mp3_file->deleteLater();
    plame_close(m_lame);

    m_lame = nullptr;
}
