#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QtCore>
#include <QtMultimedia>

class AudioRecorder : public QFile
{
    Q_OBJECT
public:
    explicit AudioRecorder(const QString &name, const QAudioFormat &format, QObject *parent);
    ~AudioRecorder();

public slots:
    bool open();
    void close();

private:
    void writeHeader();
    bool hasSupportedFormat();
    QAudioFormat format;
};

#endif // AUDIORECORDER_H
