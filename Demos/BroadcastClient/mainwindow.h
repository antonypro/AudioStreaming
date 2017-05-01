#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <AudioStreamingLibCore>
#include "spectrumanalyzer.h"
#include "bars.h"
#include "levelwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
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
    void volumeChanged(int volume);
    void error(const QString &error);
    void connected(const QHostAddress &address, const QString &id);
    void disconnected(const QHostAddress &address);
    void finished();

private:
    AudioStreamingLibCore *m_audio_lib;
    AudioStreamingLibCore *m_discover_instance;

    SpectrumAnalyzer *m_spectrum_analyzer;

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
    Bars *bars;
    LevelWidget *level;
};

#endif // MAINWINDOW_H
