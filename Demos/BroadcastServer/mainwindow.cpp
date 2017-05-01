#include "mainwindow.h"

#define TITLE "Broadcast Server Demo"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    qRegisterMetaType<QVector<SpectrumStruct> >("QVector<SpectrumStruct>");

    setWindowTitle(TITLE);

    m_audio_lib = nullptr;

    m_spectrum_analyzer = nullptr;

#ifdef Q_OS_WIN
    loopback = nullptr;
#endif

    tabwidget = new QTabWidget(this);

    QWidget *widget = new QWidget(this);

    QGridLayout *layout = new QGridLayout(widget);

    QGridLayout *layout1 = new QGridLayout();

    comboboxaudioinput = new QComboBox(this);

    listconnections = new QListWidget(this);

    lineport = new QLineEdit(this);

    linemaxconnections = new QLineEdit(this);

    buttonstart = new QPushButton(this);
    buttonstart->setText("Start Server");

    lineid = new QLineEdit(this);

    linepassword = new QLineEdit(this);
    linepassword->setEchoMode(QLineEdit::Password);

    buttonstart->setDefault(true);

    texteditsettings = new QPlainTextEdit(this);

    layout1->setMargin(0);
    layout1->addWidget(new QLabel("Port:", this), 1, 0);
    layout1->addWidget(lineport, 1, 1);
    layout1->addWidget(buttonstart, 1, 2);
    layout1->addWidget(new QLabel("Maximum connections:", this), 2, 0);
    layout1->addWidget(linemaxconnections, 2, 1, 1, 2);
    layout1->addWidget(new QLabel("ID:", this), 3, 0);
    layout1->addWidget(lineid, 3, 1, 1, 2);
    layout1->addWidget(new QLabel("Password:", this), 4, 0);
    layout1->addWidget(linepassword, 4, 1, 1, 2);

    bars = new Bars(this);
    level = new LevelWidget(this);

    tabwidget->addTab(listconnections, "Connections");
    tabwidget->addTab(bars, "Spectrum Analyzer");
    tabwidget->addTab(texteditsettings, "Settings");

    layout->addWidget(comboboxaudioinput, 0, 0, 1, 1);
    layout->addWidget(tabwidget, 1, 0, 1, 1);
    layout->addLayout(layout1, 2, 0, 1, 1);
    layout->addWidget(level, 0, 1, 3, 1);

    texteditsettings->setReadOnly(true);

    connect(lineport, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linemaxconnections, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(lineid, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linepassword, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(buttonstart, &QPushButton::clicked, this, &MainWindow::start);

    lineport->setText("1024");
    linemaxconnections->setText("10");

    setCentralWidget(widget);

    setFixedSize(sizeHint());

    getDevInfo();
}

MainWindow::~MainWindow()
{

}

void MainWindow::start()
{
    if (m_audio_lib)
    {
        m_audio_lib->stop();
#ifdef Q_OS_WIN
        if (loopback)
            loopback->deleteLater();
#endif
        return;
    }

    QByteArray password = linepassword->text().toLatin1();

    if (comboboxaudioinput->count() == 0)
    {
        QMessageBox::critical(this, "Error", "No input device found");
        return;
    }

    bool ok = false;
    int port = lineport->text().toInt(&ok);

    if (!ok || port < 1 || port > 65535)
    {
        QMessageBox::critical(this, "Error", "Port must have a value between 1 and 65535,\n"
                                             "including these values");
        return;
    }

    int max_connections = linemaxconnections->text().toInt(&ok);

    if (!ok || max_connections < 1)
    {
        QMessageBox::critical(this, "Error", "Enter a value equal or higher to 1 on maximum connections");
        return;
    }

    m_audio_lib = new AudioStreamingLibCore(this);

    connect(m_audio_lib, &AudioStreamingLibCore::destroyed, this, [this]{m_audio_lib = nullptr;});

    StreamingInfo info;

    info.setWorkMode(StreamingInfo::StreamingWorkMode::BroadcastServer);
    info.setSslEnabled(!password.isEmpty());
    info.setGetAudioEnabled(true);
    info.setNegotiationString(QByteArray("BroadcastTCPDemo"));
    info.setID(lineid->text().trimmed());

    QByteArray private_key;
    QByteArray public_key;

    if (info.isSslEnabled())
        AudioStreamingLibCore::generateAsymmetricKeys(&private_key, &public_key);

    QAudioDeviceInfo devinfo = comboboxaudioinput->currentData().value<QAudioDeviceInfo>();

#ifdef Q_OS_WIN
    if (devinfo.isNull())
    {
        QAudioFormat format;

        loopback = new QWinLoopback(this);

        connect(loopback, &QObject::destroyed, this, [this]
        {
            loopback = nullptr;

            if (isVisible())
                buffer.clear();
        });

        bool started = loopback->Start();

        if (!started)
            return;

        info.setInputDeviceType(StreamingInfo::AudioDeviceType::CustomAudioDevice);
        info.setCallBackEnabled(true);

        format = loopback->format();

        connect(loopback, &QWinLoopback::readyRead, this, &MainWindow::loopbackdata);
        connect(m_audio_lib, &AudioStreamingLibCore::inputData, this, &MainWindow::process);

        info.setInputAudioFormat(format);
    }
    else
#endif
    {
        info.setInputDeviceInfo(devinfo);

        QAudioFormat format;

        format.setSampleSize(16);
        format.setSampleRate(44100);
        format.setChannelCount(2);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setByteOrder(QAudioFormat::LittleEndian);

        info.setInputAudioFormat(format);
    }

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);
    connect(m_audio_lib, &AudioStreamingLibCore::inputLevel, level, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);
    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);
    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::updateConnections);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::updateConnections);

    m_audio_lib->start(info);

    m_audio_lib->setKeys(private_key, public_key);

    m_audio_lib->listen(port, true, password, max_connections);

    lineport->setEnabled(false);
    linemaxconnections->setEnabled(false);
    lineid->setEnabled(false);
    linepassword->setEnabled(false);

    comboboxaudioinput->setEnabled(false);
    buttonstart->setText("Stop Server");

    QString title = QString("%0 connection(s) - %1").arg(0).arg(TITLE);

    setWindowTitle(title);
}

void MainWindow::loopbackdata(const QByteArray &data)
{
#ifdef Q_OS_WIN
    if (!loopback)
        return;
    buffer.append(data);
#else
    Q_UNUSED(data);
#endif
}

void MainWindow::process(const QByteArray &data)
{
#ifdef Q_OS_WIN
    if (!m_audio_lib || !m_audio_lib->isRunning())
       return;

    int size = data.size();

    QByteArray mid = buffer.mid(0, size);
    buffer.remove(0, size);

    m_audio_lib->inputDataBack(mid);
#else
    Q_UNUSED(data);
#endif
}

void MainWindow::adjustSettings()
{
    QAudioFormat inputFormat = m_audio_lib->inputAudioFormat();
    QAudioFormat format = m_audio_lib->audioFormat();

    QString str;

    str.append("Input format:\n\n");

    str.append(QString("Sample size: %0 bits\n").arg(inputFormat.sampleSize()));
    str.append(QString("Sample rate: %0 hz\n").arg(inputFormat.sampleRate()));
    str.append(QString("Channels: %0\n").arg(inputFormat.channelCount()));
    str.append(QString("Sample type: %0\n").arg((inputFormat.sampleType() == QAudioFormat::SignedInt) ? "Signed integer" : "Unsigned integer"));
    str.append(QString("Byte order: %0\n").arg((inputFormat.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    str.append("\n");

    str.append("Resampled format:\n\n");

    str.append(QString("Sample size: %0 bits\n").arg(format.sampleSize()));
    str.append(QString("Sample rate: %0 hz\n").arg(format.sampleRate()));
    str.append(QString("Channels: %0\n").arg(format.channelCount()));
    str.append(QString("Sample type: %0\n").arg((format.sampleType() == QAudioFormat::SignedInt) ? "Signed integer" : "Unsigned integer"));
    str.append(QString("Byte order: %0").arg((format.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    texteditsettings->setPlainText(str);

    if (!m_spectrum_analyzer)
    {
        m_spectrum_analyzer = new SpectrumAnalyzer();
        QThread *thread = new QThread();
        m_spectrum_analyzer->moveToThread(thread);

        connect(m_audio_lib, &AudioStreamingLibCore::veryInputData, m_spectrum_analyzer, &SpectrumAnalyzer::calculateSpectrum);
        connect(m_spectrum_analyzer, &SpectrumAnalyzer::spectrumChanged, bars, &Bars::setValues);
        connect(m_spectrum_analyzer, &SpectrumAnalyzer::destroyed, bars, &Bars::clear);

        connect(m_audio_lib, &AudioStreamingLibCore::finished, m_spectrum_analyzer, &SpectrumAnalyzer::deleteLater);
        connect(this, &MainWindow::destroyed, m_spectrum_analyzer, &SpectrumAnalyzer::deleteLater);
        connect(m_spectrum_analyzer, &SpectrumAnalyzer::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        connect(m_spectrum_analyzer, &SpectrumAnalyzer::destroyed, this, [this]{m_spectrum_analyzer = nullptr;});

        QMetaObject::invokeMethod(m_spectrum_analyzer, "start", Qt::QueuedConnection, Q_ARG(QAudioFormat, format));

        thread->start();
    }
}

void MainWindow::updateConnections()
{
    QList<QHostAddress> connections = m_audio_lib->connectionsList();

    listconnections->clear();

    foreach (const QHostAddress &addr, connections)
        listconnections->addItem(addr.toString());

    QString title = QString("%0 connection(s) - %1").arg(connections.size()).arg(TITLE);

    setWindowTitle(title);
}

void MainWindow::error(const QString &error)
{
    if (!error.isEmpty())
        QMessageBox::critical(this, "Error", error);
}

void MainWindow::finished()
{
    if (!isVisible())
        return;

    lineport->setEnabled(true);
    linemaxconnections->setEnabled(true);
    lineid->setEnabled(true);
    linepassword->setEnabled(true);

    comboboxaudioinput->setEnabled(true);
    buttonstart->setText("Start Server");

    texteditsettings->clear();

    listconnections->clear();

    if (m_audio_lib)
        m_audio_lib->deleteLater();

    setWindowTitle(TITLE);

#ifdef Q_OS_WIN
        if (loopback)
            loopback->deleteLater();
#endif
}

void MainWindow::getDevInfo()
{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
        comboboxaudioinput->addItem("Loopback (What you hear)");

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for (int i = 0; i < devices.size(); i++)
        comboboxaudioinput->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));

    if (comboboxaudioinput->count() == 0)
        QMessageBox::warning(this, "Error", "No input device found!");
}
