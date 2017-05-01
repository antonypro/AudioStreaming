#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "spectrumanalyzer.h"
#include "bars.h"
#include "levelwidget.h"
#ifdef Q_OS_WIN
#include "qwinloopback.h"
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void start();
    void loopbackdata(const QByteArray &data);
    void process(const QByteArray &data);
    void adjustSettings();
    void updateConnections();
    void error(const QString &error);
    void finished();
    void getDevInfo();

private:
    AudioStreamingLibCore *m_audio_lib;

    SpectrumAnalyzer *m_spectrum_analyzer;

#ifdef Q_OS_WIN
    QWinLoopback *loopback;
    QByteArray buffer;
#endif
    QComboBox *comboboxaudioinput;

    QTabWidget *tabwidget;

    QListWidget *listconnections;

    QLineEdit *lineport;
    QLineEdit *linemaxconnections;
    QPushButton *buttonstart;
    QLineEdit *lineid;
    QLineEdit *linepassword;
    QPlainTextEdit *texteditsettings;
    Bars *bars;
    LevelWidget *level;
};

#endif // MAINWINDOW_H
