#ifndef COMMON_H
#define COMMON_H

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wattributes"
#endif

#include <QtCore>
#include <QtMultimedia>
#include "audiostreaminglibcore.h"
#include "audiocommon.h"

#if Q_BYTE_ORDER != Q_LITTLE_ENDIAN
#error "Only little endian machines are supported"
#endif

#define TIMEOUT 30*1000

#define AES_PADDING 32

#define MAX_PENDING_BEATS 50

#define MAX_NETWORK_CHUNK_SIZE 10*1024*1024

#if defined(IS_TO_DEBUG_VERBOSE_1) || defined(IS_TO_DEBUG_VERBOSE_2)
#define IS_TO_DEBUG

extern QMutex record_mutex;
extern qint64 record_count;

#define DEBUG_FUNCTION(message)\
{\
    record_mutex.lock();\
    \
    qDebug()\
    << "Date and time(UTC):" << qPrintable(QDateTime::currentDateTimeUtc().toString("yyyy/MM/dd hh:mm:ss"))\
    << "\nFile:" << __FILE__\
    << "\nLine:" << __LINE__\
    << "\nFunction:" << __FUNCTION__\
    << "\nIndex:" << ++record_count;\
    \
    qDebug()\
    << "Message:"\
    << message\
    << "\n";\
    \
    record_mutex.unlock();\
}

#ifdef IS_TO_DEBUG_VERBOSE_1

#define LIB_DEBUG_LEVEL_1(message)\
    DEBUG_FUNCTION(message)
#define LIB_DEBUG_LEVEL_2(message) //Does nothing

#elif defined(IS_TO_DEBUG_VERBOSE_2) // IS_TO_DEBUG_VERBOSE_1

#define LIB_DEBUG_LEVEL_1(message)\
    DEBUG_FUNCTION(message)
#define LIB_DEBUG_LEVEL_2(message)\
    DEBUG_FUNCTION(message)

#endif // IS_TO_DEBUG_VERBOSE_1

#else //IS_TO_DEBUG

#define LIB_DEBUG_LEVEL_1(message) //Does nothing
#define LIB_DEBUG_LEVEL_2(message) //Does nothing

#endif //IS_TO_DEBUG

#ifdef OPUS

#define BUFFER_LEN 1024*1024 //Used by resampler

#define MAX_PACKET_SIZE 3*1276

#endif //OPUS

/*** THREAD BEGIN ***/

extern int m_worker_count;
extern QMutex m_worker_mutex;
extern QSemaphore m_worker_semaphore;

#define START_THREAD \
{\
    setParent(nullptr);\
    QThread *new_thread = new QThread();\
    moveToThread(new_thread);\
    \
    m_worker_mutex.lock();\
    m_worker_count++;\
    m_worker_mutex.unlock();\
    connect(parent, &QObject::destroyed, this, &QObject::deleteLater);\
    \
    new_thread->start();\
}

#define STOP_THREAD \
{\
    m_worker_mutex.lock();\
    bool empty = (--m_worker_count == 0);\
    m_worker_mutex.unlock();\
    if (empty) \
    {\
        m_worker_semaphore.release();\
    }\
}

/*** THREAD END ***/

//\cond HIDDEN_SYMBOLS
typedef struct PeerData {
    QByteArray data;
    QHostAddress host;
    qintptr descriptor;
} PeerData;

namespace Command
{
enum
{
    AudioHeader,
    AudioData,
    HeartBeat,
    ExtraData,
    ExtraDataReceived
};
}

namespace ServerCommand
{
enum
{
    PeerTryConnect,
    ConnectionRequested,
    ConnectionAnswer,
    LoggedIn,
    ConnectedToPeer,
    DisconnectedFromPeer,
    P2PData,
    Alive,
    Warning,
    XML
};
}
//\endcond

static inline QString cleanString(const QString &s)
{
    QString diacriticLetters;
    QStringList noDiacriticLetters;
    QStringList acceptedCharacters;

    diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
    noDiacriticLetters << "S"<<"OE"<<"Z"<<"s"<<"oe"<<"z"<<"Y"<<"Y"<<"u"<<"A"<<"A"<<"A"<<"A"<<"A"<<"A"<<"AE"<<"C"
                       <<"E"<<"E"<<"E"<<"E"<<"I"<<"I"<<"I"<<"I"<<"D"<<"N"<<"O"<<"O"<<"O"<<"O"<<"O"<<"O"<<"U"<<"U"
                      <<"U"<<"U"<<"Y"<<"s"<<"a"<<"a"<<"a"<<"a"<<"a"<<"a"<<"ae"<<"c"<<"e"<<"e"<<"e"<<"e"<<"i"<<"i"
                     <<"i"<<"i"<<"o"<<"n"<<"o"<<"o"<<"o"<<"o"<<"o"<<"o"<<"u"<<"u"<<"u"<<"u"<<"y"<<"y";

    acceptedCharacters << "0" << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9"
                       << "a" << "b" << "c" << "d" << "e" << "f" << "g" << "h" << "i" << "j"
                       << "k" << "l" << "m" << "n" << "o" << "p" << "q" << "r" << "s" << "t"
                       << "u" << "v" << "w" << "x" << "y" << "z";

    QString output_tmp;

    for (int i = 0; i < s.length(); i++)
    {
        QChar c = s[i];
        int dIndex = diacriticLetters.indexOf(c);
        if (dIndex < 0)
        {
            output_tmp.append(c);
        }
        else
        {
            QString replacement = noDiacriticLetters[dIndex];
            output_tmp.append(replacement);
        }
    }

    output_tmp = output_tmp.toLower();

    QString output;

    for (int i = 0; i < output_tmp.length(); i++)
    {
        QChar c = output_tmp[i];

        if (acceptedCharacters.contains(c))
            output.append(c);
    }

    return output;
}

template <typename T>
static inline QByteArray getBytes(T input)
{
    QByteArray tmp;
    QDataStream data(&tmp, QIODevice::WriteOnly);
    data << input;
    return tmp;
}

template <typename T>
static inline T getValue(QByteArray bytes)
{
    T tmp;
    QDataStream data(&bytes, QIODevice::ReadOnly);
    data >> tmp;
    return tmp;
}

#endif // COMMON_H
