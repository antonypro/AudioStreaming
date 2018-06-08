#include "mainwindow.h"

#define TITLE "Broadcast Client Demo"

QPlainTextEdit *debug_edit = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)

    QMetaObject::invokeMethod(debug_edit, "appendPlainText", Q_ARG(QString, msg));
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    qRegisterMetaType<QVector<SpectrumStruct> >("QVector<SpectrumStruct>");

    setWindowTitle(TITLE);

    m_audio_lib = nullptr;
    m_discover_instance = new AudioStreamingLibCore(this);

    m_spectrum_analyzer = nullptr;

    m_audio_recorder = nullptr;

    comboboxaudiooutput = new QComboBox(this);

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

    bars = new BarsWidget(this);
    waveform = new WaveFormWidget(this);
    level = new LevelWidget(this);

    waveform->setMinimumHeight(10);
    waveform->setMaximumHeight(100);
    bars->setMinimumHeight(100);

    QWidget *analyzer = new QWidget(this);

    QGridLayout *layout_analyzer = new QGridLayout(analyzer);
    layout_analyzer->setMargin(0);
    layout_analyzer->addWidget(waveform, 0, 0);
    layout_analyzer->addWidget(bars, 1, 0);

    QScrollArea *areasettings = new QScrollArea(this);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(Q_OS_WINPHONE)
    areasettings->setWidgetResizable(true);
    areasettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
#endif

    QWidget *widgetsettings = new QWidget(this);

    QGridLayout *layout1 = new QGridLayout(widgetsettings);

    QGridLayout *layout = new QGridLayout(widget);

    layout1->addWidget(new QLabel("Output device:", this), 0, 0);
    layout1->addWidget(comboboxaudiooutput, 0, 1, 1, 2);
    layout1->addWidget(new QLabel("Host:", this), 1, 0);
    layout1->addWidget(linehost, 1, 1, 1, 2);
    layout1->addWidget(new QLabel("Port:", this), 2, 0);
    layout1->addWidget(lineport, 2, 1);
    layout1->addWidget(buttonconnect, 2, 2);
    layout1->addWidget(new QLabel("Buffer time(ms):", this), 3, 0);
    layout1->addWidget(linetime, 3, 1, 1, 2);
    layout1->addWidget(new QLabel("Password:", this), 4, 0);
    layout1->addWidget(linepassword, 4, 1, 1, 2);

    areasettings->setWidget(widgetsettings);

    layout->addWidget(tabwidget, 0, 0, 1, 1);
    layout->addWidget(labelvolume, 1, 0, 1, 1);
    layout->addWidget(slidervolume, 2, 0, 1, 1);
    layout->addWidget(level, 0, 1, 3, 1);

    linerecordpath = new QLineEdit(this);
    buttonsearch = new QPushButton(this);
    buttonrecord = new QPushButton(this);
    buttonrecordstop = new QPushButton(this);
    lcdtime = new QLCDNumber(this);
    boxautostart = new QCheckBox("Auto start recording when connected", this);

    QWidget *recorder = new QWidget(this);

    QGridLayout *layout_record = new QGridLayout(recorder);
    layout_record->addWidget(new QLabel("Wave file:", this), 0, 0);
    layout_record->addWidget(linerecordpath, 0, 1);
    layout_record->addWidget(buttonsearch, 0, 2);
    layout_record->addWidget(buttonrecord, 0, 3);
    layout_record->addWidget(buttonrecordstop, 0, 4);
    layout_record->addWidget(lcdtime, 1, 0, 1, 5);
    layout_record->addWidget(boxautostart, 2, 0, 1, 5);

    texteditlog = new QPlainTextEdit(this);
    debug_edit = texteditlog;

    tabwidget->addTab(areasettings, "Settings");
    tabwidget->addTab(analyzer, "Analyzer");
    tabwidget->addTab(listwidgetpeers, "Search");
    tabwidget->addTab(recorder, "Record");
    tabwidget->addTab(texteditsettings, "Info");
    tabwidget->addTab(texteditlog, "Log");

    connect(listwidgetpeers, &QListWidget::itemClicked, this, &MainWindow::selectPeer);

    connect(tabwidget, &QTabWidget::currentChanged, this, &MainWindow::currentChanged);

    linepassword->setEchoMode(QLineEdit::Password);
    buttonconnect->setText("Connect");
    texteditsettings->setReadOnly(true);
    slidervolume->setRange(0, 100);

    buttonconnect->setDefault(true);

    texteditlog->setReadOnly(true);

    connect(slidervolume, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    connect(linehost, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(lineport, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linetime, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linepassword, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(buttonconnect, &QPushButton::clicked, this, &MainWindow::start);

    connect(buttonsearch, &QPushButton::clicked, this, &MainWindow::setRecordPath);
    connect(buttonrecord, &QPushButton::clicked, this, &MainWindow::startPauseRecord);
    connect(buttonrecordstop, &QPushButton::clicked, this, &MainWindow::stopRecord);

    widgetsettings->setFixedHeight(widgetsettings->sizeHint().height());

    slidervolume->setValue(100);

    resetRecordPage();

    setCentralWidget(widget);

    linehost->setText("localhost");
    lineport->setText("1024");
    linetime->setText("300");

    getDevInfo();

    qInstallMessageHandler(messageHandler);
}

MainWindow::~MainWindow()
{
    qInstallMessageHandler(0);
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
    tabwidget->setCurrentIndex(0);
}

void MainWindow::start()
{
    if (m_audio_lib)
    {
        stopRecord();

        m_audio_lib->stop();
        return;
    }

    if (comboboxaudiooutput->count() == 0)
        return;

    comboboxaudiooutput->setEnabled(false);

    buttonconnect->setText("Disconnect");

    linehost->setEnabled(false);
    lineport->setEnabled(false);
    linetime->setEnabled(false);
    linepassword->setEnabled(false);
    tabwidget->setTabEnabled(2, false);

    buttonrecord->setEnabled(true);

    m_audio_lib = new AudioStreamingLibCore(this);

    connect(m_audio_lib, &AudioStreamingLibCore::destroyed, [&]{m_audio_lib = nullptr;});

    QByteArray password = linepassword->text().toLatin1();

    StreamingInfo info;

    QAudioDeviceInfo outputdevinfo = comboboxaudiooutput->currentData().value<QAudioDeviceInfo>();

    info.setWorkMode(StreamingInfo::StreamingWorkMode::BroadcastClient);
    info.setOutputDeviceInfo(outputdevinfo);
    info.setTimeToBuffer(linetime->text().toInt());
    info.setEncryptionEnabled(!password.isEmpty());
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
    str.append(QString("Sample type: %0\n").arg((inputFormat.sampleType()  == QAudioFormat::Float) ? "Float" : "Integer"));
    str.append(QString("Byte order: %0\n").arg((inputFormat.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    str.append("\n");

    str.append("Resampled format:\n\n");

    str.append(QString("Sample size: %0 bits\n").arg(format.sampleSize()));
    str.append(QString("Sample rate: %0 hz\n").arg(format.sampleRate()));
    str.append(QString("Channels: %0\n").arg(format.channelCount()));
    str.append(QString("Sample type: %0\n").arg((format.sampleType()  == QAudioFormat::Float) ? "Float" : "Integer"));
    str.append(QString("Byte order: %0\n").arg((format.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    str.append(QString("\nBuffer time: %0 ms").arg(m_audio_lib->streamingInfo().timeToBuffer()));

    texteditsettings->setPlainText(str);

    if (!m_spectrum_analyzer)
    {
        m_spectrum_analyzer = new SpectrumAnalyzer();
        QThread *thread = new QThread();
        m_spectrum_analyzer->moveToThread(thread);

        connect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, m_spectrum_analyzer, &SpectrumAnalyzer::calculateSpectrum);
        connect(m_spectrum_analyzer, &SpectrumAnalyzer::spectrumChanged, bars, &BarsWidget::setValues);
        connect(m_spectrum_analyzer, &SpectrumAnalyzer::destroyed, bars, &BarsWidget::clear);

        connect(m_audio_lib, &AudioStreamingLibCore::finished, m_spectrum_analyzer, &SpectrumAnalyzer::deleteLater);
        connect(this, &MainWindow::destroyed, m_spectrum_analyzer, &SpectrumAnalyzer::deleteLater);
        connect(m_spectrum_analyzer, &SpectrumAnalyzer::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        connect(m_spectrum_analyzer, &SpectrumAnalyzer::destroyed, [&]{m_spectrum_analyzer = nullptr;});

        QMetaObject::invokeMethod(m_spectrum_analyzer, "start", Qt::QueuedConnection, Q_ARG(QAudioFormat, format));

        thread->start();
    }

    {
        waveform->start(format);
        connect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, waveform, &WaveFormWidget::calculateWaveForm);
        connect(m_audio_lib, &AudioStreamingLibCore::finished, waveform, &WaveFormWidget::clear);
    }

    {
        connect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, level, &LevelWidget::calculateRMSLevel);
    }

    {
        connect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, level, &LevelWidget::calculateRMSLevel);
    }
}

void MainWindow::setRecordPath()
{
    QString path = QFileDialog::getSaveFileName(this, "Select the wave path", QString(), "WAVE (*.wav)");

    if (path.isEmpty())
        return;

    linerecordpath->setText(QDir::toNativeSeparators(path));
}

void MainWindow::startPauseRecord()
{
    if (m_paused)
    {
        if (!m_audio_recorder)
        {
            linerecordpath->setEnabled(false);
            buttonsearch->setEnabled(false);
            buttonrecordstop->setEnabled(true);

            m_format = m_audio_lib->audioFormat();
            //Change to compatible settings
            m_format.setSampleSize(16);
            m_format.setSampleType(QAudioFormat::SignedInt);

            m_audio_recorder = new AudioRecorder(linerecordpath->text(), m_format, m_audio_lib);

            connect(m_audio_recorder, &AudioRecorder::destroyed, [&]{m_audio_recorder = nullptr;});

            if (!m_audio_recorder->open())
            {
                stopRecord();
                QMessageBox::critical(this, "Error", "Error openning file for record!");
                return;
            }
        }

        connect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, this, &MainWindow::writeToFile);

        buttonrecord->setText("Pause");

        m_paused = false;
    }
    else
    {
        disconnect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, this, &MainWindow::writeToFile);

        buttonrecord->setText("Record");

        m_paused = true;
    }
}

void MainWindow::stopRecord()
{
    if (m_audio_recorder)
    {
        disconnect(m_audio_lib, &AudioStreamingLibCore::veryOutputData, this, &MainWindow::writeToFile);
        m_audio_recorder->deleteLater();
    }

    linerecordpath->setEnabled(true);
    buttonsearch->setEnabled(true);
    buttonrecord->setText("Record");
    buttonrecordstop->setEnabled(false);

    m_paused = true;

    lcdtime->display("--:--");

    m_format = QAudioFormat();
}

void MainWindow::resetRecordPage()
{
    buttonsearch->setText("...");
    buttonrecord->setText("Record");
    buttonrecordstop->setText("Stop record");

    buttonrecord->setEnabled(false);
    buttonrecordstop->setEnabled(false);

    lcdtime->display("--:--");
}

void MainWindow::writeToFile(const QByteArray &data)
{
    m_audio_recorder->write(AudioStreamingLibCore::convertFloatToInt16(data));

    qint64 recorded = AudioStreamingLibCore::sizeToTime(m_audio_recorder->size() - 44, m_format);

    QTime time = QTime(0, 0, 0).addMSecs(recorded);

    lcdtime->display(time.toString(Qt::ISODate));
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

    boxautostart->setEnabled(false);

    if (boxautostart->isChecked())
        startPauseRecord(); //Auto start recording when connected
}

void MainWindow::disconnected(const QHostAddress &address)
{
    Q_UNUSED(address)

    stopRecord();
    resetRecordPage();

    if (m_audio_lib)
        m_audio_lib->stop();

    boxautostart->setEnabled(true);

    setWindowTitle(TITLE);
}

void MainWindow::finished()
{
    if (!isVisible())
        return;

    stopRecord();
    resetRecordPage();

    boxautostart->setEnabled(true);

    buttonconnect->setText("Connect");

    linehost->setEnabled(true);
    lineport->setEnabled(true);
    linetime->setEnabled(true);
    linepassword->setEnabled(true);
    tabwidget->setTabEnabled(2, true);

    comboboxaudiooutput->setEnabled(true);

    texteditsettings->clear();

    if (m_audio_lib)
        m_audio_lib->deleteLater();

    setWindowTitle(TITLE);
}

void MainWindow::getDevInfo()
{
    QList<QAudioDeviceInfo> outputdevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    for (int i = 0; i < outputdevices.size(); i++)
        comboboxaudiooutput->addItem(outputdevices.at(i).deviceName(), qVariantFromValue(outputdevices.at(i)));

    if (comboboxaudiooutput->count() == 0)
        QMessageBox::warning(this, "Error", "No output device found!");
}
