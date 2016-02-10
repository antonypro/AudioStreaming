#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QtCore>
#include <QtMultimedia>

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(const QAudioFormat &format, uint timetobuffer, QObject *parent = 0);

signals:
    void currentlevel(float);

public slots:
    void writeData(const QByteArray &data);
    void setVolume(int vol);

private slots:
    void read();

private:
    QAudioOutput *audio;
    QIODevice *device;
    QByteArray buffer;
    bool bufferrequested;
    bool readcalled;
    bool issigned;
    int samplesize;
    int volume;
    int sizetobuffer;
    QAudioFormat f;
    QElapsedTimer time;
    double savedlevel;
    double level;
};

#endif // AUDIOOUTPUT_H
