#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QtCore>
#include <QtMultimedia>

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = 0);

signals:

public slots:
    void writeData(QByteArray data);

private:
    QAudioOutput *audio;
    QIODevice *device;
};

#endif // AUDIOOUTPUT_H
