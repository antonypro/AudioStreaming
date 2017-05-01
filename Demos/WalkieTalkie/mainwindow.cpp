#include "mainwindow.h"

#define TITLE "Walkie Talkie Demo"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(TITLE);

    m_audio_lib = nullptr;
    m_discover_instance = new AudioStreamingLibCore(this);

    listwidgetpeers = new QListWidget(this);

    connect(listwidgetpeers, &QListWidget::itemClicked, this, &MainWindow::selectPeer);

    //Client

    comboboxaudioinput = new QComboBox(this);

    labelhost = new QLabel(this);
    linehost = new QLineEdit(this);
    linehost->setMinimumWidth(100);
    labelhost->setText("Host:");
    linehost->setText("localhost");

    labelport = new QLabel(this);
    lineport = new QLineEdit(this);
    lineport->setMinimumWidth(100);
    labelport->setText("Port:");
    lineport->setText("1024");

    texteditsettings = new QPlainTextEdit(this);
    texteditsettings->setReadOnly(true);

    buttonconnect = new QPushButton(this);
    buttonconnect->setText("Connect");

    labelclientid = new QLabel(this);
    labelclientid->setText("ID:");

    lineclientid = new QLineEdit(this);

    labelclientpassword = new QLabel(this);
    labelclientpassword->setText("Password:");

    lineclientpassword = new QLineEdit(this);
    lineclientpassword->setEchoMode(QLineEdit::Password);

    widget1 = new QWidget(this);

    {
        QGridLayout *layout = new QGridLayout(widget1);

        layout->addWidget(labelhost, 0, 0, 1, 1);
        layout->addWidget(linehost, 0, 1, 1, 2);
        layout->addWidget(labelport, 1, 0, 1, 1);
        layout->addWidget(lineport, 1, 1, 1, 1);
        layout->addWidget(buttonconnect, 1, 2, 1, 1);
        layout->addWidget(labelclientid, 2, 0, 1, 1);
        layout->addWidget(lineclientid, 2, 1, 1, 2);
        layout->addWidget(labelclientpassword, 3, 0, 1, 1);
        layout->addWidget(lineclientpassword, 3, 1, 1, 2);
    }

    //Server

    labelportserver = new QLabel(this);
    labelportserver->setText("Port:");

    lineportserver = new QLineEdit(this);
    lineportserver->setMinimumWidth(100);
    lineportserver->setText("1024");

    buttonstartserver = new QPushButton(this);
    buttonstartserver->setText("Start Server");

    labelsamplesize = new QLabel(this);
    labelsamplesize->setText("Sample size:");
    linesamplesize = new QLineEdit(this);
    linesamplesize->setText("16");

    labelsamplerate = new QLabel(this);
    labelsamplerate->setText("Sample rate:");
    linesamplerate = new QLineEdit(this);
    linesamplerate->setText("44100");

    labelchannels = new QLabel(this);
    labelchannels->setText("Channels:");
    linechannels = new QLineEdit(this);
    linechannels->setText("2");

    labelbuffertime = new QLabel(this);
    labelbuffertime->setText("Buffer time (ms):");
    linebuffertime = new QLineEdit(this);
    linebuffertime->setText("300");

    labelserverid = new QLabel(this);
    labelserverid->setText("ID:");

    lineserverid = new QLineEdit(this);

    labelserverpassword = new QLabel(this);
    labelserverpassword->setText("Password:");

    lineserverpassword = new QLineEdit(this);
    lineserverpassword->setEchoMode(QLineEdit::Password);

    widget2 = new QWidget(this);

    {
        QGridLayout *layout = new QGridLayout(widget2);

        layout->addWidget(labelportserver, 0, 0);
        QGridLayout *layout1 = new QGridLayout();
        layout1->addWidget(lineportserver, 0, 0);
        layout1->addWidget(buttonstartserver, 0, 1);
        layout->addLayout(layout1, 0, 1);
        layout->addWidget(labelserverid, 1, 0);
        layout->addWidget(lineserverid, 1, 1);
        layout->addWidget(labelserverpassword, 2, 0);
        layout->addWidget(lineserverpassword, 2, 1);
        layout->addWidget(labelsamplesize, 3, 0);
        layout->addWidget(linesamplesize, 3, 1);
        layout->addWidget(labelsamplerate, 4, 0);
        layout->addWidget(linesamplerate, 4, 1);
        layout->addWidget(labelchannels, 5, 0);
        layout->addWidget(linechannels, 5, 1);
        layout->addWidget(labelbuffertime, 6, 0);
        layout->addWidget(linebuffertime, 6, 1);
    }

    //Volume

    labelvolume = new QLabel(this);
    slidervolume = new QSlider(Qt::Horizontal, this);
    slidervolume->setRange(0, 100);

    boxinputmuted = new QCheckBox(this);
    boxinputmuted->setText("Mute input");

    tabwidget = new QTabWidget(this);

    scrollclientserver = new QScrollArea(this);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(Q_OS_WINPHONE)
    scrollclientserver->setWidgetResizable(true);
    scrollclientserver->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#endif

    QWidget *widgetserverclient = new QWidget(this);
    QGridLayout *layoutserverclient = new QGridLayout(widgetserverclient);

    layoutserverclient->addWidget(comboboxaudioinput, 0, 0);

    {
        QWidget *widget = new QWidget(this);
        QGridLayout *layout = new QGridLayout(widget);
        layout->setMargin(0);

        QGroupBox *box1 = new QGroupBox(widget);
        box1->setTitle("Client");
        QGridLayout *layout1 = new QGridLayout(box1);
        layout1->addWidget(widget1, 0, 0);

        layout->addWidget(box1, 0, 0);
        layoutserverclient->addWidget(widget, 1, 0);
    }

    {
        QWidget *widget = new QWidget(this);
        QGridLayout *layout = new QGridLayout(widget);
        layout->setMargin(0);

        QGroupBox *box1 = new QGroupBox(widget);
        box1->setTitle("Server");
        QGridLayout *layout1 = new QGridLayout(box1);
        layout1->addWidget(widget2, 0, 0);

        layout->addWidget(box1, 0, 0);
        layoutserverclient->addWidget(widget, 2, 0);
    }

    scrollclientserver->setWidget(widgetserverclient);

    //Chat

    widgetchat = new QWidget(this);

    texteditchat = new QPlainTextEdit(this);
    texteditchat->setReadOnly(true);

    linechat = new QLineEdit(this);

    buttonsendchat = new QPushButton(this);
    buttonsendchat->setText("Send");

    {
        QGridLayout *layout = new QGridLayout(widgetchat);
        layout->addWidget(texteditchat, 0, 0);
        QGridLayout *layout1 = new QGridLayout();
        layout1->addWidget(linechat, 0, 0);
        layout1->addWidget(buttonsendchat, 0, 1);
        layout->addLayout(layout1, 1, 0);
    }

    tabwidget->addTab(scrollclientserver, "Client Server");
    tabwidget->addTab(listwidgetpeers, "Search");
    tabwidget->addTab(widgetchat, "Chat");
    tabwidget->addTab(texteditsettings, "Settings");

    connect(tabwidget, &QTabWidget::currentChanged, this, &MainWindow::currentChanged);

    connect(linechat, &QLineEdit::returnPressed, this, &MainWindow::writeText);
    connect(buttonsendchat, &QPushButton::clicked, this, &MainWindow::writeText);

    connect(slidervolume, &QSlider::valueChanged, this, &MainWindow::sliderVolumeValueChanged);

    connect(buttonconnect, &QPushButton::clicked, this, &MainWindow::client);
    connect(buttonstartserver, &QPushButton::clicked, this, &MainWindow::server);

    linechat->blockSignals(true);

    buttonsendchat->setEnabled(false);
    buttonsendchat->setDefault(true);

    getDevInfo();

    levelinput = new LevelWidget(this);
    leveloutput = new LevelWidget(this);

    QWidget *mainwidget = new QWidget(this);

    QGridLayout *mainlayout = new QGridLayout(mainwidget);

    mainlayout->addWidget(levelinput, 0, 0, 3, 1);
    mainlayout->addWidget(tabwidget, 0, 1, 1, 3);
    mainlayout->addWidget(boxinputmuted, 1, 1, 1, 1);
    mainlayout->addWidget(labelvolume, 1, 2, 1, 1);
    mainlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 3, 1, 1);
    mainlayout->addWidget(slidervolume, 2, 1, 1, 3);
    mainlayout->addWidget(leveloutput, 0, 4, 3, 1);

    slidervolume->setValue(100);

    setCentralWidget(mainwidget);

    setFixedSize(sizeHint());
}

MainWindow::~MainWindow()
{

}

void MainWindow::currentChanged(int index)
{
    if (index < 0)
        return;

    if (index == 1)
        startDiscover();
    else
        stopDiscover();
}

void MainWindow::startDiscover()
{
    if (!isVisible() || !listwidgetpeers->isEnabled())
        return;

    listwidgetpeers->clear();

    DiscoverClient *instance = m_discover_instance->discoverInstance();

    connect(instance, &DiscoverClient::peerFound, this, &MainWindow::peerFound);
    connect(this, &MainWindow::stoprequest, instance, &DiscoverClient::deleteLater);

    instance->discover(lineport->text().toInt(), QByteArray("WalkieTalkieTCPDemo"));
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
    tabwidget->setCurrentIndex(0);
}

void MainWindow::client()
{
    if (m_audio_lib)
    {
        m_audio_lib->stop();
        return;
    }

    m_audio_lib = new AudioStreamingLibCore(this);

    connect(m_audio_lib, &AudioStreamingLibCore::destroyed, this, [this]{m_audio_lib = nullptr;});

    QByteArray password = lineclientpassword->text().toLatin1();

    QAudioDeviceInfo devinfo = comboboxaudioinput->itemData(comboboxaudioinput->currentIndex()).value<QAudioDeviceInfo>();

    StreamingInfo info;
    info.setInputDeviceInfo(devinfo);
    info.setWorkMode(StreamingInfo::StreamingWorkMode::WalkieTalkieClient);
    info.setSslEnabled(!password.isEmpty());
    info.setNegotiationString(QByteArray("WalkieTalkieTCPDemo"));
    info.setID(lineclientid->text().trimmed());

    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);

    connect(m_audio_lib, &AudioStreamingLibCore::inputLevel, levelinput, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::outputLevel, leveloutput, &LevelWidget::setlevel);

    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::clientConnected);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::clientDisconnected);

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);

    connect(m_audio_lib, &AudioStreamingLibCore::extraData, this, &MainWindow::receiveText);

    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);

    connect(boxinputmuted, &QCheckBox::toggled, m_audio_lib, &AudioStreamingLibCore::setInputMuted);

    m_audio_lib->start(info);

    m_audio_lib->setInputMuted(boxinputmuted->isChecked());

    m_audio_lib->setVolume(slidervolume->value());

    m_audio_lib->connectToHost(linehost->text().trimmed(), lineport->text().toInt(), password);

    clientStarted(false);

    buttonconnect->setText("Connecting...");

    buttonconnect->setEnabled(false);

    tabwidget->setTabEnabled(1, false);
}

void MainWindow::server()
{
    if (m_audio_lib)
    {
        m_audio_lib->stop();
        return;
    }

    m_audio_lib = new AudioStreamingLibCore(this);

    connect(m_audio_lib, &AudioStreamingLibCore::destroyed, this, [this]{m_audio_lib = nullptr;});

    QByteArray password = lineserverpassword->text().toLatin1();

    QAudioDeviceInfo devinfo = comboboxaudioinput->itemData(comboboxaudioinput->currentIndex()).value<QAudioDeviceInfo>();

    QAudioFormat format;
    format.setSampleSize(linesamplesize->text().toInt());
    format.setSampleRate(linesamplerate->text().toInt());
    format.setChannelCount(linechannels->text().toInt());
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);

    StreamingInfo info;
    info.setInputAudioFormat(format);
    info.setInputDeviceInfo(devinfo);
    info.setWorkMode(StreamingInfo::StreamingWorkMode::WalkieTalkieServer);
    info.setTimeToBuffer(linebuffertime->text().toInt());
    info.setSslEnabled(!password.isEmpty());
    info.setNegotiationString(QByteArray("WalkieTalkieTCPDemo"));
    info.setID(lineserverid->text().trimmed());

    QByteArray private_key;
    QByteArray public_key;

    if (info.isSslEnabled())
        AudioStreamingLibCore::generateAsymmetricKeys(&private_key, &public_key);

    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);

    connect(m_audio_lib, &AudioStreamingLibCore::inputLevel, levelinput, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::outputLevel, leveloutput, &LevelWidget::setlevel);

    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::serverConnected);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::serverDisconnected);

    connect(m_audio_lib, &AudioStreamingLibCore::pending, this, &MainWindow::pending);

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);

    connect(m_audio_lib, &AudioStreamingLibCore::extraData, this, &MainWindow::receiveText);

    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);

    connect(boxinputmuted, &QCheckBox::toggled, m_audio_lib, &AudioStreamingLibCore::setInputMuted);

    m_audio_lib->start(info);

    m_audio_lib->setInputMuted(boxinputmuted->isChecked());

    m_audio_lib->setVolume(slidervolume->value());

    m_audio_lib->setKeys(private_key, public_key);

    m_audio_lib->listen(lineportserver->text().toInt(), false, password);

    serverStarted(false);

    buttonstartserver->setText("Stop Server");

    tabwidget->setTabEnabled(1, false);
}

void MainWindow::sliderVolumeValueChanged(int value)
{
    if (m_audio_lib)
        m_audio_lib->setVolume(value);

    QString str = QString("Output Volume: %0").arg(value);

    labelvolume->setText(str);
}

void MainWindow::getDevInfo()
{
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    if (!devices.isEmpty())
    {
        for (int i = 0; i < devices.size(); i++)
            comboboxaudioinput->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));
    }
    else
    {
        QMessageBox::warning(this, "Error", "No input device found!");
    }
}

void MainWindow::clientStarted(bool enable)
{
    comboboxaudioinput->setEnabled(enable);
    widget2->setEnabled(enable);
    linehost->setEnabled(enable);
    lineport->setEnabled(enable);
    lineclientid->setEnabled(enable);
    lineclientpassword->setEnabled(enable);
}

void MainWindow::serverStarted(bool enable)
{
    comboboxaudioinput->setEnabled(enable);
    widget1->setEnabled(enable);
    lineportserver->setEnabled(enable);
    linesamplesize->setEnabled(enable);
    linesamplerate->setEnabled(enable);
    linechannels->setEnabled(enable);
    linebuffertime->setEnabled(enable);
    lineserverid->setEnabled(enable);
    lineserverpassword->setEnabled(enable);
}

void MainWindow::clientConnected(const QHostAddress &address, const QString &id)
{
    m_peer = !id.isEmpty() ? id : QHostAddress(address.toIPv4Address()).toString();

    QString title = QString("Connected to: %0 - %1").arg(m_peer).arg(TITLE);

    setWindowTitle(title);

    buttonconnect->setText("Disconnect");
    buttonconnect->setEnabled(true);

    linechat->blockSignals(false);
    buttonsendchat->setEnabled(true);
}

void MainWindow::serverConnected(const QHostAddress &address, const QString &id)
{
    m_peer = !id.isEmpty() ? id : QHostAddress(address.toIPv4Address()).toString();

    QString title = QString("Connected to: %0 - %1").arg(m_peer).arg(TITLE);

    setWindowTitle(title);

    buttonstartserver->setText("Stop server");

    linechat->blockSignals(false);
    buttonsendchat->setEnabled(true);
}

void MainWindow::clientDisconnected()
{
    if (m_audio_lib)
        m_audio_lib->stop();

    setWindowTitle(TITLE);

    m_peer = QString();
}

void MainWindow::serverDisconnected()
{
    linechat->blockSignals(true);
    buttonsendchat->setEnabled(false);

    setWindowTitle(TITLE);

    m_peer = QString();
}

void MainWindow::pending(const QHostAddress &address, const QString &id)
{
    QString str = QString("Do you want to start a talk with %0?").arg(QHostAddress(address.toIPv4Address()).toString());

    if (!id.isEmpty())
        str.append(QString("\nID: %0").arg(id));

    int result = QMessageBox::question(this, "Pending connection", str);

    if (result == QMessageBox::Yes)
        m_audio_lib->acceptConnection();
    else
        m_audio_lib->rejectConnection();
}

void MainWindow::error(const QString &error)
{
    if (!error.isEmpty())
        QMessageBox::critical(this, "Error", error);
}

void MainWindow::adjustSettings()
{
    if (!m_audio_lib)
        return;

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
}

void MainWindow::writeText()
{
    if (!m_audio_lib->isReadyToWriteExtraData())
        return;

    QString text = linechat->text().trimmed();

    if (text.isEmpty())
        return;

    linechat->clear();

    QString localtext = QString("You wrote at %0:\n%1\n")
            .arg(QTime::currentTime().toString(Qt::SystemLocaleLongDate)).arg(text);

    texteditchat->appendPlainText(localtext);

    m_audio_lib->writeExtraData(text.toLatin1());
}

void MainWindow::receiveText(const QByteArray &data)
{
    QString text = QLatin1String(data);

    QString localtext = QString("%0 wrote at %1:\n%2\n").arg(m_peer)
            .arg(QTime::currentTime().toString(Qt::SystemLocaleLongDate)).arg(text);

    texteditchat->appendPlainText(localtext);

    m_audio_lib->writeExtraDataResult();
}

void MainWindow::finished()
{
    if (!isVisible())
        return;

    clientStarted(true);
    serverStarted(true);

    linechat->blockSignals(true);
    buttonsendchat->setEnabled(false);

    texteditsettings->clear();
    buttonconnect->setText("Connect");
    buttonconnect->setEnabled(true);
    buttonstartserver->setText("Start Server");

    m_peer = QString();

    levelinput->setlevel(0);
    leveloutput->setlevel(0);

    tabwidget->setTabEnabled(1, true);

    if (m_audio_lib)
        m_audio_lib->deleteLater();

    setWindowTitle(TITLE);
}
