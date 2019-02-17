#include "opusencoderclass.h"

//Opus encoder class

OpusEncoderClass::OpusEncoderClass(QObject *parent) : QObject(parent)
{
    m_initialized = false;

    START_THREAD
}

OpusEncoderClass::~OpusEncoderClass()
{
    if (m_initialized)
    {
        m_initialized = false;

        /*Destroy the encoder state*/
        opus_encoder_destroy(m_encoder);
    }

    STOP_THREAD
}

void OpusEncoderClass::startPrivate(int sample_rate, int channels, int bitrate, int frame_size, int application)
{
    if (m_initialized)
        return;

    int err;

    /*Create a new encoder state */
    m_encoder = opus_encoder_create(sample_rate, channels, application, &err);
    if (err < 0)
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
    if (err < 0)
    {
        emit error(QString("Failed to set Opus bitrate: %0").arg(opus_strerror(err)));
        return;
    }

    m_channels = channels;
    m_frame_size = frame_size;
}

void OpusEncoderClass::start(int sample_rate, int channels, int bitrate, int frame_size, int application)
{
    QMetaObject::invokeMethod(this, "startPrivate", Qt::QueuedConnection,
                              Q_ARG(int, sample_rate),
                              Q_ARG(int, channels),
                              Q_ARG(int, bitrate),
                              Q_ARG(int, frame_size),
                              Q_ARG(int, application));
}

void OpusEncoderClass::writePrivate(const QByteArray &data)
{
    m_buffer.append(data);

    forever
    {
        QByteArray result = encode();

        if (result.isEmpty())
            break;

        emit encoded(result);
    }
}

void OpusEncoderClass::write(const QByteArray &data)
{
    QMetaObject::invokeMethod(this, "writePrivate", Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));
}

QByteArray OpusEncoderClass::encode()
{
    if (!m_initialized)
        return QByteArray();

    int size = int(sizeof(float)) * m_channels * m_frame_size;

    if (m_buffer.size() < size)
        return QByteArray();

    int nbBytes;

    QByteArray input = m_buffer.mid(0, size);
    m_buffer.remove(0, size);

    QByteArray output = QByteArray(MAX_PACKET_SIZE, char(0));

    /* Encode the frame. */
    nbBytes = opus_encode_float(m_encoder, reinterpret_cast<const float*>(input.constData()), m_frame_size, reinterpret_cast<uchar*>(output.data()), MAX_PACKET_SIZE);

    if (nbBytes < 0)
    {
        emit error(QString("Opus encode failed: %0").arg(opus_strerror(nbBytes)));

        return QByteArray();
    }

    output.resize(nbBytes);

    return output;
}
