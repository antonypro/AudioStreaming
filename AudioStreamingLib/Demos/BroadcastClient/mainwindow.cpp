#include "mainwindow.h"

#define TITLE "Broadcast Client Demo"

static QPlainTextEdit *debug_edit = nullptr;
static QMutex debug_mutex;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)

    QMutexLocker locker(&debug_mutex);

    if (debug_edit)
        QMetaObject::invokeMethod(debug_edit, "appendPlainText", Q_ARG(QString, msg));
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    qRegisterMetaType<QVector<SpectrumStruct>>("QVector<SpectrumStruct>");

    setWindowTitle(TITLE);

    m_discover_instance = new AudioStreamingLibCore(this);

    m_total_size = 0;

    initWidgets();
}

MainWindow::~MainWindow()
{
    QMutexLocker locker(&debug_mutex);

    debug_edit = nullptr;
}

void MainWindow::initWidgets()
{
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

#ifndef Q_OS_ANDROID
    areasettings->setWidgetResizable(true);
    areasettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
#endif

#ifdef Q_OS_ANDROID
    areasettings->setWidgetResizable(true);

    areasettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    areasettings->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QScroller::grabGesture(areasettings, QScroller::LeftMouseButtonGesture);
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
    layout_record->addWidget(new QLabel("Audio file:", this), 0, 0);
    layout_record->addWidget(linerecordpath, 0, 1);
    layout_record->addWidget(buttonsearch, 0, 2);
    layout_record->addWidget(buttonrecord, 0, 3);
    layout_record->addWidget(buttonrecordstop, 0, 4);
    layout_record->addWidget(lcdtime, 1, 0, 1, 5);
    layout_record->addWidget(boxautostart, 2, 0, 1, 5);

    QWidget *log = new QWidget(this);

    texteditlog = new QPlainTextEdit(this);
    texteditlog->setMaximumBlockCount(10000);

    boxlogtowidget = new QCheckBox("Log to widget", this);
    buttonclearlog = new QPushButton("Clear", this);

    debug_edit = texteditlog;

    QHBoxLayout *layout_log_widgets = new QHBoxLayout();
    layout_log_widgets->addWidget(boxlogtowidget);
    layout_log_widgets->addWidget(buttonclearlog);
    layout_log_widgets->addStretch();

    QVBoxLayout *layout_log = new QVBoxLayout(log);
    layout_log->setMargin(0);
    layout_log->addWidget(texteditlog);
    layout_log->addLayout(layout_log_widgets);

#ifdef Q_OS_ANDROID
    texteditlog->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    texteditlog->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QScroller::grabGesture(texteditlog, QScroller::LeftMouseButtonGesture);
#endif

    tabwidget->addTab(areasettings, "Settings");
    tabwidget->addTab(analyzer, "Analyzer");
    tabwidget->addTab(listwidgetpeers, "Search");
    tabwidget->addTab(recorder, "Record");
    tabwidget->addTab(texteditsettings, "Info");
    tabwidget->addTab(log, "Log");

    connect(listwidgetpeers, &QListWidget::itemClicked, this, &MainWindow::selectPeer);

    connect(tabwidget, &QTabWidget::currentChanged, this, &MainWindow::currentChanged);

    linepassword->setEchoMode(QLineEdit::Password);
    buttonconnect->setText("Connect");
    texteditsettings->setReadOnly(true);
    slidervolume->setRange(0, 150);

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

    connect(boxlogtowidget, &QCheckBox::clicked, [=](bool checked){
        qInstallMessageHandler(checked ? messageHandler : nullptr);
    });

    connect(buttonclearlog, &QCheckBox::clicked, texteditlog, &QPlainTextEdit::clear);

    widgetsettings->setFixedHeight(widgetsettings->sizeHint().height());

    slidervolume->setValue(100);

    resetRecordPage();

    setCentralWidget(widget);

    linehost->setText("localhost");
    lineport->setText("1024");
    linetime->setText("0");

    getDevInfo();
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

    instance->discover(quint16(lineport->text().toInt()), QByteArray("BroadcastTCPDemo"));
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

    buttonconnect->setText("Disconnect");

    linehost->setEnabled(false);
    lineport->setEnabled(false);
    linetime->setEnabled(false);
    linepassword->setEnabled(false);
    tabwidget->setTabEnabled(2, false);

    buttonrecord->setEnabled(true);

    m_audio_lib = new AudioStreamingLibCore(this);

    QByteArray password = linepassword->text().toLatin1();

    AudioStreamingLibInfo info;

    QAudioDeviceInfo outputdevinfo = comboboxaudiooutput->currentData().value<QAudioDeviceInfo>();

    info.setWorkMode(AudioStreamingLibInfo::StreamingWorkMode::BroadcastClient);
    info.setOutputDeviceInfo(outputdevinfo);
    info.setTimeToBuffer(linetime->text().toInt());
    info.setEncryptionEnabled(!password.isEmpty());
    info.setGetAudioEnabled(true);
    info.setNegotiationString("BroadcastTCPDemo");

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);
    connect(m_audio_lib, &AudioStreamingLibCore::outputLevel, level, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::warning, this, &MainWindow::warning);
    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);
    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::connected);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::disconnected);
    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);

    m_audio_lib->start(info);

    m_audio_lib->setVolume(slidervolume->value());

    m_audio_lib->connectToHost(linehost->text().trimmed(), quint16(lineport->text().toInt()), password);
}

void MainWindow::adjustSettings()
{
    QAudioFormat inputFormat = m_audio_lib->inputAudioFormat();
    QAudioFormat format = m_audio_lib->audioFormat();
    qint32 bufferTime = m_audio_lib->audioStreamingLibInfo().timeToBuffer();

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

    str.append(QString("\nBuffer time: %0 ms%1").arg(bufferTime).arg(bufferTime == 0 ? ("(Smart buffer)") : QString()));

    str.append("\n\n");

    str.append("Eigen instructions set:\n");

    str.append(AudioStreamingLibCore::EigenInstructionsSet());

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
    QString selected_filter;
    QString path = QFileDialog::getSaveFileName(this, "Select the audio path", QString(),
                                                "WAVE (*.wav);;MP3 (*.mp3)", &selected_filter,
                                                QFileDialog::DontConfirmOverwrite);

    if (path.isEmpty())
        return;

    if (selected_filter == "WAVE (*.wav)" && !path.endsWith(".wav", Qt::CaseInsensitive))
        path.append(".wav");
    else if (selected_filter == "MP3 (*.mp3)" && !path.endsWith(".mp3", Qt::CaseInsensitive))
        path.append(".mp3");

    linerecordpath->setText(QDir::toNativeSeparators(path));
}

void MainWindow::startPauseRecord()
{
    if (!m_audio_recorder && !m_audio_recorder_mp3)
    {
        if (QFileInfo(linerecordpath->text()).exists())
        {
            int result = msgBoxQuestion("File already exists",
                                        QString("File %0 already exists. Replace it?")
                                        .arg(QFileInfo(linerecordpath->text()).fileName()), this);

            if (result != QMessageBox::Yes)
                return;
        }
    }

    if (m_paused)
    {
        linerecordpath->setEnabled(false);
        buttonsearch->setEnabled(false);
        buttonrecordstop->setEnabled(true);

        m_format = m_audio_lib->audioFormat();
        //Change to compatible settings
        m_format.setSampleSize(16);
        m_format.setSampleType(QAudioFormat::SignedInt);

        if (!linerecordpath->text().endsWith(".mp3"))
        {
            if (!m_audio_recorder)
            {
                m_audio_recorder = new AudioRecorder(linerecordpath->text(), m_format, m_audio_lib);

                if (!m_audio_recorder->open())
                {
                    stopRecord();

                    msgBoxCritical("Error", "Error openning wave file for record!", this);

                    return;
                }
            }
        }
        else
        {
            if (!m_audio_recorder_mp3)
            {
                m_audio_recorder_mp3 = new MP3Recorder(this);

                if (!m_audio_recorder_mp3->start(linerecordpath->text(), m_format, 128))
                {
                    stopRecord();

                    msgBoxCritical("Error", "Error openning mp3 file for record!", this);

                    return;
                }
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
        disconnect(m_audio_lib, &AudioStreamingLibCore::veryInputData, this, &MainWindow::writeToFile);
        m_audio_recorder->deleteLater();
    }
    else if (m_audio_recorder_mp3)
    {
        disconnect(m_audio_lib, &AudioStreamingLibCore::veryInputData, this, &MainWindow::writeToFile);
        m_audio_recorder_mp3->deleteLater();
    }

    m_total_size = 0;

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
    if (m_paused)
        return;

    if (m_audio_recorder)
        m_audio_recorder->write(AudioStreamingLibCore::convertFloatToInt16(data));
    else if (m_audio_recorder_mp3)
        m_audio_recorder_mp3->encode(AudioStreamingLibCore::convertFloatToInt16(data));
    else
        return;

    m_total_size += data.size() / int(sizeof(float) / sizeof(qint16));

    int recorded = int(AudioStreamingLibCore::sizeToTime(m_total_size, m_format));

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

void MainWindow::warning(const QString &warning)
{
    if (!warning.isEmpty())
        msgBoxCritical("Warning", warning, this);
}

void MainWindow::error(const QString &error)
{
    if (!error.isEmpty())
        msgBoxCritical("Error", error, this);
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

    texteditsettings->clear();

    if (m_audio_lib)
        m_audio_lib->deleteLater();

    setWindowTitle(TITLE);
}

void MainWindow::currentIndexChanged(int index)
{
    if (!m_audio_lib)
        return;

    QAudioDeviceInfo dev_info = comboboxaudiooutput->itemData(index).value<QAudioDeviceInfo>();

    m_audio_lib->changeOutputDevice(dev_info);
}

void MainWindow::getDevInfo()
{
    QList<QAudioDeviceInfo> outputdevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    for (int i = 0; i < outputdevices.size(); i++)
        comboboxaudiooutput->addItem(outputdevices.at(i).deviceName(), qVariantFromValue(outputdevices.at(i)));

    if (comboboxaudiooutput->count() == 0)
        comboboxaudiooutput->setEnabled(false);

    connect(comboboxaudiooutput, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::currentIndexChanged);
}
