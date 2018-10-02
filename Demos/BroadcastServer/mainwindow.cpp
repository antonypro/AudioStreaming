#include "mainwindow.h"

#define TITLE "Broadcast Server Demo"

static QPlainTextEdit *debug_edit = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)

    if (debug_edit)
        QMetaObject::invokeMethod(debug_edit, "appendPlainText", Q_ARG(QString, msg));
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    qRegisterMetaType<QVector<SpectrumStruct> >("QVector<SpectrumStruct>");

    setWindowTitle(TITLE);

    m_audio_lib = nullptr;

    m_spectrum_analyzer = nullptr;

#ifdef Q_OS_WIN
    loopback = nullptr;
#endif

    m_audio_recorder = nullptr;

    m_paused = true;

    tabwidget = new QTabWidget(this);

    QWidget *widget = new QWidget(this);

    QGridLayout *layout = new QGridLayout(widget);

    QScrollArea *areasettings = new QScrollArea(this);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(Q_OS_WINPHONE)
    areasettings->setWidgetResizable(true);
    areasettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
#endif

    QWidget *widgetsettings = new QWidget(this);

    QGridLayout *layout1 = new QGridLayout(widgetsettings);

    comboboxaudioinput = new QComboBox(this);

    listconnections = new QListWidget(this);

    lineport = new QLineEdit(this);

    linemaxconnections = new QLineEdit(this);

    linesamplerate = new QLineEdit(this);

    linechannels = new QLineEdit(this);

    buttonstart = new QPushButton(this);
    buttonstart->setText("Start Server");

    lineid = new QLineEdit(this);

    linepassword = new QLineEdit(this);
    linepassword->setEchoMode(QLineEdit::Password);

    buttonstart->setDefault(true);

    texteditsettings = new QPlainTextEdit(this);

    layout1->addWidget(new QLabel("Input device:", this), 0, 0);
    layout1->addWidget(comboboxaudioinput, 0, 1, 1, 2);
    layout1->addWidget(new QLabel("Port:", this), 1, 0);
    layout1->addWidget(lineport, 1, 1);
    layout1->addWidget(buttonstart, 1, 2);
    layout1->addWidget(new QLabel("Maximum connections:", this), 2, 0);
    layout1->addWidget(linemaxconnections, 2, 1, 1, 2);
    layout1->addWidget(new QLabel("ID:", this), 3, 0);
    layout1->addWidget(lineid, 3, 1, 1, 2);
    layout1->addWidget(new QLabel("Password:", this), 4, 0);
    layout1->addWidget(linepassword, 4, 1, 1, 2);
    layout1->addWidget(new QLabel("Sample rate:", this), 5, 0);
    layout1->addWidget(linesamplerate, 5, 1, 1, 2);
    layout1->addWidget(new QLabel("Channels:", this), 6, 0);
    layout1->addWidget(linechannels, 6, 1, 1, 2);

    areasettings->setWidget(widgetsettings);

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

    linerecordpath = new QLineEdit(this);
    buttonsearch = new QPushButton(this);
    buttonrecord = new QPushButton(this);
    buttonrecordstop = new QPushButton(this);
    lcdtime = new QLCDNumber(this);
    boxautostart = new QCheckBox("Auto start recording when server starts", this);

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
    texteditlog->setMaximumBlockCount(10000);
    debug_edit = texteditlog;

    tabwidget->addTab(areasettings," Settings");
    tabwidget->addTab(analyzer, "Analyzer");
    tabwidget->addTab(listconnections, "Connections");
    tabwidget->addTab(recorder, "Record");
    tabwidget->addTab(texteditsettings, "Info");
    tabwidget->addTab(texteditlog, "Log");

    layout->addWidget(tabwidget, 0, 0, 1, 1);
    layout->addWidget(level, 0, 1, 1, 1);

    texteditsettings->setReadOnly(true);

    texteditlog->setReadOnly(true);

    connect(lineport, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linemaxconnections, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(lineid, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linepassword, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(buttonstart, &QPushButton::clicked, this, &MainWindow::start);

    connect(buttonsearch, &QPushButton::clicked, this, &MainWindow::setRecordPath);
    connect(buttonrecord, &QPushButton::clicked, this, &MainWindow::startPauseRecord);
    connect(buttonrecordstop, &QPushButton::clicked, this, &MainWindow::stopRecord);

    resetRecordPage();

    lineport->setText("1024");
    linemaxconnections->setText("10");

    widgetsettings->setFixedHeight(widgetsettings->sizeHint().height());

    setCentralWidget(widget);

    getDevInfo();

    qInstallMessageHandler(messageHandler);
}

MainWindow::~MainWindow()
{
    debug_edit = nullptr;
}

void MainWindow::start()
{
    if (m_audio_lib)
    {
        stopRecord();

        buttonrecord->setEnabled(false);

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
        msgBoxCritical("Error", "No input device found", this);
        return;
    }

    bool ok = false;
    int port = lineport->text().toInt(&ok);

    if (!ok || port < 1 || port > 65535)
    {
        msgBoxCritical("Error", "Port must have a value between 1 and 65535,\n"
                                "including these values", this);

        return;
    }

    int max_connections = linemaxconnections->text().toInt(&ok);

    if (!ok || max_connections < 1)
    {
        msgBoxCritical("Error", "Enter a value equal or higher to 1 on maximum connections", this);
        return;
    }

    m_audio_lib = new AudioStreamingLibCore(this);

    connect(m_audio_lib, &AudioStreamingLibCore::destroyed, [&]{m_audio_lib = nullptr;});

    StreamingInfo info;

    info.setWorkMode(StreamingInfo::StreamingWorkMode::BroadcastServer);
    info.setEncryptionEnabled(!password.isEmpty());
    info.setGetAudioEnabled(true);
    info.setNegotiationString(QByteArray("BroadcastTCPDemo"));
    info.setID(lineid->text().trimmed());

    QAudioDeviceInfo inputdevinfo = comboboxaudioinput->currentData().value<QAudioDeviceInfo>();

#ifdef Q_OS_WIN
    if (inputdevinfo.isNull())
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
        info.setInputDeviceInfo(inputdevinfo);

        QAudioFormat format;

        format.setSampleSize(32);
        format.setSampleRate(linesamplerate->text().toInt());
        format.setChannelCount(linechannels->text().toInt());
        format.setSampleType(QAudioFormat::Float);
        format.setByteOrder(QAudioFormat::LittleEndian);

        info.setInputAudioFormat(format);
    }

    lineport->setEnabled(false);
    linemaxconnections->setEnabled(false);
    lineid->setEnabled(false);
    linepassword->setEnabled(false);
    linesamplerate->setEnabled(false);
    linechannels->setEnabled(false);

    comboboxaudioinput->setEnabled(false);
    buttonstart->setText("Stop Server");

    buttonrecord->setEnabled(true);

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);
    connect(m_audio_lib, &AudioStreamingLibCore::inputLevel, level, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);
    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);
    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::updateConnections);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::updateConnections);

    m_audio_lib->start(info);

    m_audio_lib->listen(quint16(port), true, password, max_connections);

    boxautostart->setEnabled(false);

    if (boxautostart->isChecked())
        startPauseRecord(); //Auto start recording when server starts

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
    Q_UNUSED(data)
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
    Q_UNUSED(data)
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
    str.append(QString("Sample type: %0\n").arg((inputFormat.sampleType()  == QAudioFormat::Float) ? "Float" : "Integer"));
    str.append(QString("Byte order: %0\n").arg((inputFormat.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    str.append("\n");

    str.append("Resampled format:\n\n");

    str.append(QString("Sample size: %0 bits\n").arg(format.sampleSize()));
    str.append(QString("Sample rate: %0 hz\n").arg(format.sampleRate()));
    str.append(QString("Channels: %0\n").arg(format.channelCount()));
    str.append(QString("Sample type: %0\n").arg((format.sampleType()  == QAudioFormat::Float) ? "Float" : "Integer"));
    str.append(QString("Byte order: %0").arg((format.byteOrder() == QAudioFormat::LittleEndian) ? "Little endian" : "Big endian"));

    texteditsettings->setPlainText(str);

    if (!m_spectrum_analyzer)
    {
        m_spectrum_analyzer = new SpectrumAnalyzer();
        QThread *thread = new QThread();
        m_spectrum_analyzer->moveToThread(thread);

        connect(m_audio_lib, &AudioStreamingLibCore::veryInputData, m_spectrum_analyzer, &SpectrumAnalyzer::calculateSpectrum);
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
        connect(m_audio_lib, &AudioStreamingLibCore::veryInputData, waveform, &WaveFormWidget::calculateWaveForm);
        connect(m_audio_lib, &AudioStreamingLibCore::finished, waveform, &WaveFormWidget::clear);
    }

    {
        connect(m_audio_lib, &AudioStreamingLibCore::veryInputData, level, &LevelWidget::calculateRMSLevel);
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

                msgBoxCritical("Error", "Error openning file for record!", this);

                return;
            }
        }

        connect(m_audio_lib, &AudioStreamingLibCore::veryInputData, this, &MainWindow::writeToFile);

        buttonrecord->setText("Pause");

        m_paused = false;
    }
    else
    {
        disconnect(m_audio_lib, &AudioStreamingLibCore::veryInputData, this, &MainWindow::writeToFile);

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

    int recorded = int(AudioStreamingLibCore::sizeToTime(m_audio_recorder->size() - 44, m_format));

    QTime time = QTime(0, 0, 0).addMSecs(recorded);

    lcdtime->display(time.toString(Qt::ISODate));
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
        msgBoxCritical("Error", error, this);
}

void MainWindow::finished()
{
    if (!isVisible())
        return;

    stopRecord();
    resetRecordPage();

    boxautostart->setEnabled(true);

    lineport->setEnabled(true);
    linemaxconnections->setEnabled(true);
    lineid->setEnabled(true);
    linepassword->setEnabled(true);

    currentIndexChanged(comboboxaudioinput->currentIndex());

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

void MainWindow::currentIndexChanged(int index)
{
    bool disable = (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA && index == 0);

    if (disable)
    {
        linesamplerate->setEnabled(false);
        linechannels->setEnabled(false);

        linesamplerate->clear();
        linechannels->clear();
    }
    else
    {
        linesamplerate->setEnabled(true);
        linechannels->setEnabled(true);

        linesamplerate->setText("44100");
        linechannels->setText("2");
    }
}

void MainWindow::getDevInfo()
{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
        comboboxaudioinput->addItem("Loopback (What you hear)");

    QList<QAudioDeviceInfo> inputdevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for (int i = 0; i < inputdevices.size(); i++)
        comboboxaudioinput->addItem(inputdevices.at(i).deviceName(), qVariantFromValue(inputdevices.at(i)));

    if (comboboxaudioinput->count() == 0)
        msgBoxWarning("Error", "No input device found!", this);

    currentIndexChanged(comboboxaudioinput->currentIndex());

    connect(comboboxaudioinput,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::currentIndexChanged);
}
