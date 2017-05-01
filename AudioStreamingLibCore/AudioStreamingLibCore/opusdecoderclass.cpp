#include "opusdecoderclass.h"

//Opus decoder class

OpusDecoderClass::OpusDecoderClass(QObject *parent) : QObject(parent)
{
    m_initialized = false;
}

OpusDecoderClass::~OpusDecoderClass()
{
    if (!m_initialized)
        return;

    /*Destroy the encoder state*/
    opus_decoder_destroy(m_decoder);
}

void OpusDecoderClass::start(int sample_rate, int channels, int frame_size, int max_frame_size)
{
    if (m_initialized)
        return;

    /* Create a new decoder state. */
    int err;
    m_decoder = opus_decoder_create(sample_rate, channels, &err);
    if (err<0)
    {
        emit error(QString("Failed to create Opus decoder: %0").arg(opus_strerror(err)));
        return;
    }

    m_channels = channels;
    m_frame_size = frame_size;
    m_max_frame_size = max_frame_size;

    m_initialized = true;
}

void OpusDecoderClass::write(const QByteArray &data)
{
    if (!m_initialized)
        return;

    int i;
    int frame_size;
    /* Decode the data. In this example, frame_size will be constant because
         the encoder is using a constant frame size. However, that may not
         be the case for all encoders, so the decoder must always check
         the frame size returned. */
    opus_int16 *out = new opus_int16[m_frame_size * m_channels];

    frame_size = opus_decode(m_decoder, (unsigned char*)data.data(), data.size(), out, m_max_frame_size, 0);
    if (frame_size<0)
    {
        emit error(QString("Opus decoder failed: %0").arg(opus_strerror(frame_size)));
        delete[] out;
        return;
    }

    int size = sizeof(opus_int16) * m_channels * frame_size;
    unsigned char *pcm_bytes = new unsigned char[size];

    /* Convert to little-endian ordering. */
    for(i=0;i<m_channels*frame_size;i++)
    {
        pcm_bytes[2*i]=out[i]&0xFF;
        pcm_bytes[2*i+1]=(out[i]>>8)&0xFF;
    }
    /* Write the decoded audio to output. */
    QByteArray output = QByteArray((char*)pcm_bytes, size);

    delete[] pcm_bytes;
    delete[] out;

    emit decoded(output);
}
