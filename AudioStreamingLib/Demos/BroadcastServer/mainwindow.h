#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "common.h"
#ifndef Q_OS_ANDROID
#include "ffmpegclass.h"
#endif
#include "spectrumanalyzer.h"
#include "barswidget.h"
#include "waveformwidget.h"
#include "levelwidget.h"
#include "audiorecorder.h"
#include "mp3recorder.h"
#ifdef Q_OS_WIN
#include "qwinloopback.h"
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void initWidgets();
    void start();
    void loopbackdata(const QByteArray &data);
    void ffmpegdata(const QByteArray &data);
    void ffmpegplay();
    void ffmpegpause();
    void ffmpegstop();
    void ffmpegallfinished();
    void process(const QByteArray &data);
    void adjustSettings();
    void setRecordPath();
    void startPauseRecord();
    void stopRecord();
    void resetRecordPage();
    void writeToFile(const QByteArray &data);
    void updateConnections();
    void boxListenInputClicked(bool checked);
    void volumeChanged(int volume);
    void warning(const QString &warning);
    void error(const QString &error);
    void finished();
    void deviceTypeChanged();
    void currentIndexChanged(int index);
    void getDevInfo();

private:
    QPointer<AudioStreamingLibCore> m_audio_lib;

    QPointer<SpectrumAnalyzer> m_spectrum_analyzer;

#ifdef Q_OS_WIN
    QPointer<QWinLoopback> m_loopback;
#endif

    QByteArray m_buffer;

    QGroupBox *groupboxtype;

    QRadioButton *buttonloopbackaudioinput;

    QRadioButton *buttonffmpegaudioinput;

    QRadioButton *buttonaudioinput;

    QComboBox *comboboxaudioinput;

    QTabWidget *tabwidget;

    QListWidget *listconnections;

    QCheckBox *boxlisteninput;
    QLineEdit *lineport;
    QLineEdit *linemaxconnections;
    QPushButton *buttonstart;
    QLineEdit *linesamplerate;
    QLineEdit *linechannels;
    QLineEdit *lineid;
    QLineEdit *linepassword;
    QLabel *labelvolume;
    QSlider *slidervolume;
    QPlainTextEdit *texteditsettings;
    BarsWidget *bars;
    WaveFormWidget *waveform;
    LevelWidget *level;

#ifndef Q_OS_ANDROID
    FFMPEGClass *m_ffmpeg;
#endif

    bool m_ffmpeg_playing;
    bool m_ffmpeg_terminating;

    QLineEdit *linerecordpath;
    QPushButton *buttonsearch;
    QPushButton *buttonrecord;
    QPushButton *buttonrecordstop;
    QLCDNumber *lcdtime;
    QCheckBox *boxautostart;

    QPlainTextEdit *texteditlog;
    QCheckBox *boxlogtowidget;
    QPushButton *buttonclearlog;

    QPointer<AudioRecorder> m_audio_recorder;
    QPointer<MP3Recorder> m_audio_recorder_mp3;
    QAudioFormat m_format;
    qint64 m_total_size;
    bool m_paused;
};

#endif // MAINWINDOW_H
