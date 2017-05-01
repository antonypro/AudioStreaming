#include "opusencoderclass.h"

//Opus encoder class

OpusEncoderClass::OpusEncoderClass(QObject *parent) : QObject(parent)
{
    m_initialized = false;
}

OpusEncoderClass::~OpusEncoderClass()
{
    if (!m_initialized)
        return;

    /*Destroy the encoder state*/
    opus_encoder_destroy(m_encoder);
}

void OpusEncoderClass::start(int sample_rate, int channels, int bitrate, int frame_size, int application)
{
    if (m_initialized)
        return;

    int err;

    /*Create a new encoder state */
    m_encoder = opus_encoder_create(sample_rate, channels, application, &err);
    if (err<0)
    {
        emit error(QString("Failed to create an Opus encoder: %0").arg(opus_strerror(err)));
        return;
    }

    m_initialized = true;

    /* Set the desired bit-rate. You can also set other parameters if needed.
          The Opus library is designed to have good defaults, so only set
          parameters you know you need. Doing otherwise is likely to result
          in worse quality, but better. */
    err = opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(bitrate));
    if (err<0)
    {
        emit error(QString("Failed to set Opus bitrate: %0").arg(opus_strerror(err)));
        return;
    }

    m_channels = channels;
    m_frame_size = frame_size;
}

void OpusEncoderClass::write(const QByteArray &data)
{
    m_buffer.append(data);

    QByteArray result;

    while ((result = encode()) != QByteArray())
        emit encoded(result);
}

QByteArray OpusEncoderClass::encode()
{
    if (!m_initialized)
        return QByteArray();

    int size = sizeof(opus_int16) * m_channels * m_frame_size;

    if (m_buffer.size() < size)
        return QByteArray();

    opus_int16 *in = new opus_int16[m_frame_size * m_channels];
    unsigned char *cbits = new unsigned char[MAX_PACKET_SIZE];
    int nbBytes;

    int i;
    unsigned char *pcm_bytes = new unsigned char[size];

    /* Read a 16 bits/sample audio frame. */
    memcpy(pcm_bytes, m_buffer.data(), size);

    m_buffer.remove(0, size);

    /* Convert from little-endian ordering. */
    for (i=0;i<m_channels*m_frame_size;i++)
        in[i]=pcm_bytes[2*i+1]<<8|pcm_bytes[2*i];

    /* Encode the frame. */
    nbBytes = opus_encode(m_encoder, in, m_frame_size, cbits, MAX_PACKET_SIZE);
    if (nbBytes<0)
    {
        emit error(QString("Opus encode failed: %0").arg(opus_strerror(nbBytes)));

        delete[] pcm_bytes;

        delete[] in;
        delete[] cbits;

        return QByteArray();
    }

    /* Write the encoded audio to output. */
    QByteArray output = QByteArray((char*)cbits, nbBytes);

    delete[] pcm_bytes;

    delete[] in;
    delete[] cbits;

    return output;
}
