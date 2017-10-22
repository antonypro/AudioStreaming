#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "levelwidget.h"
#include "audiorecorder.h"

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
    void client();
    void server();
    void sliderVolumeValueChanged(int value);
    void getDevInfo();
    void clientStarted(bool enable);
    void serverStarted(bool enable);
    void error(const QString &error);
    void adjustSettings();
    void setRecordPath();
    void startPauseRecord();
    void stopRecord();
    void resetRecordPage();
    void writeLocalToBuffer(const QByteArray &data);
    void writePeerToBuffer(const QByteArray &data);
    void mixLocalPeer();
    void clientConnected(const QHostAddress &address, const QString &id);
    void serverConnected(const QHostAddress &address, const QString &id);
    void clientDisconnected();
    void serverDisconnected();
    void pending(const QHostAddress &address, const QString &id);
    void writeText();
    void receiveText(const QByteArray &data);
    void finished();

private:
    QString m_peer;

    AudioStreamingLibCore *m_audio_lib;
    AudioStreamingLibCore *m_discover_instance;

    QListWidget *listwidgetpeers;

    LevelWidget *levelinput;
    LevelWidget *leveloutput;

    QTabWidget *tabwidget;

    QComboBox *comboboxaudioinput;
    QLabel *labelhost;
    QLineEdit *linehost;
    QLabel *labelport;
    QLineEdit *lineport;
    QPlainTextEdit *texteditsettings;
    QCheckBox *boxinputmuted;
    QLabel *labelvolume;
    QSlider *slidervolume;

    QLabel *labelportserver;
    QLineEdit *lineportserver;
    QLabel *labelsamplerate;
    QLineEdit *linesamplerate;
    QLabel *labelchannels;
    QLineEdit *linechannels;
    QLabel *labelbuffertime;
    QLineEdit *linebuffertime;

    QPlainTextEdit *texteditchat;
    QLineEdit *linechat;
    QPushButton *buttonsendchat;

    QLabel *labelclientid;
    QLabel *labelserverid;

    QLabel *labelclientpassword;
    QLabel *labelserverpassword;

    QLineEdit *lineclientid;
    QLineEdit *lineserverid;

    QLineEdit *lineclientpassword;
    QLineEdit *lineserverpassword;

    QPushButton *buttonconnect;
    QPushButton *buttonstartserver;

    QWidget *widget1;
    QWidget *widget2;

    QScrollArea *scrollclientserver;
    QWidget *widgetchat;

    QLineEdit *linerecordpath;
    QPushButton *buttonsearch;
    QPushButton *buttonrecord;
    QPushButton *buttonrecordstop;
    QLCDNumber *lcdtime;

    AudioRecorder *m_audio_recorder;
    QAudioFormat m_format;
    bool m_paused;

    QByteArray m_local_audio;
    QByteArray m_peer_audio;
};

#endif // MAINWINDOW_H
