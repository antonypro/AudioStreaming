#ifndef OPUSENCODE_H
#define OPUSENCODE_H

#include <QtCore>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <opus.h>
#include <stdio.h>

/*The frame size is hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 960
#define APPLICATION OPUS_APPLICATION_VOIP
#define BITRATE 64000

#define MAX_FRAME_SIZE 6*960
#define MAX_PACKET_SIZE (3*1276)

class OpusEncode : public QObject
{
    Q_OBJECT
public:
    explicit OpusEncode(QObject *parent = 0);
    ~OpusEncode();

signals:

public slots:
    void start(int CHANNELS, int SAMPLE_RATE);
    QByteArray encode(const QByteArray &data);

private:
    /*Holds the state of the encoder and decoder */
    OpusEncoder *encoder;

    int channels;

    bool initialized;
};

#endif // OPUSENCODE_H
