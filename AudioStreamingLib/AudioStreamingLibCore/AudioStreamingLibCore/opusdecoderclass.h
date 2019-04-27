#ifndef OPUSDECODERCLASS_H
#define OPUSDECODERCLASS_H

#include <QtCore>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <opus.h>
#include <stdio.h>
#include "common.h"

//\cond HIDDEN_SYMBOLS
class OpusDecoderClass : public QObject
{
    Q_OBJECT
public:
    explicit OpusDecoderClass(QObject *parent = nullptr);
    ~OpusDecoderClass();

signals:
    void decoded(QByteArray);
    void error(QString);

public slots:
    void start(int sample_rate, int channels, int frame_size, int max_frame_size);
    void write(const QByteArray &data);

private slots:
    void startPrivate(int sample_rate, int channels, int frame_size, int max_frame_size);
    void writePrivate(const QByteArray &data);

private:
    /*Holds the state of the encoder and decoder */
    OpusDecoder *m_decoder;

    int m_channels;
    int m_frame_size;
    int m_max_frame_size;

    bool m_initialized;
};
//\endcond

#endif // OPUSDECODERCLASS_H
