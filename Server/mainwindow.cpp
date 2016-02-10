#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    input = NULL;
    server = NULL;
    thread = NULL;

    combobox = new QComboBox(this);

    label1 = new QLabel(this);
    label1->setText("Port:");

    edit1 = new QLineEdit(this);
    edit1->setMinimumWidth(100);
    edit1->setText("1024");

    button = new QPushButton(this);
    button->setText("Start Server");

    label2 = new QLabel(this);
    label2->setText("Sample size:");
    edit2 = new QLineEdit(this);
    edit2->setText("16");

    label3 = new QLabel(this);
    label3->setText("Sample rate:");
    edit3 = new QLineEdit(this);
    edit3->setText("44100");

    label4 = new QLabel(this);
    label4->setText("Channels:");
    edit4 = new QLineEdit(this);
    edit4->setText("2");

    box1 = new QGroupBox(this);
    box1->setTitle("Sample type:");
    radiobutton1 = new QRadioButton(this);
    radiobutton1->setText("Signed int");
    radiobutton2 = new QRadioButton(this);
    radiobutton2->setText("Unsigned int");

    box2 = new QGroupBox(this);
    box2->setTitle("Byte order:");
    radiobutton3 = new QRadioButton(this);
    radiobutton3->setText("Little Endian");
    radiobutton4 = new QRadioButton(this);
    radiobutton4->setText("Big Endian");

    connect(button, &QPushButton::clicked, this, &MainWindow::on_pushButton_clicked);

    QWidget *widget = new QWidget(this);
    QGridLayout *layout = new QGridLayout(widget);

    layout->addWidget(combobox, 0, 0, 1, 2);
    layout->addWidget(label1, 1, 0);
    QGridLayout *layout1 = new QGridLayout();
    layout1->addWidget(edit1, 0, 0);
    layout1->addWidget(button, 0, 1);
    layout->addLayout(layout1, 1, 1);
    layout->addWidget(label2, 2, 0);
    layout->addWidget(edit2, 2, 1);
    layout->addWidget(label3, 3, 0);
    layout->addWidget(edit3, 3, 1);
    layout->addWidget(label4, 4, 0);
    layout->addWidget(edit4, 4, 1);
    QGridLayout *layout2 = new QGridLayout(box1);
    layout2->addWidget(radiobutton1, 0, 0);
    layout2->addWidget(radiobutton2, 0, 1);
    QGridLayout *layout3 = new QGridLayout(box2);
    layout3->addWidget(radiobutton3, 0, 0);
    layout3->addWidget(radiobutton4, 0, 1);
    layout->addWidget(box1, 5, 0, 1, 2);
    layout->addWidget(box2, 6, 0, 1, 2);

    radiobutton1->setChecked(true);
    radiobutton3->setChecked(true);

    getDevInfo();

    setCentralWidget(widget);

    setFixedSize(minimumSizeHint());
}

MainWindow::~MainWindow()
{

}

void MainWindow::finish()
{
    if (thread)
    {
        if (input)
            QMetaObject::invokeMethod(input, "deleteLater", Qt::QueuedConnection);
        QEventLoop loop;
        connect(thread, &QThread::destroyed, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    finish();
}

void MainWindow::getDevInfo()
{
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    if (!devices.isEmpty())
    {
        for (int i = 0; i < devices.size(); i++)
            combobox->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));
    }
    else
    {
        QMessageBox::warning(this, "Error", "No input device found!");
    }
}

void MainWindow::on_pushButton_clicked()
{
    startStop();
}

void MainWindow::startStop()
{
    if (!thread)
    {
        quint16 port = edit1->text().toInt();

        QAudioDeviceInfo devinfo = combobox->itemData(combobox->currentIndex()).value<QAudioDeviceInfo>();

        thread = new QThread();
        input = new AudioInput();
        server = new Server();
        input->moveToThread(thread);
        server->moveToThread(thread);
        connect(input, &AudioInput::adjustSettings, this, &MainWindow::adjustSettings);
        connect(input, &AudioInput::error, this, &MainWindow::error);
        connect(input, &AudioInput::dataReady, server, &Server::writeData);
        connect(input, &AudioInput::destroyed, thread, &QThread::quit);
        connect(input, &AudioInput::destroyed, this, &MainWindow::zeropointer);
        connect(server, &Server::destroyed, this, &MainWindow::zeropointer);
        connect(server, &Server::error, this, &MainWindow::error);
        connect(thread, &QThread::finished, server, &Server::deleteLater);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(thread, &QThread::destroyed, this, &MainWindow::zeropointer);
        connect(thread, &QThread::started, this, &MainWindow::started);

        QMetaObject::invokeMethod(input, "start", Qt::QueuedConnection,
                                  Q_ARG(QAudioDeviceInfo, devinfo),
                                  Q_ARG(int, edit2->text().toInt()),
                                  Q_ARG(int, edit3->text().toInt()),
                                  Q_ARG(int, edit4->text().toInt()),
                                  Q_ARG(int, (int)(radiobutton1->isChecked() ?
                                                       QAudioFormat::SignedInt : QAudioFormat::UnSignedInt)),
                                  Q_ARG(int, (int)(radiobutton3->isChecked() ?
                                                       QAudioFormat::LittleEndian : QAudioFormat::BigEndian)));

        QMetaObject::invokeMethod(server, "start", Qt::QueuedConnection, Q_ARG(quint16, port));

        thread->start();

        combobox->setEnabled(false);
        edit1->setEnabled(false);
        edit2->setEnabled(false);
        edit3->setEnabled(false);
        edit4->setEnabled(false);
        box1->setEnabled(false);
        box2->setEnabled(false);

        button->setText("Stop Server");
    }
    else
    {
        button->setEnabled(false);
        finish();
        button->setEnabled(true);

        combobox->setEnabled(true);
        edit1->setEnabled(true);
        edit2->setEnabled(true);
        edit3->setEnabled(true);
        edit4->setEnabled(true);
        box1->setEnabled(true);
        box2->setEnabled(true);

        button->setText("Start Server");
    }
}

void MainWindow::error(const QString &errorstr)
{
    startStop();

    QMessageBox::information(this, "Error", errorstr);
}

void MainWindow::started()
{
    QByteArray header;
    QMetaObject::invokeMethod(input, "header", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QByteArray, header));
    QMetaObject::invokeMethod(server, "setHeader", Qt::BlockingQueuedConnection, Q_ARG(QByteArray, header));
}

void MainWindow::adjustSettings(const QAudioFormat &format)
{
    edit2->setText(QString::number(format.sampleSize()));
    edit3->setText(QString::number(format.sampleRate()));
    edit4->setText(QString::number(format.channelCount()));

    if (format.sampleType() == QAudioFormat::SignedInt)
        radiobutton1->setChecked(true);
    else
        radiobutton2->setChecked(true);

    if (format.byteOrder() == QAudioFormat::LittleEndian)
        radiobutton3->setChecked(true);
    else
        radiobutton4->setChecked(true);

    QMessageBox::information(this, "Info", "The audio setting are not accepted by the input device and must be adjusted!");
}

void MainWindow::zeropointer(QObject *object)
{
    if (object == input)
        input = NULL;
    else if (object == server)
        server = NULL;
    else if (object == thread)
        thread = NULL;
}
