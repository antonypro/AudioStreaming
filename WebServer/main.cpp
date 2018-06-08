#include <QtCore>
#include <signal.h>
#include "sslserver.h"
#include "common.h"

QFile *log_file = nullptr;
QSettings *global_settings = nullptr;

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)

    printf("%s\n", qPrintable(msg));
    fflush(stdout);

    if (log_file)
    {
        log_file->write(msg.toLatin1());
        log_file->write("\n");
        log_file->flush();
    }
}

void signalHandler(int signum)
{
    Q_UNUSED(signum)

    qApp->exit();
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    signal(SIGBREAK, signalHandler);
#else
    signal(SIGHUP, signalHandler);
#endif
    signal(SIGINT, signalHandler);

    qInstallMessageHandler(messageOutput);

    QCoreApplication a(argc, argv);

    if (!QDir("../data/").exists())
    {
        if (!QDir().mkdir("../data/"))
        {
            DEBUG_FUNCTION("Cant open data folder! Exiting...");
            return 0;
        }
    }

    QFile file("../data/log.txt");

    log_file = &file;

    SETTONULLPTR(log_file);

    if (!log_file->open(QFile::WriteOnly | QFile::Text | QFile::Append))
    {
        qInstallMessageHandler(nullptr);
        qDebug() << "Couldn't log to file!";
    }

    QSettings settings("../data/settings.ini", QSettings::IniFormat);

    global_settings = &settings;

    SETTONULLPTR(global_settings);

    DEBUG_FUNCTION("Session started!");

    SslServer server;
    Q_UNUSED(server)

    return a.exec();
}
