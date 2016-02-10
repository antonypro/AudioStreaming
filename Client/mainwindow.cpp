#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    finishing = false;

    thread = NULL;
    client = NULL;
    recorder = NULL;

    label1 = new QLabel(this);
    edit1 = new QLineEdit(this);
    edit1->setMinimumWidth(100);
    label1->setText("Host:");
    edit1->setText("localhost");

    label2 = new QLabel(this);
    edit2 = new QLineEdit(this);
    edit2->setMinimumWidth(100);
    label2->setText("Port:");
    edit2->setText("1024");

    label3 = new QLabel(this);
    edit3 = new QLineEdit(this);
    edit3->setMinimumWidth(100);
    label3->setText("Buffer time (ms):");
    edit3->setText("300");

    checkbox = new QCheckBox(this);
    checkbox->setText("Record");

    label4 = new QLabel(this);
    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, 100);
    slider->setValue(100);

    button = new QPushButton(this);
    button->setText("Connect");

    levelwidget = new LevelWidget(this);
    levelwidget->setFixedWidth(20);

    QWidget *widget = new QWidget(this);
    QGridLayout *layout = new QGridLayout(widget);

    connect(slider, &QSlider::valueChanged, this, &MainWindow::on_horizontalSlider_valueChanged);
    connect(checkbox, &QCheckBox::clicked, this, &MainWindow::on_checkBox_clicked);
    connect(button, &QPushButton::clicked, this, &MainWindow::on_pushButton_clicked);

    QGridLayout *layout1 = new QGridLayout();
    layout1->addWidget(label1, 0, 0);
    layout1->addWidget(edit1, 0, 1);
    layout1->addWidget(label2, 1, 0);
    layout1->addWidget(edit2, 1, 1);
    layout1->addWidget(label3, 2, 0);
    layout1->addWidget(edit3, 2, 1);
    layout->addLayout(layout1, 0, 0);
    layout->addWidget(checkbox, 2, 0);
    layout->addWidget(label4, 3, 0, 1, 2);
    layout->addWidget(slider, 4, 0, 1, 2);
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QGridLayout *layout3 = new QGridLayout();
    layout3->addItem(spacer, 0, 0);
    layout3->addWidget(button, 0, 1);
    layout->addLayout(layout3, 5, 0, 1, 2);
    layout->addWidget(levelwidget, 0, 2, 6, 1);

    on_horizontalSlider_valueChanged(100);

    setCentralWidget(widget);

    setFixedSize(minimumSizeHint());

    QMetaObject::invokeMethod(this, "level", Qt::QueuedConnection, Q_ARG(float, 0));
}

MainWindow::~MainWindow()
{

}

void MainWindow::finish()
{
    if (thread)
    {
        button->setEnabled(false);
        finishing = true;

        if (client)
            QMetaObject::invokeMethod(client, "deleteLater", Qt::QueuedConnection);
        QEventLoop loop;
        connect(thread, &QThread::destroyed, &loop, &QEventLoop::quit);
        loop.exec();

        finishing = false;
        button->setEnabled(true);
        button->setText("Connect");
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    finish();
}

void MainWindow::on_pushButton_clicked()
{
    startStop();
}

void MainWindow::startStop()
{
    if (finishing)
        return;

    if (!thread)
    {
        QString host = edit1->text();
        quint16 port = edit2->text().toInt();
        uint timetobuffer = edit3->text().toUInt();

        client = new Client();
        thread = new QThread();

        client->moveToThread(thread);
        connect(client, &Client::error, this, &MainWindow::error);
        connect(client, &Client::currentlevel, this, &MainWindow::level);
        connect(client, &Client::destroyed, thread, &QThread::quit);
        connect(client, &Client::destroyed, this, &MainWindow::zeropointer);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(thread, &QThread::destroyed, this, &MainWindow::zeropointer);
        QMetaObject::invokeMethod(client, "start", Qt::QueuedConnection, Q_ARG(QString, host), Q_ARG(quint16, port), Q_ARG(uint, timetobuffer));
        QMetaObject::invokeMethod(client, "setVolume", Qt::QueuedConnection, Q_ARG(int, slider->value()));
        thread->start();

        edit1->setEnabled(false);
        edit2->setEnabled(false);
        edit3->setEnabled(false);
        checkbox->setEnabled(true);

        button->setText("Disconnect");
    }
    else
    {
        if (recorder)
        {
            disconnect(client, &Client::audioReady, this, &MainWindow::write);
            recorder->deleteLater();
        }

        finish();

        level(0);

        edit1->setEnabled(true);
        edit2->setEnabled(true);
        edit3->setEnabled(true);
        checkbox->setChecked(false);
        checkbox->setEnabled(false);
    }
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    if (checked)
    {
        QAudioFormat format;
        QMetaObject::invokeMethod(client, "currentAudioFormat", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QAudioFormat, format));

        QString filename = QFileDialog::getSaveFileName(this, "Save to wav file",
                                                        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                        "Sound (*.wav)");

        if (!filename.isEmpty())
        {
            if (!filename.endsWith(".wav"))
                filename.append(".wav");

            recorder = new AudioRecorder(filename, format, this);
            connect(recorder, &AudioRecorder::destroyed, this, &MainWindow::zeropointer);

            if (!recorder->open())
            {
                QMessageBox::warning(this, "Error", recorder->errorString());
                checkbox->setChecked(false);
                recorder->deleteLater();
                return;
            }

            if (client)
                connect(client, &Client::audioReady, this, &MainWindow::write);
        }
        else
        {
            checkbox->setChecked(false);
        }
    }
    else if (recorder)
    {
        if (client)
            disconnect(client, &Client::audioReady, this, &MainWindow::write);
        recorder->deleteLater();
    }
}

void MainWindow::level(float size)
{
    levelwidget->setlevel(size);
}

void MainWindow::write(QByteArray data)
{
    recorder->write(data);
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    if (client)
        QMetaObject::invokeMethod(client, "setVolume", Qt::QueuedConnection, Q_ARG(int, value));

    label4->setText(QString("Volume: %0%").arg(value));
}

void MainWindow::error(const QString &errorstr)
{
    startStop();

    QMessageBox::information(this, "Error", errorstr);
}

void MainWindow::zeropointer(QObject *object)
{
    if (object == client)
        client = NULL;
    else if (object == thread)
        thread = NULL;
    else if (object == recorder)
        recorder = NULL;
}
