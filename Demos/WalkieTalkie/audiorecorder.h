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

    using QFile::open;

public slots:
    bool open();
    qint64 write(const QByteArray &data);
    void close();

private:
    void writeHeader();
    bool hasSupportedFormat();
    QAudioFormat format;
};

#endif // AUDIORECORDER_H
