#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "common.h"
#include "spectrumanalyzer.h"
#include "barswidget.h"
#include "waveformwidget.h"
#include "levelwidget.h"
#include "audiorecorder.h"
#include "mp3recorder.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void stoprequest();

private slots:
    void initWidgets();
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
    QPointer<AudioStreamingLibCore> m_audio_lib;
    QPointer<AudioStreamingLibCore> m_discover_instance;

    QPointer<SpectrumAnalyzer> m_spectrum_analyzer;

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

    QPointer<AudioRecorder> m_audio_recorder;
    QPointer<MP3Recorder> m_audio_recorder_mp3;
    QAudioFormat m_format;
    qint64 m_total_size;
    bool m_paused;
};

#endif // MAINWINDOW_H
