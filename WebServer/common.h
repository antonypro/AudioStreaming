#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtNetwork>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define TIMEOUT 30*1000

extern QSettings *global_settings;

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

#define SETTONULLPTR(obj) QObject::connect(obj, &QObject::destroyed, [&]{obj = nullptr;})

#define NEGOTIATION_STRING QByteArray("WalkieTalkieTCPDemo").leftJustified(128, char(0), true)

typedef struct PeerData {
    QByteArray data;
    QHostAddress host;
    qintptr descriptor;
} PeerData;

namespace ServerCommand
{
enum
{
    PeerInfo,
    PeerTryConnect,
    ConnectionRequested,
    ConnectionAnswer,
    LoggedIn,
    ConnectedToPeer,
    DisconnectedFromPeer,
    P2PData,
    Alive,
    Warning
};
}

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

static inline void setStdinEcho(bool enable)
{
#ifdef Q_OS_WIN
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);

    if (!enable)
        mode &= ~ENABLE_ECHO_INPUT;
    else
        mode |= ENABLE_ECHO_INPUT;

    SetConsoleMode(hStdin, mode);
#else
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if( !enable )
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

static inline const QByteArray getPassword(const char *display_msg)
{
    QByteArray password;

    printf("%s ", display_msg);

    setStdinEcho(false);

    forever
    {
        char c = getc(stdin);

        if (c == '\n' || c == EOF)
            break;

        password.append(c);

        c = 0;
    }

    setStdinEcho(true);

    printf("\n");
    fflush(stdout);

    return password;
}

#endif // COMMON_H
