#include "opusdecode.h"

OpusDecode::OpusDecode(QObject *parent) : QObject(parent)
{

}

OpusDecode::~OpusDecode()
{
    if (!initialized)
        return;

    /*Destroy the encoder state*/
    opus_decoder_destroy(decoder);
}

void OpusDecode::start(int CHANNELS, int SAMPLE_RATE)
{
    initialized = false;

    /* Create a new decoder state. */
    int err;
    decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (err<0)
    {
        qDebug() << "failed to create decoder:" << opus_strerror(err);
        return;
    }

    channels = CHANNELS;

    initialized = true;
}

QByteArray OpusDecode::decode(const QByteArray &data)
{
    if (!initialized)
        return QByteArray();

    int i;
    int frame_size;
    /* Decode the data. In this example, frame_size will be constant because
         the encoder is using a constant frame size. However, that may not
         be the case for all encoders, so the decoder must always check
         the frame size returned. */
    opus_int16 *out = new opus_int16[FRAME_SIZE*channels];

    frame_size = opus_decode(decoder, (unsigned char*)data.data(), data.size(), out, MAX_FRAME_SIZE, 0);
    if (frame_size<0)
    {
        qDebug() << "decoder failed:" << opus_strerror(frame_size);
        delete[] out;
        return QByteArray();
    }

    int size = sizeof(qint16)*channels*frame_size;
    unsigned char *pcm_bytes = new unsigned char[size];

    /* Convert to little-endian ordering. */
    for(i=0;i<channels*frame_size;i++)
    {
        pcm_bytes[2*i]=out[i]&0xFF;
        pcm_bytes[2*i+1]=(out[i]>>8)&0xFF;
    }
    /* Write the decoded audio to file. */
    QByteArray output((char*)pcm_bytes, size);

    delete[] pcm_bytes;
    delete[] out;

    return output;
}
