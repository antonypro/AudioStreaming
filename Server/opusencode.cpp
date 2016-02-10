#include "opusencode.h"

OpusEncode::OpusEncode(QObject *parent) : QObject(parent)
{

}

OpusEncode::~OpusEncode()
{
    if (!initialized)
        return;

    /*Destroy the encoder state*/
    opus_encoder_destroy(encoder);
}

void OpusEncode::start(int CHANNELS, int SAMPLE_RATE)
{
    initialized = false;

    int err;

    /*Create a new encoder state */
    encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, APPLICATION, &err);
    if (err<0)
    {
        qDebug() << "failed to create an encoder:" << opus_strerror(err);
        return;
    }
    /* Set the desired bit-rate. You can also set other parameters if needed.
          The Opus library is designed to have good defaults, so only set
          parameters you know you need. Doing otherwise is likely to result
          in worse quality, but better. */
    err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));
    if (err<0)
    {
        qDebug() << "failed to set bitrate:" << opus_strerror(err);
        return;
    }

    channels = CHANNELS;

    initialized = true;
}

QByteArray OpusEncode::encode(const QByteArray &data)
{
    if (!initialized)
        return QByteArray();

    opus_int16 *in = new opus_int16[FRAME_SIZE*channels];
    unsigned char *cbits = new unsigned char[MAX_PACKET_SIZE];
    int nbBytes;

    int size = sizeof(qint16)*channels*FRAME_SIZE;

    int i;
    unsigned char *pcm_bytes = new unsigned char[size];

    /* Read a 16 bits/sample audio frame. */
    memcpy(pcm_bytes, data.data(), size);

    /* Convert from little-endian ordering. */
    for (i=0;i<channels*FRAME_SIZE;i++)
        in[i]=pcm_bytes[2*i+1]<<8|pcm_bytes[2*i];

    /* Encode the frame. */
    nbBytes = opus_encode(encoder, in, FRAME_SIZE, cbits, MAX_PACKET_SIZE);
    if (nbBytes<0)
    {
        qDebug() << "encode failed:" << opus_strerror(nbBytes);

        delete[] pcm_bytes;

        delete[] in;
        delete[] cbits;

        return QByteArray();
    }

    QByteArray dataret((char*)cbits, nbBytes);

    delete[] pcm_bytes;

    delete[] in;
    delete[] cbits;

    return dataret;
}
