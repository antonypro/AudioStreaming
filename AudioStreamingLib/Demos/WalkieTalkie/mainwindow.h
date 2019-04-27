#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "common.h"
#include "settings.h"
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
    void webClient(const QString &username, const QString &password, bool unavailable = false,
                   bool new_user = false, const QString &code = QString());
    void client();
    void server();
    void sliderVolumeValueChanged(int value);
    void webClientStarted(bool enable);
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
    void connectedToServer(const QByteArray &hash);
    void connectToPeer();
    void webClientConnected(const QHostAddress &address, const QString &id);
    void clientConnected(const QHostAddress &address, const QString &id);
    void serverConnected(const QHostAddress &address, const QString &id);
    void webClientDisconnected();
    void clientDisconnected();
    void serverDisconnected();
    void pending(const QHostAddress &address, const QString &id);
    void webClientLoggedIn();
    void webClientWarning(const QString &message);
    void writeText();
    void receiveText(const QByteArray &data);
    void finished();
    void getDevInfo();

private:
    QString m_peer;
    bool m_connecting_connected_to_peer;

    QPointer<AudioStreamingLibCore> m_audio_lib;
    QPointer<AudioStreamingLibCore> m_discover_instance;

    Settings *websettings;

    QListWidget *listwidgetpeers;

    LevelWidget *levelinput;
    LevelWidget *leveloutput;

    QTabWidget *tabwidget;

    QComboBox *comboboxaudioinput;
    QComboBox *comboboxaudiooutput;

    QPushButton *buttonconnecttopeer;
    QLabel *labelservertweb;
    QLineEdit *lineserverweb;
    QLabel *labelportweb;
    QLineEdit *lineportweb;
    QLabel *labelsamplerateweb;
    QLineEdit *linesamplerateweb;
    QLabel *labelchannelsweb;
    QLineEdit *linechannelsweb;
    QLabel *labelbuffertimeweb;
    QLineEdit *linebuffertimeweb;

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

    QLabel *labelwebid;
    QLabel *labelclientid;
    QLabel *labelserverid;

    QLabel *labelwebcode;
    QLabel *labelwebpassword;
    QLabel *labelclientpassword;
    QLabel *labelserverpassword;

    QLineEdit *linewebid;
    QLineEdit *lineclientid;
    QLineEdit *lineserverid;

    QLineEdit *linewebpassword;
    QLineEdit *lineclientpassword;
    QLineEdit *lineserverpassword;

    QLineEdit *linewebcode;

    QPushButton *buttonsigninweb;
    QPushButton *buttonsignupweb;
    QPushButton *buttonconnect;
    QPushButton *buttonstartserver;

    QWidget *widget1;
    QWidget *widget2;
    QWidget *widget3;

    QScrollArea *scrollclientserver;
    QWidget *widgetchat;

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

    QByteArray m_local_audio;
    QByteArray m_peer_audio;

    bool m_msgbox_visible;

    QByteArray m_web_login_xml;
};

#endif // MAINWINDOW_H
