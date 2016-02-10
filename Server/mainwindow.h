#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "audioinput.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void finish();
    void getDevInfo();
    void on_pushButton_clicked();
    void startStop();
    void error(const QString &errorstr);
    void started();
    void adjustSettings(const QAudioFormat &format);
    void zeropointer(QObject *object);

private:
    void closeEvent(QCloseEvent *);

    QComboBox *combobox;
    QLabel *label1;
    QLineEdit *edit1;
    QPushButton *button;
    QLabel *label2;
    QLineEdit *edit2;
    QLabel *label3;
    QLineEdit *edit3;
    QLabel *label4;
    QLineEdit *edit4;
    QGroupBox *box1;
    QGroupBox *box2;
    QRadioButton *radiobutton1;
    QRadioButton *radiobutton2;
    QRadioButton *radiobutton3;
    QRadioButton *radiobutton4;

    QThread *thread;
    AudioInput *input;
    Server *server;
};

#endif // MAINWINDOW_H
