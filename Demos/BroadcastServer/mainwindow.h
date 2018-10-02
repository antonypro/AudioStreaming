#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "spectrumanalyzer.h"
#include "barswidget.h"
#include "waveformwidget.h"
#include "levelwidget.h"
#include "audiorecorder.h"
#ifdef Q_OS_WIN
#include "qwinloopback.h"
#endif

static inline int msgBoxQuestion(const QString &title, const QString &message, QWidget *parent = nullptr)
{
    QMessageBox msgbox(parent);

#ifdef Q_OS_MACOS
    msgbox.setWindowModality(Qt::WindowModal);
#endif
    msgbox.setIcon(QMessageBox::Question);
    msgbox.setStandardButtons(QMessageBox::Yes);
    msgbox.addButton(QMessageBox::No);
    msgbox.setDefaultButton(QMessageBox::Yes);
    msgbox.setWindowTitle(title);
    msgbox.setText(message);

    return msgbox.exec();
}

static inline void msgBoxWarning(const QString &title, const QString &message, QWidget *parent = nullptr)
{
    QMessageBox msgbox(parent);

#ifdef Q_OS_MACOS
    msgbox.setWindowModality(Qt::WindowModal);
#endif
    msgbox.setIcon(QMessageBox::Warning);
    msgbox.setWindowTitle(title);
    msgbox.setText(message);

    msgbox.exec();
}

static inline void msgBoxCritical(const QString &title, const QString &message, QWidget *parent = nullptr)
{
    QMessageBox msgbox(parent);

#ifdef Q_OS_MACOS
    msgbox.setWindowModality(Qt::WindowModal);
#endif
    msgbox.setIcon(QMessageBox::Critical);
    msgbox.setWindowTitle(title);
    msgbox.setText(message);

    msgbox.exec();
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void start();
    void loopbackdata(const QByteArray &data);
    void process(const QByteArray &data);
    void adjustSettings();
    void setRecordPath();
    void startPauseRecord();
    void stopRecord();
    void resetRecordPage();
    void writeToFile(const QByteArray &data);
    void updateConnections();
    void error(const QString &error);
    void finished();
    void currentIndexChanged(int index);
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
    QLineEdit *linesamplerate;
    QLineEdit *linechannels;
    QLineEdit *lineid;
    QLineEdit *linepassword;
    QPlainTextEdit *texteditsettings;
    BarsWidget *bars;
    WaveFormWidget *waveform;
    LevelWidget *level;

    QLineEdit *linerecordpath;
    QPushButton *buttonsearch;
    QPushButton *buttonrecord;
    QPushButton *buttonrecordstop;
    QLCDNumber *lcdtime;
    QCheckBox *boxautostart;

    QPlainTextEdit *texteditlog;

    AudioRecorder *m_audio_recorder;
    QAudioFormat m_format;
    bool m_paused;
};

#endif // MAINWINDOW_H
