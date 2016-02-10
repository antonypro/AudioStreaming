#ifndef OPUSDECODE_H
#define OPUSDECODE_H

#include <QtCore>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <opus.h>
#include <stdio.h>

#define FRAME_SIZE 960
#define MAX_FRAME_SIZE 6*960

class OpusDecode : public QObject
{
    Q_OBJECT
public:
    explicit OpusDecode(QObject *parent = 0);
    ~OpusDecode();

signals:

public slots:
    void start(int CHANNELS, int SAMPLE_RATE);
    QByteArray decode(const QByteArray &data);

private:
    /*Holds the state of the encoder and decoder */
    OpusDecoder *decoder;
    int channels;

    bool initialized;
};

#endif // OPUSDECODE_H
