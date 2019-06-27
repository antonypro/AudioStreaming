#include "mainwindow.h"

#define TITLE "Broadcast Server Demo"

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

    m_total_size = 0;

    m_paused = true;

    initWidgets();
}

MainWindow::~MainWindow()
{
    QMutexLocker locker(&debug_mutex);

    debug_edit = nullptr;
}

void MainWindow::initWidgets()
{
    tabwidget = new QTabWidget(this);

    QWidget *widget = new QWidget(this);

    QGridLayout *layout = new QGridLayout(widget);

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

#ifndef Q_OS_ANDROID
    m_ffmpeg = new FFMPEGClass(this);
#endif

    m_ffmpeg_terminating = false;

    m_ffmpeg_playing = false;

    QWidget *widgetsettings = new QWidget(this);

    QGridLayout *layout1 = new QGridLayout(widgetsettings);

    buttonloopbackaudioinput = new QRadioButton(this);
    buttonloopbackaudioinput->setText("Loopback (What you hear)");

    buttonffmpegaudioinput = new QRadioButton(this);
    buttonffmpegaudioinput->setText("FFMPEG");

    buttonaudioinput = new QRadioButton(this);
    buttonaudioinput->setText("Library input audio device");

    comboboxaudioinput = new QComboBox(this);

    boxlisteninput = new QCheckBox(this);
    boxlisteninput->setText("Listen");

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

    labelvolume = new QLabel(this);
    slidervolume = new QSlider(Qt::Horizontal, this);

    groupboxtype = new QGroupBox(this);

    QVBoxLayout *vbox = new QVBoxLayout();

    vbox->addWidget(buttonloopbackaudioinput);
    vbox->addWidget(buttonffmpegaudioinput);
    vbox->addWidget(buttonaudioinput);

    groupboxtype->setLayout(vbox);

#ifndef Q_OS_WIN
    buttonloopbackaudioinput->setEnabled(false);
#endif

#ifdef Q_OS_ANDROID
    buttonffmpegaudioinput->setEnabled(false);
#endif

    slidervolume->setRange(0, 150);

    buttonstart->setDefault(true);

    texteditsettings = new QPlainTextEdit(this);

    layout1->addWidget(new QLabel("Input type:", this), 0, 0);
    layout1->addWidget(groupboxtype, 0, 1, 1, 2);
    layout1->addWidget(new QLabel("Input device:", this), 1, 0);
    layout1->addWidget(comboboxaudioinput, 1, 1);
    layout1->addWidget(boxlisteninput, 1, 2);
    layout1->addWidget(new QLabel("Port:", this), 2, 0);
    layout1->addWidget(lineport, 2, 1);
    layout1->addWidget(buttonstart, 2, 2);
    layout1->addWidget(new QLabel("Maximum connections:", this), 3, 0);
    layout1->addWidget(linemaxconnections, 3, 1, 1, 2);
    layout1->addWidget(new QLabel("ID:", this), 4, 0);
    layout1->addWidget(lineid, 4, 1, 1, 2);
    layout1->addWidget(new QLabel("Password:", this), 5, 0);
    layout1->addWidget(linepassword, 5, 1, 1, 2);
    layout1->addWidget(new QLabel("Sample rate:", this), 6, 0);
    layout1->addWidget(linesamplerate, 6, 1, 1, 2);
    layout1->addWidget(new QLabel("Channels:", this), 7, 0);
    layout1->addWidget(linechannels, 7, 1, 1, 2);

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

    tabwidget->addTab(areasettings," Settings");
#ifndef Q_OS_ANDROID
    tabwidget->addTab(m_ffmpeg, "FFMPEG");
#else
    tabwidget->addTab(new QWidget(this), "FFMPEG");
    tabwidget->setTabEnabled(1, false);
#endif
    tabwidget->addTab(analyzer, "Analyzer");
    tabwidget->addTab(listconnections, "Connections");
    tabwidget->addTab(recorder, "Record");
    tabwidget->addTab(texteditsettings, "Info");
    tabwidget->addTab(log, "Log");

    layout->addWidget(tabwidget, 0, 0, 1, 1);
    layout->addWidget(labelvolume, 1, 0, 1, 1);
    layout->addWidget(slidervolume, 2, 0, 1, 1);
    layout->addWidget(level, 0, 1, 3, 1);

    slidervolume->setEnabled(false);

    texteditsettings->setReadOnly(true);

    texteditlog->setReadOnly(true);

    connect(boxlisteninput, &QCheckBox::clicked, this, &MainWindow::boxListenInputClicked);

    connect(slidervolume, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    connect(lineport, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linemaxconnections, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(lineid, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(linepassword, &QLineEdit::returnPressed, this, &MainWindow::start);
    connect(buttonstart, &QPushButton::clicked, this, &MainWindow::start);

    connect(buttonsearch, &QPushButton::clicked, this, &MainWindow::setRecordPath);
    connect(buttonrecord, &QPushButton::clicked, this, &MainWindow::startPauseRecord);
    connect(buttonrecordstop, &QPushButton::clicked, this, &MainWindow::stopRecord);

    connect(boxlogtowidget, &QCheckBox::clicked, [=](bool checked){
        qInstallMessageHandler(checked ? messageHandler : nullptr);
    });

    connect(buttonclearlog, &QCheckBox::clicked, texteditlog, &QPlainTextEdit::clear);

    resetRecordPage();

#ifndef Q_OS_ANDROID
    connect(m_ffmpeg, &FFMPEGClass::rawAudio, this, &MainWindow::ffmpegdata);

    connect(m_ffmpeg, &FFMPEGClass::mediaPlay, this, &MainWindow::ffmpegplay);

    connect(m_ffmpeg, &FFMPEGClass::mediaPause, this, &MainWindow::ffmpegpause);

    connect(m_ffmpeg, &FFMPEGClass::mediaStop, this, &MainWindow::ffmpegstop);

    connect(m_ffmpeg, &FFMPEGClass::allFinished, this, &MainWindow::ffmpegallfinished);
#endif

    lineport->setText("1024");
    linemaxconnections->setText("10");

    slidervolume->setValue(100);

    widgetsettings->setFixedHeight(widgetsettings->sizeHint().height());

    setCentralWidget(widget);

    getDevInfo();

    connect(buttonloopbackaudioinput, &QRadioButton::clicked, this, &MainWindow::deviceTypeChanged);

    connect(buttonffmpegaudioinput, &QRadioButton::clicked, this, &MainWindow::deviceTypeChanged);

    connect(buttonaudioinput, &QRadioButton::clicked, this, &MainWindow::deviceTypeChanged);

    buttonaudioinput->setChecked(true);

    deviceTypeChanged();
}

void MainWindow::start()
{
    if (m_audio_lib)
    {
        stopRecord();

        buttonrecord->setEnabled(false);

        m_audio_lib->stop();
#ifdef Q_OS_WIN
        if (m_loopback)
            m_loopback->deleteLater();
#endif
        return;
    }

    if (buttonaudioinput->isChecked() && comboboxaudioinput->count() == 0)
        return;

    QByteArray password = linepassword->text().toLatin1();

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

    AudioStreamingLibInfo info;

    info.setWorkMode(AudioStreamingLibInfo::StreamingWorkMode::BroadcastServer);
    info.setEncryptionEnabled(!password.isEmpty());
    info.setGetAudioEnabled(true);
    info.setListenAudioInputEnabled(boxlisteninput->isChecked());
    info.setNegotiationString("BroadcastTCPDemo");
    info.setID(lineid->text().trimmed());

    if (buttonloopbackaudioinput->isChecked())
    {
#ifdef Q_OS_WIN
        m_loopback = new QWinLoopback(this);
        
        connect(m_loopback, &QObject::destroyed, this, [=]
        {
            if (isVisible())
                m_buffer.clear();
        });
        
        bool started = m_loopback->start();
        
        if (!started)
            return;
        
        info.setInputDeviceType(AudioStreamingLibInfo::AudioDeviceType::CustomAudioDevice);
        info.setCallBackEnabled(true);
        
        QAudioFormat format = m_loopback->format();
        
        connect(m_loopback, &QWinLoopback::readyRead, this, &MainWindow::loopbackdata);
        connect(m_audio_lib, &AudioStreamingLibCore::inputData, this, &MainWindow::process);
        
        info.setInputAudioFormat(format);
#endif
    }
    else if (buttonffmpegaudioinput->isChecked())
    {
        QAudioFormat format;
        
        format.setSampleSize(32);
        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setSampleType(QAudioFormat::Float);
        format.setByteOrder(QAudioFormat::LittleEndian);

        info.setInputDeviceType(AudioStreamingLibInfo::AudioDeviceType::CustomAudioDevice);
        info.setCallBackEnabled(true);
        
        connect(m_audio_lib, &AudioStreamingLibCore::inputData, this, &MainWindow::process);
        
        info.setInputAudioFormat(format);
    }
    else
    {
        QAudioDeviceInfo dev_info = comboboxaudioinput->currentData().value<QAudioDeviceInfo>();

        info.setInputDeviceInfo(dev_info);

        QAudioFormat format;

        format.setSampleSize(32);
        format.setSampleRate(linesamplerate->text().toInt());
        format.setChannelCount(linechannels->text().toInt());
        format.setSampleType(QAudioFormat::Float);
        format.setByteOrder(QAudioFormat::LittleEndian);

        info.setInputAudioFormat(format);
    }

    groupboxtype->setEnabled(false);

    lineport->setEnabled(false);
    linemaxconnections->setEnabled(false);
    lineid->setEnabled(false);
    linepassword->setEnabled(false);
    linesamplerate->setEnabled(false);
    linechannels->setEnabled(false);

    boxlisteninput->setEnabled(false);
    buttonstart->setText("Stop Server");

    buttonrecord->setEnabled(true);

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);
    connect(m_audio_lib, &AudioStreamingLibCore::inputLevel, level, &LevelWidget::setlevel);
    connect(m_audio_lib, &AudioStreamingLibCore::warning, this, &MainWindow::warning);
    connect(m_audio_lib, &AudioStreamingLibCore::error, this, &MainWindow::error);
    connect(m_audio_lib, &AudioStreamingLibCore::finished, this, &MainWindow::finished);
    connect(m_audio_lib, &AudioStreamingLibCore::connected, this, &MainWindow::updateConnections);
    connect(m_audio_lib, &AudioStreamingLibCore::disconnected, this, &MainWindow::updateConnections);

    m_audio_lib->start(info);

    if (info.isListenAudioInputEnabled())
        m_audio_lib->setVolume(slidervolume->value());

    m_audio_lib->listen(quint16(port), true, password, max_connections);

    boxautostart->setEnabled(false);

    if (boxautostart->isChecked())
        startPauseRecord(); //Auto start recording when server starts

#ifndef Q_OS_ANDROID
    m_ffmpeg->setReadingEnabled(true);
#endif

    QString title = QString("%0 connection(s) - %1").arg(0).arg(TITLE);

    setWindowTitle(title);
}

void MainWindow::loopbackdata(const QByteArray &data)
{
#ifdef Q_OS_WIN
    if (!m_loopback)
        return;

    m_buffer.append(data);
#else
    Q_UNUSED(data)
#endif
}

void MainWindow::ffmpegdata(const QByteArray &data)
{
    if (!m_audio_lib || !m_audio_lib->isRunning())
       return;

    m_buffer.append(data);
}

void MainWindow::ffmpegplay()
{
    m_ffmpeg_terminating = false;
    m_ffmpeg_playing = true;
}

void MainWindow::ffmpegpause()
{
    m_ffmpeg_playing = false;
}

void MainWindow::ffmpegstop()
{
    m_ffmpeg_playing = false;
    m_buffer.clear();
}

void MainWindow::ffmpegallfinished()
{
    m_ffmpeg_terminating = true;
}

void MainWindow::process(const QByteArray &data)
{
    if (!m_audio_lib || !m_audio_lib->isRunning())
       return;

    bool loopback = buttonloopbackaudioinput->isChecked();

    int size = data.size();

    QByteArray mid = data;

    if (loopback || m_ffmpeg_playing)
    {
        if (m_buffer.size() >= size)
        {
            mid = m_buffer.mid(0, size);
            m_buffer.remove(0, size);
        }
        else if (m_ffmpeg_terminating)
        {
            m_ffmpeg_terminating = false;
            m_ffmpeg_playing = false;
        }
    }

#ifndef Q_OS_ANDROID
    if (m_ffmpeg_playing && SIZETOTIME(m_buffer.size()) < 1000)
        m_ffmpeg->read();
#endif

    m_audio_lib->inputDataBack(mid);
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

    str.append("\n\n");

    str.append("Eigen instructions set:\n");

    str.append(AudioStreamingLibCore::EigenInstructionsSet());

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

void MainWindow::updateConnections()
{
    QList<QHostAddress> connections = m_audio_lib->connectionsList();

    listconnections->clear();

    for (const QHostAddress &addr : connections)
        listconnections->addItem(addr.toString());

    QString title = QString("%0 connection(s) - %1").arg(connections.size()).arg(TITLE);

    setWindowTitle(title);
}

void MainWindow::boxListenInputClicked(bool checked)
{
    slidervolume->setEnabled(checked);

    if (!checked)
        slidervolume->setValue(100);
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
        msgBoxWarning("Warning", warning, this);
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

#ifndef Q_OS_ANDROID
    m_ffmpeg->setReadingEnabled(false);
#endif

    stopRecord();
    resetRecordPage();

    boxautostart->setEnabled(true);

    lineport->setEnabled(true);
    linemaxconnections->setEnabled(true);
    lineid->setEnabled(true);
    linepassword->setEnabled(true);

    groupboxtype->setEnabled(true);

    boxlisteninput->setEnabled(true);
    buttonstart->setText("Start Server");

    texteditsettings->clear();

    listconnections->clear();

    if (m_audio_lib)
        m_audio_lib->deleteLater();

    m_buffer.clear();

    setWindowTitle(TITLE);

#ifdef Q_OS_WIN
        if (m_loopback)
            m_loopback->deleteLater();
#endif
}

void MainWindow::deviceTypeChanged()
{
    comboboxaudioinput->setEnabled(buttonaudioinput->isChecked());

    //Disable some settings if in Loopback or FFMPEG mode
    bool disable = (!buttonaudioinput->isChecked());

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

#ifndef Q_OS_ANDROID
    bool enabled = buttonffmpegaudioinput->isEnabled();
    tabwidget->setTabEnabled(1, enabled);
    if (!enabled)
        m_ffmpeg->clearPlayList();
#else
    tabwidget->setTabEnabled(1, false);
#endif
}

void MainWindow::currentIndexChanged(int index)
{
    if (!m_audio_lib)
        return;

    QAudioDeviceInfo dev_info = comboboxaudioinput->itemData(index).value<QAudioDeviceInfo>();

    m_audio_lib->changeInputDevice(dev_info);
}

void MainWindow::getDevInfo()
{
    QList<QAudioDeviceInfo> inputdevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for (int i = 0; i < inputdevices.size(); i++)
        comboboxaudioinput->addItem(inputdevices.at(i).deviceName(), qVariantFromValue(inputdevices.at(i)));

    if (comboboxaudioinput->count() == 0)
        comboboxaudioinput->setEnabled(false);

    connect(comboboxaudioinput, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::currentIndexChanged);
}
