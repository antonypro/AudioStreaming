#include <QtCore>
#include <AudioStreamingLibCore>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("BroadcastClientDemoConsole");

    QCommandLineParser parser;

    QCommandLineOption address = QCommandLineOption(QStringList() << "a" << "address",
                                                    "Set the address to connect to <addr>",
                                                    "addr");

    QCommandLineOption port = QCommandLineOption(QStringList() << "p" << "port",
                                                 "Set the address port to connect to <port>, "
                                                 "default is 1024",
                                                 "port", "1024");

    QCommandLineOption time_to_buffer = QCommandLineOption(QStringList() << "time-to-buffer",
                                                           "Set the time to buffer in milliseconds to <time>, "
                                                           "default is 0 (Smart buffer)",
                                                           "time", "0");

    QCommandLineOption password = QCommandLineOption(QStringList() << "password",
                                                     "Enable encryption with the password <pass> "
                                                     "(Not secure in console), default is no password",
                                                     "pass");

    QCommandLineOption help = parser.addHelpOption();
    parser.setApplicationDescription("\nBroadcast client demo console is a simple example\n"
                                     "on how to use the AudioStreamingLib with minimal code,\n"
                                     "on envinronments with no graphical interface and/or in automated mode!");

    parser.addOption(address);
    parser.addOption(port);
    parser.addOption(time_to_buffer);
    parser.addOption(password);

    if (!parser.parse(QCoreApplication::arguments()))
    {
        qDebug() << qPrintable(parser.errorText());
        return 1;
    }

    if (parser.isSet(help))
        parser.showHelp(0);

    if (!parser.isSet(address))
        parser.showHelp(1);

    AudioStreamingLibCore audio_lib;

    AudioStreamingLibInfo info;

    info.setWorkMode(AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient);
    info.setTimeToBuffer(parser.value(time_to_buffer).toInt());
    info.setEncryptionEnabled(!parser.value(password).isEmpty());
    info.setNegotiationString("BroadcastTCPDemo");

    QObject::connect(&audio_lib, &AudioStreamingLibCore::error, &a, [&](const QString &error){
        qDebug() << "Error:" << qPrintable(error) << "\n";
        QCoreApplication::exit(1);
    });

    QObject::connect(&audio_lib, &AudioStreamingLibCore::connected, &a, [&](const QHostAddress &host_address){
        qDebug() << "Connected to:" << qPrintable(host_address.toString()) << "\n";
    });

    QObject::connect(&audio_lib, &AudioStreamingLibCore::disconnected, &a, [&](const QHostAddress &host_address){
        qDebug() << "Disconnected from:" << qPrintable(host_address.toString()) << "\n";
    });

    QObject::connect(&audio_lib, &AudioStreamingLibCore::finished, &a, [&]{
        qDebug() << "AudioStreamingLib finished, exiting..." << "\n";
        QCoreApplication::exit(0);
    });

    audio_lib.start(info);

    audio_lib.setVolume(100);

    qDebug() << "Connecting to:" << qPrintable(parser.value(address))
             << "on port:" << parser.value(port).toInt() << "..." << "\n";

    audio_lib.connectToHost(parser.value(address),
                            quint16(parser.value(port).toInt()),
                            parser.value(password).toLatin1());

    return a.exec();
}
