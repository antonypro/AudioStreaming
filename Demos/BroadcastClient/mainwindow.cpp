#include "mainwindow.h"

#define TITLE "Broadcast Client Demo"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    qRegisterMetaType<QVector<SpectrumStruct> >("QVector<SpectrumStruct>");

    setWindowTitle(TITLE);

    m_audio_lib = nullptr;
    m_discover_instance = new AudioStreamingLibCore(this);

    m_spectrum_analyzer = nullptr;

    QWidget *widget = new QWidget(this);

    linehost = new QLineEdit(this);
    buttonconnect = new QPushButton(this);
    lineport = new QLineEdit(this);
    linetime = new QLineEdit(this);
    linepassword = new QLineEdit(this);
    tabwidget = new QTabWidget(this);
    texteditsettings = new QPlainTextEdit(this);
    listwidgetpeers = new QListWidget(this);
    labelvolume = new QLabel(this);
    slidervolume = new QSlider(Qt::Horizontal, this);
    bars = new Bars(this);
    level = new LevelWidget(this);

    QGridLayout *layout = new QGridLayout(widget);

    layout->addWidget(new QLabel("Host:", this), 0, 0);
    layout->addWidget(linehost, 0, 1, 1, 2);
    layout->addWidget(new QLabel("Port:", this), 1, 0);
    layout->addWidget(lineport, 1, 1);
    layout->addWidget(buttonconnect, 1, 2);
    layout->addWidget(new QLabel("Buffer time(ms):", this), 2, 0);
    layout->addWidget(linetime, 2, 1, 1, 2);
    layout->addWidget(new QLabel("Password:", this), 3, 0);
    layout->addWidget(linepassword, 3, 1, 1, 2);
    layout->addWidget(tabwidget, 4, 0, 1, 3);
    layout->addWidget(labelvolume, 5, 0, 1, 3);
    layout->addWidget(slidervolume, 6, 0, 1, 3);
    layout->addWidget(level, 0, 5, 7, 1);

    tabwidget->addTab(texteditsettings, "Settings");
    tabwidget->addTab(bars, "Spectrum Analyzer");
    tabwidget->addTab(listwidgetpeers, "Search");

    connect(listwidgetpeers, &QListWidget::itemClicked, this, &MainWindow::selectPeer);

    connect(tabwidget, &QTabWidget::currentChanged, this, &MainWindow::currentChanged);

    linepassword->setEchoMode(QLineEdit::Password);
    buttonconnect->setText("Connect");
    texteditsettings->setReadOnly(true);
    slidervolume->setRange(0, 100);

    buttonconnect->setDefault(true);

    connect(slidervolume, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    connect(linehost, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(lineport, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linetime, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linepassword, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(buttonconnect, &QPushButton::clicked, this, &MainWindow::start);

    slidervolume->setValue(100);

    setCentralWidget(widget);

    linehost->setText("localhost");
    lineport->setText("1024");
    linetime->setText("300");

    setFixedSize(sizeHint());
}

MainWindow::~MainWindow()
{

}

void MainWindow::currentChanged(int index)
{
    if (index < 0)
        return;

    if (index == 2)
        startDiscover();
    else
        stopDiscover();
}

void MainWindow::startDiscover()
{
    if (!isVisible())
        return;

    listwidgetpeers->clear();

    DiscoverClient *instance = m_discover_instance->discoverInstance();

    connect(instance, &DiscoverClient::peerFound, this, &MainWindow::peerFound);
    connect(this, &MainWindow::stoprequest, instance, &DiscoverClient::deleteLater);

    instance->discover(lineport->text().toInt(), QByteArray("BroadcastTCPDemo"));
}

void MainWindow::stopDiscover()
{
    if (!isVisible())
        return;

    emit stoprequest();
    listwidgetpeers->clear();
}

void MainWindow::peerFound(const QHostAddress &address, const QString &id)
{
    QString peer;

    QString ip = QHostAddress(address.toIPv4Address()).toString();

    peer.append(QString("IP: %0").arg(ip));

    if (!id.isEmpty())
        peer.append(QString(" - ID: %0").arg(id));

    QListWidgetItem *item = new QListWidgetItem();
    item->setText(peer);
    item->setData(Qt::UserRole, ip);

    listwidgetpeers->addItem(item);
}

void MainWindow::selectPeer(QListWidgetItem *item)
{
    linehost->setText(item->data(Qt::UserRole).toString());
    linehost->setFocus();
}

void MainWindow::start()
{
    if (m_audio_lib)
    {
        m_audio_lib->stop();
        return;
    }

    buttonconnect->setText("Disconnect");

    linehost->setEnabled(false);
    lineport->setEnabled(false);
    linetime->setEnabled(false);
    linepassword->setEnabled(false);
    tabwidget->setTabEnabled(2, false);

    m_audio_lib = new AudioStreamingLibCore(this);

    connect(m_audio_lib, &AudioStreamingLibCore::destroyed, this, [this]{m_audio_lib = nullptr;});

    QByteArray password = linepassword->text().toLatin1();

    StreamingInfo info;

    info.setWorkMode(StreamingInfo::StreamingWorkMode::BroadcastClient);
    info.setTimeToBuffer(linetime->text().toInt());
    info.setSslEnabled(!password.isEmpty());
    info.setGetAudioEnabled(true);
    info.setNegotiationString(QByteArray("BroadcastTCPDemo"));

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);
    connect(m_audio_lib, &AudioStreamingLibCore::outputLevel, level, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);
    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::connected);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::disconnected);
    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);

    m_audio_lib->start(info);

    m_audio_lib->setVolume(slidervolume->value());

    m_audio_lib->connectToHost(linehost->text().trimmed(), lineport->text().toInt(), password);
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
    str.append(QString("Byte order: %0\n").arg((format.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    str.append(QString("\nBuffer time: %0 ms").arg(m_audio_lib->streamingInfo().timeToBuffer()));

    texteditsettings->setPlainText(str);

    if (!m_spectrum_analyzer)
    {
        m_spectrum_analyzer = new SpectrumAnalyzer();
        QThread *thread = new QThread();
        m_spectrum_analyzer->moveToThread(thread);

        connect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, m_spectrum_analyzer, &SpectrumAnalyzer::calculateSpectrum);
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

void MainWindow::volumeChanged(int volume)
{
    if (m_audio_lib)
        m_audio_lib->setVolume(volume);

    QString str = QString("Volume: %0").arg(volume);
    labelvolume->setText(str);
}

void MainWindow::error(const QString &error)
{
    if (!error.isEmpty())
        QMessageBox::critical(this, "Error", error);
}

void MainWindow::connected(const QHostAddress &address, const QString &id)
{
    QString peer = !id.isEmpty() ? id : QHostAddress(address.toIPv4Address()).toString();

    QString title = QString("Connected to: %0 - %1").arg(peer).arg(TITLE);

    setWindowTitle(title);
}

void MainWindow::disconnected(const QHostAddress &address)
{
    Q_UNUSED(address)

    if (m_audio_lib)
        m_audio_lib->stop();

    setWindowTitle(TITLE);
}

void MainWindow::finished()
{
    if (!isVisible())
        return;

    buttonconnect->setText("Connect");

    linehost->setEnabled(true);
    lineport->setEnabled(true);
    linetime->setEnabled(true);
    linepassword->setEnabled(true);
    tabwidget->setTabEnabled(2, true);

    texteditsettings->clear();

    if (m_audio_lib)
        m_audio_lib->deleteLater();

    setWindowTitle(TITLE);
}
