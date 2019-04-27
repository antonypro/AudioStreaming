#ifndef MP3RECORDER_H
#define MP3RECORDER_H

#include <QtCore>
#include <QtMultimedia>
#include <lame.h>

#define MP3_SIZE 8192

class MP3Recorder : public QObject
{
    Q_OBJECT
public:
    explicit MP3Recorder(QObject *parent = nullptr);
    ~MP3Recorder();

public slots:
    bool start(const QString &mp3_path, const QAudioFormat &mp3_format, int mp3_bitrate, const QString &title = QString());
    void encode(const QByteArray &data);
    void stop();

private:
    //Functions
    bool loadFunction();

    //Variables
    typedef lame_global_flags*(CDECL *tlame_init)(void);
    tlame_init plame_init;

    typedef int(CDECL *tlame_set_in_samplerate)(lame_global_flags*, int);
    tlame_set_in_samplerate plame_set_in_samplerate;

    typedef int(CDECL *tlame_set_VBR)(lame_global_flags*, int);
    tlame_set_VBR plame_set_VBR;

    typedef int(CDECL *tlame_set_brate)(lame_global_flags*, int);
    tlame_set_brate plame_set_brate;

    typedef int(CDECL *tlame_set_num_channels)(lame_global_flags*, int);
    tlame_set_num_channels plame_set_num_channels;

    typedef int(CDECL *tlame_init_params)(lame_global_flags*);
    tlame_init_params plame_init_params;

    typedef int(CDECL *tlame_encode_flush)(lame_global_flags*, unsigned char*, int);
    tlame_encode_flush plame_encode_flush;

    typedef int(CDECL *tlame_encode_buffer_interleaved)(lame_global_flags*, short int[], int, unsigned char*, int);
    tlame_encode_buffer_interleaved plame_encode_buffer_interleaved;

    typedef int(CDECL *tlame_encode_buffer)(lame_global_flags*, const short int[], const short int[], const int, unsigned char*, const int);
    tlame_encode_buffer plame_encode_buffer;

    typedef int(CDECL *tlame_close)(lame_global_flags*);
    tlame_close plame_close;

    typedef void(CDECL *tid3tag_init)(lame_t gfp);
    tid3tag_init pid3tag_init;

    typedef void(CDECL *tid3tag_v2_only)(lame_t gfp);
    tid3tag_v2_only pid3tag_v2_only;

    typedef void(CDECL *tid3tag_set_title)(lame_t gfp, const char* title);
    tid3tag_set_title pid3tag_set_title;

    lame_t m_lame;

    int CHANNELS;
    int SAMPLE_RATE;
    int BITRATE;

    QFile *m_mp3_file;

    uchar mp3_buffer[MP3_SIZE];

    QAudioFormat m_mp3_format;

    int m_mp3_bitrate;
};

#endif // MP3RECORDER_H
