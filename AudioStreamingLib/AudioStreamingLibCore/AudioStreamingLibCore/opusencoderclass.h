#ifndef OPUSENCODERCLASS_H
#define OPUSENCODERCLASS_H

#include <QtCore>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <opus.h>
#include <stdio.h>
#include "common.h"

//\cond HIDDEN_SYMBOLS
class OpusEncoderClass : public QObject
{
    Q_OBJECT
public:
    explicit OpusEncoderClass(QObject *parent = nullptr);
    ~OpusEncoderClass();

signals:
    void encoded(QByteArray);
    void error(QString);

public slots:
    void start(int sample_rate, int channels, int bitrate, int frame_size, int application);
    void write(const QByteArray &data);

private slots:
    void startPrivate(int sample_rate, int channels, int bitrate, int frame_size, int application);
    void writePrivate(const QByteArray &data);

    QByteArray encode();

private:
    /*Holds the state of the encoder and decoder */
    OpusEncoder *m_encoder;

    int m_channels;

    int m_frame_size;

    bool m_initialized;

    QByteArray m_buffer;
};
//\endcond

#endif // OPUSENCODERCLASS_H
