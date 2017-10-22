#include "opusdecoderclass.h"

//Opus decoder class

OpusDecoderClass::OpusDecoderClass(QObject *parent) : QObject(parent)
{
    m_initialized = false;

    START_THREAD
}

OpusDecoderClass::~OpusDecoderClass()
{
    if (!m_initialized)
        return;

    /*Destroy the encoder state*/
    opus_decoder_destroy(m_decoder);
}

void OpusDecoderClass::startPrivate(int sample_rate, int channels, int frame_size, int max_frame_size)
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

void OpusDecoderClass::start(int sample_rate, int channels, int frame_size, int max_frame_size)
{
    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection,
                              Q_ARG(int, sample_rate),
                              Q_ARG(int, channels),
                              Q_ARG(int, frame_size),
                              Q_ARG(int, max_frame_size));
}

void OpusDecoderClass::writePrivate(const QByteArray &data)
{
    if (!m_initialized)
        return;

    int frame_size;
    /* Decode the data. In this example, frame_size will be constant because
         the encoder is using a constant frame size. However, that may not
         be the case for all encoders, so the decoder must always check
         the frame size returned. */

    QByteArray output = QByteArray(sizeof(float) * m_channels * m_frame_size, (char)0);

    frame_size = opus_decode_float(m_decoder, (const uchar*)data.constData(), data.size(), (float*)output.data(), m_max_frame_size, 0);

    if (frame_size < 0)
    {
        emit error(QString("Opus decoder failed: %0").arg(opus_strerror(frame_size)));
        return;
    }

    int size = sizeof(float) * m_channels * frame_size;

    /* Write the decoded audio to output. */
    output.resize(size);

    emit decoded(output);
}

void OpusDecoderClass::write(const QByteArray &data)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));
}
