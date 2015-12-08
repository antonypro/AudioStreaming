#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFixedSize(size());

    getDevInfo();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getDevInfo()
{
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    if (devices.isEmpty()) QMessageBox::warning(this, "Error", "No input device found!");
    for(int i = 0; i < devices.size(); ++i)
        ui->comboBox->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));
}

void MainWindow::on_pushButton_clicked()
{
    QAudioDeviceInfo devinfo = ui->comboBox->itemData(ui->comboBox->currentIndex()).value<QAudioDeviceInfo>();
    input = new AudioInput(devinfo, this);
    quint16 port = ui->lineEdit->text().toInt();
    server = new Server(port, this);
    server->setHeader(input->header());
    connect(input, SIGNAL(dataReady(QByteArray)), server, SLOT(writeData(QByteArray)));

    ui->comboBox->setEnabled(false);
    ui->lineEdit->setEnabled(false);
    ui->pushButton->setEnabled(false);
}
