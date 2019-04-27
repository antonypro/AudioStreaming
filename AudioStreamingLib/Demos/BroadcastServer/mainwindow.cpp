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
    qRegisterMetaType<QVector<SpectrumStruct> >("QVector<SpectrumStruct>");

    setWindowTitle(TITLE);

    m_total_size = 0;

    m_paused = true;

    initWidgets();

    qInstallMessageHandler(messageHandler);
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

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(Q_OS_WINPHONE)
    areasettings->setWidgetResizable(true);
    areasettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
#endif

#ifndef Q_OS_ANDROID
    m_ffmpeg = new FFMPEGClass(this);
#endif

    m_ffmpeg_terminating = false;

    m_ffmpeg_playing = false;

    QWidget *widgetsettings = new QWidget(this);

    QGridLayout *layout1 = new QGridLayout(widgetsettings);

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

    slidervolume->setRange(0, 100);

    buttonstart->setDefault(true);

    texteditsettings = new QPlainTextEdit(this);

    layout1->addWidget(new QLabel("Input device:", this), 0, 0);
    layout1->addWidget(comboboxaudioinput, 0, 1);
    layout1->addWidget(boxlisteninput, 0, 2);
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
    layout_record->addWidget(new QLabel("Audio file:", this), 0, 0);
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
    tabwidget->addTab(texteditlog, "Log");

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

    StreamingInfo info;

    info.setWorkMode(StreamingInfo::StreamingWorkMode::BroadcastServer);
    info.setEncryptionEnabled(!password.isEmpty());
    info.setGetAudioEnabled(true);
    info.setListenAudioInputEnabled(boxlisteninput->isChecked());
    info.setNegotiationString(QByteArray("BroadcastTCPDemo"));
    info.setID(lineid->text().trimmed());

    QAudioDeviceInfo inputdevinfo = comboboxaudioinput->currentData().value<QAudioDeviceInfo>();

    if (inputdevinfo.isNull())
    {
#ifdef Q_OS_WIN
        if (comboboxaudioinput->currentData(Qt::UserRole + 1).value<int>() == AudioInputInfo::Loopback)
        {
            m_loopback = new QWinLoopback(this);

            connect(m_loopback, &QObject::destroyed, this, [=]
            {
                if (isVisible())
                    m_buffer.clear();
            });

            bool started = m_loopback->start();

            if (!started)
                return;

            info.setInputDeviceType(StreamingInfo::AudioDeviceType::CustomAudioDevice);
            info.setCallBackEnabled(true);

            QAudioFormat format = m_loopback->format();

            connect(m_loopback, &QWinLoopback::readyRead, this, &MainWindow::loopbackdata);
            connect(m_audio_lib, &AudioStreamingLibCore::inputData, this, &MainWindow::process);

            info.setInputAudioFormat(format);
        }
        else //FFMPEG
#endif
        {
            QAudioFormat format;

            format.setSampleSize(32);
            format.setSampleRate(48000);
            format.setChannelCount(2);
            format.setSampleType(QAudioFormat::Float);
            format.setByteOrder(QAudioFormat::LittleEndian);

            info.setInputDeviceType(StreamingInfo::AudioDeviceType::CustomAudioDevice);
            info.setCallBackEnabled(true);

            connect(m_audio_lib, &AudioStreamingLibCore::inputData, this, &MainWindow::process);

            info.setInputAudioFormat(format);
        }
    }
    else
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
    boxlisteninput->setEnabled(false);
    buttonstart->setText("Stop Server");

    buttonrecord->setEnabled(true);

    connect(m_audio_lib, &AudioStreamingLibCore::adjustSettings, this, &MainWindow::adjustSettings);
    connect(m_audio_lib, &AudioStreamingLibCore::inputLevel, level, &LevelWidget::setlevel);
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

    bool loopback = (comboboxaudioinput->currentData(Qt::UserRole + 1).value<int>() == AudioInputInfo::Loopback);

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
    if (m_audio_recorder)
        m_audio_recorder->write(AudioStreamingLibCore::convertFloatToInt16(data));
    else if (m_audio_recorder_mp3)
        m_audio_recorder_mp3->encode(AudioStreamingLibCore::convertFloatToInt16(data));

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

    currentIndexChanged(comboboxaudioinput->currentIndex());

    boxlisteninput->setEnabled(true);
    comboboxaudioinput->setEnabled(true);
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

void MainWindow::currentIndexChanged(int index)
{
    //Disable some settings if in Loopback or FFMPEG mode
    bool disable = (comboboxaudioinput->itemData(index, Qt::UserRole + 1) != int(AudioInputInfo::Other));

#ifndef Q_OS_ANDROID
    bool enabled = (comboboxaudioinput->itemData(index, Qt::UserRole + 1) == int(AudioInputInfo::FFMPEG));
    tabwidget->setTabEnabled(1, enabled);
    if (!enabled)
        m_ffmpeg->clearPlayList();
#else
    tabwidget->setTabEnabled(1, false);
#endif

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
    {
        comboboxaudioinput->addItem("Loopback (What you hear)");
        comboboxaudioinput->setItemData(comboboxaudioinput->count() - 1, qVariantFromValue(int(AudioInputInfo::Loopback)), Qt::UserRole + 1);
    }

#ifndef Q_OS_ANDROID
    comboboxaudioinput->addItem("FFMPEG");
    comboboxaudioinput->setItemData(comboboxaudioinput->count() - 1, qVariantFromValue(int(AudioInputInfo::FFMPEG)), Qt::UserRole + 1);
#endif

    QList<QAudioDeviceInfo> inputdevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for (int i = 0; i < inputdevices.size(); i++)
    {
        comboboxaudioinput->addItem(inputdevices.at(i).deviceName(), qVariantFromValue(inputdevices.at(i)));
        comboboxaudioinput->setItemData(comboboxaudioinput->count() - 1, qVariantFromValue(int(AudioInputInfo::Other)), Qt::UserRole + 1);
    }

    if (comboboxaudioinput->count() == 0)
        msgBoxWarning("Error", "No input device found!", this);

    currentIndexChanged(comboboxaudioinput->currentIndex());

    connect(comboboxaudioinput,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::currentIndexChanged);
}
