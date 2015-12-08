#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    recorder = 0;
    setFixedSize(size());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString host = ui->lineEdit->text();
    quint16 port = ui->lineEdit_2->text().toInt();
    client = new Client(host, port, this);

    ui->lineEdit->setEnabled(false);
    ui->lineEdit_2->setEnabled(false);
    ui->pushButton->setEnabled(false);
    ui->checkBox->setEnabled(true);
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    if (checked)
    {
        QAudioFormat format = client->currentAudioFormat();

        QString filename = QFileDialog::getSaveFileName(this, "Save to wav file",
                                                        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                        "Sound (*.wav)");

        if (!filename.isEmpty())
        {
            recorder = new AudioRecorder(filename, format, this);
            recorder->open();

            connect(client, SIGNAL(audioReady(QByteArray)), recorder, SLOT(write(QByteArray)));
        }
    }
    else if (recorder)
    {
        disconnect(client, SIGNAL(audioReady(QByteArray)), recorder, SLOT(write(QByteArray)));
        delete recorder;
        recorder = 0;
    }
}
