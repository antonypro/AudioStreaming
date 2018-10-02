#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "levelwidget.h"
#include "audiorecorder.h"

static inline QString cleanString(const QString &s)
{
    QString diacriticLetters;
    QStringList noDiacriticLetters;
    QStringList acceptedCharacters;

    diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
    noDiacriticLetters << "S"<<"OE"<<"Z"<<"s"<<"oe"<<"z"<<"Y"<<"Y"<<"u"<<"A"<<"A"<<"A"<<"A"<<"A"<<"A"<<"AE"<<"C"
                        <<"E"<<"E"<<"E"<<"E"<<"I"<<"I"<<"I"<<"I"<<"D"<<"N"<<"O"<<"O"<<"O"<<"O"<<"O"<<"O"<<"U"<<"U"
                       <<"U"<<"U"<<"Y"<<"s"<<"a"<<"a"<<"a"<<"a"<<"a"<<"a"<<"ae"<<"c"<<"e"<<"e"<<"e"<<"e"<<"i"<<"i"
                      <<"i"<<"i"<<"o"<<"n"<<"o"<<"o"<<"o"<<"o"<<"o"<<"o"<<"u"<<"u"<<"u"<<"u"<<"y"<<"y";

    acceptedCharacters << "0" << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9"
                       << "a" << "b" << "c" << "d" << "e" << "f" << "g" << "h" << "i" << "j"
                       << "k" << "l" << "m" << "n" << "o" << "p" << "q" << "r" << "s" << "t"
                       << "u" << "v" << "w" << "x" << "y" << "z";

    QString output_tmp;

    for (int i = 0; i < s.length(); i++)
    {
        QChar c = s[i];
        int dIndex = diacriticLetters.indexOf(c);
        if (dIndex < 0)
        {
            output_tmp.append(c);
        }
        else
        {
            QString replacement = noDiacriticLetters[dIndex];
            output_tmp.append(replacement);
        }
    }

    output_tmp = output_tmp.toLower();

    QString output;

    for (int i = 0; i < output_tmp.length(); i++)
    {
        QChar c = output_tmp[i];

        if (acceptedCharacters.contains(c))
            output.append(c);
    }

    return output;
}

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
    void webClient();
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

    AudioStreamingLibCore *m_audio_lib;
    AudioStreamingLibCore *m_discover_instance;

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

    QLabel *labelwebpassword;
    QLabel *labelclientpassword;
    QLabel *labelserverpassword;

    QLineEdit *linewebid;
    QLineEdit *lineclientid;
    QLineEdit *lineserverid;

    QLineEdit *linewebpassword;
    QLineEdit *lineclientpassword;
    QLineEdit *lineserverpassword;

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

    AudioRecorder *m_audio_recorder;
    QAudioFormat m_format;
    bool m_paused;

    QByteArray m_local_audio;
    QByteArray m_peer_audio;

    bool m_msgbox_visible;
};

#endif // MAINWINDOW_H
