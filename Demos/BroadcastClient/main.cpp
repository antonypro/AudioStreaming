#include "mainwindow.h"
#include <QApplication>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>
#include "keepandroidawake.h"
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    const char *name = "QT_USE_ANDROID_NATIVE_STYLE";
    if (qEnvironmentVariableIsSet(name))
        qunsetenv(name);
    qputenv(name, "1");
#endif

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

#ifdef Q_OS_ANDROID
    KeepAndroidAwake keep_awake;
    Q_UNUSED(keep_awake)
#endif
    return a.exec();
}
