#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "client.h"
#include "audiorecorder.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_checkBox_clicked(bool checked);

private:
    Ui::MainWindow *ui;
    Client *client;
    AudioRecorder *recorder;
};

#endif // MAINWINDOW_H
