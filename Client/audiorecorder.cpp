#include "audiorecorder.h"

AudioRecorder::AudioRecorder(const QString &name, const QAudioFormat &format, QObject *parent) : QFile(name, parent), format(format)
{

}

AudioRecorder::~AudioRecorder()
{
    if (!isOpen())
        return;

    close();
}

bool AudioRecorder::hasSupportedFormat()
{
    return (format.sampleSize() == 8
            && format.sampleType() == QAudioFormat::UnSignedInt)
            || (format.sampleSize() > 8
                && format.sampleType() == QAudioFormat::SignedInt
                && format.byteOrder() == QAudioFormat::LittleEndian);
}

bool AudioRecorder::open()
{
    if (!hasSupportedFormat())
    {
        setErrorString("Wav PCM supports only 8-bit unsigned samples "
                       "or 16-bit (or more) signed samples (in little endian)");
        return false;
    }
    else
    {
        if (!QFile::open(ReadWrite | Truncate))
            return false;
        writeHeader();
        return true;
    }
}

void AudioRecorder::writeHeader()
{
    QDataStream out(this);
    out.setByteOrder(QDataStream::LittleEndian);

    // RIFF chunk
    out.writeRawData("RIFF", 4);
    out << quint32(0); // Placeholder for the RIFF chunk size (filled by close())
    out.writeRawData("WAVE", 4);

    // Format description chunk
    out.writeRawData("fmt ", 4);
    out << quint32(16); // "fmt " chunk size (always 16 for PCM)
    out << quint16(1); // data format (1 => PCM)
    out << quint16(format.channelCount());
    out << quint32(format.sampleRate());
    out << quint32(format.sampleRate() * format.channelCount()
                   * format.sampleSize() / 8 ); // bytes per second
    out << quint16(format.channelCount() * format.sampleSize() / 8); // Block align
    out << quint16(format.sampleSize()); // Significant Bits Per Sample

    // Data chunk
    out.writeRawData("data", 4);
    out << quint32(0); // Placeholder for the data chunk size (filled by close())

    Q_ASSERT(pos() == 44); // Must be 44 for WAV PCM
}

void AudioRecorder::close()
{
    // Fill the header size placeholders
    quint32 fileSize = size();

    QDataStream out(this);
    // Set the same ByteOrder like in writeHeader()
    out.setByteOrder(QDataStream::LittleEndian);
    // RIFF chunk size
    seek(4);
    out << quint32(fileSize - 8);

    // data chunk size
    seek(40);
    out << quint32(fileSize - 44);

    QFile::close();
}
