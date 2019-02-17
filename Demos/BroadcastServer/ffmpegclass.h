#ifndef FFMPEGCLASS_H
#define FFMPEGCLASS_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtConcurrent>
#include "common.h"

class FFMPEGClass : public QWidget
{
    Q_OBJECT
public:
    explicit FFMPEGClass(QWidget *parent = nullptr);
    ~FFMPEGClass();

signals:
    void rawAudio(QByteArray);
    void decoded(qint64, qint64);
    void mediaPlay();
    void mediaPause();
    void mediaStop();
    void finished();
    void allFinished();

public slots:
    void read();
    void clearPlayList();
    void setReadingEnabled(bool enabled);

private slots:
    void initWidgets();
    void start();
    void readThread(const QString &path);
    bool testFFMPEG();
    void playPausePrivate();
    void stopPrivate();
    void addPrivate();
    void removePrivate();
    void doubleClickPrivate(const QModelIndex &index);
    void rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                   const QModelIndex &destinationParent, int destinationRow);
    void finishedPrivate();
    void decodedPrivate(qint64 decoded_ms, qint64 total_ms);

private:
    QPushButton *play_pause;
    QPushButton *stop;
    QLabel *info;
    QPushButton *add;
    QPushButton *remove;
    QListWidget *media_list;
    bool m_reading;
    QFuture<void> m_thread;
    QSemaphore m_semaphore;
    QAtomicInteger<bool> m_running;
    QAtomicInteger<bool> m_paused_stopped;
    qint64 m_begin;
    QString m_current_path;
    int m_current_row;
};

#endif // FFMPEGCLASS_H
