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

signals:
    void stoprequest();

private slots:
    void currentChanged(int index);
    void startDiscover();
    void stopDiscover();
    void peerFound(const QHostAddress &address, const QString &id);
    void selectPeer(QListWidgetItem *item);
    void start();
    void adjustSettings();
    void setRecordPath();
    void startPauseRecord();
    void stopRecord();
    void resetRecordPage();
    void writeToFile(const QByteArray &data);
    void volumeChanged(int volume);
    void error(const QString &error);
    void connected(const QHostAddress &address, const QString &id);
    void disconnected(const QHostAddress &address);
    void finished();
    void getDevInfo();

private:
    AudioStreamingLibCore *m_audio_lib;
    AudioStreamingLibCore *m_discover_instance;

    SpectrumAnalyzer *m_spectrum_analyzer;

    QComboBox *comboboxaudiooutput;

    QLineEdit *linehost;
    QLineEdit *lineport;
    QLineEdit *linetime;
    QPushButton *buttonconnect;
    QLineEdit *linepassword;
    QTabWidget *tabwidget;
    QPlainTextEdit *texteditsettings;
    QListWidget *listwidgetpeers;
    QLabel *labelvolume;
    QSlider *slidervolume;
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
