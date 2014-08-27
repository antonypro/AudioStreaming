#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "audioinput.h"

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
    void getDevInfo();
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    AudioInput *input;
    Server *server;
};

#endif // MAINWINDOW_H
