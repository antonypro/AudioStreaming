#include "ffmpegclass.h"

#ifdef Q_OS_WIN
#define ffmpeg_path (QCoreApplication::applicationDirPath() + "/ffmpeg/ffmpeg.exe")
#define ffprobe_path (QCoreApplication::applicationDirPath() + "/ffmpeg/ffprobe.exe")
#else
#define ffmpeg_path "ffmpeg"
#define ffprobe_path "ffprobe"
#endif

FFMPEGClass::FFMPEGClass(QWidget *parent) : QWidget(parent)
{
    m_reading = false;
    m_running = false;
    m_paused_stopped = true;
    m_begin = 0;
    m_current_row = 0;

    initWidgets();

    int threads = QThreadPool::globalInstance()->maxThreadCount() + 1;
    QThreadPool::globalInstance()->setMaxThreadCount(threads);
}

FFMPEGClass::~FFMPEGClass()
{
    m_running = false;

    m_semaphore.release();

    m_thread.waitForFinished();

    int threads = QThreadPool::globalInstance()->maxThreadCount() - 1;
    QThreadPool::globalInstance()->setMaxThreadCount(threads);
}

void FFMPEGClass::initWidgets()
{
    play_pause = new QPushButton(this);
    stop = new QPushButton(this);
    repeat = new QPushButton(this);

    info = new QLabel(this);

    add = new QPushButton(this);
    remove = new QPushButton(this);

    media_list = new QListWidget(this);

    info->setStyleSheet("QLabel {color: blue;}");

    stop->setEnabled(false);
    repeat->setCheckable(true);
    media_list->setDragDropMode(QAbstractItemView::InternalMove);

    connect(play_pause, &QPushButton::clicked, this, &FFMPEGClass::playPausePrivate);

    connect(stop, &QPushButton::clicked, this, &FFMPEGClass::stopPrivate);

    connect(add, &QPushButton::clicked, this, &FFMPEGClass::addPrivate);

    connect(remove, &QPushButton::clicked, this, &FFMPEGClass::removePrivate);

    connect(media_list, &QListWidget::doubleClicked, this, &FFMPEGClass::doubleClickPrivate);

    connect(media_list->model(), &QAbstractItemModel::rowsMoved, this, &FFMPEGClass::rowsMoved);

    connect(this, &FFMPEGClass::finished, this, &FFMPEGClass::finishedPrivate);

    connect(this, &FFMPEGClass::decoded, this, &FFMPEGClass::decodedPrivate);

    play_pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    stop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    repeat->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    add->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    remove->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));

    QHBoxLayout *layouth = new QHBoxLayout();
    layouth->addWidget(play_pause);
    layouth->addWidget(stop);
    layouth->addWidget(repeat);
    layouth->addSpacing(10);
    layouth->addWidget(info);
    layouth->addSpacing(10);
    layouth->addStretch();
    layouth->addWidget(add);
    layouth->addWidget(remove);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addLayout(layouth);
    layout->addWidget(media_list);
}

void FFMPEGClass::read()
{
    if (m_paused_stopped)
        return;

    if (m_semaphore.available() > 0)
        return;

    m_semaphore.release();
}

void FFMPEGClass::clearPlayList()
{
    media_list->clear();
}

void FFMPEGClass::setReadingEnabled(bool enabled)
{
    m_reading = enabled;

    if (!m_reading)
        stopPrivate();
}

void FFMPEGClass::start()
{
    if (m_running)
        return;

    if (m_current_path.isEmpty())
        return;

    m_running = true;

    m_thread = QtConcurrent::run(this, &FFMPEGClass::readThread, m_current_path);
}

void FFMPEGClass::readThread(const QString &path)
{
    QProcess ffmpeg_app;
    QStringList ffmpeg_args;

    QProcess ffprobe_app;
    QStringList ffprobe_args;

    ffprobe_app.setProgram(ffprobe_path);

    ffprobe_args << "-i" << QDir::toNativeSeparators(path) << "-show_entries" << "format=duration" << "-v" << "quiet" << "-of" << "csv=p=0";

    ffprobe_app.setArguments(ffprobe_args);

    ffprobe_app.start();

    ffprobe_app.waitForStarted(-1);

    ffprobe_app.waitForFinished(-1);

    QString time = ffprobe_app.readAllStandardOutput();

    qint64 total_ms = qCeil(time.toDouble() * 1000);

    total_ms = qMax(total_ms, qint64(1));

    qint64 decoded_ms = 0;

    ffmpeg_app.setProgram(ffmpeg_path);

    ffmpeg_args << "-i" << QDir::toNativeSeparators(path) << "-f" << "f32le" << "-ar" << "48000" << "-ac" << "2" << "-vn" << "pipe:";

    ffmpeg_app.setArguments(ffmpeg_args);

    ffmpeg_app.start();

    ffmpeg_app.waitForStarted(-1);

    forever
    {
        if (!m_running)
            break;

        if (!ffmpeg_app.waitForReadyRead(-1))
            break;

        QByteArray data = ffmpeg_app.readAllStandardOutput();

        m_semaphore.acquire();

        if (!m_running)
            break;

        if (data.isEmpty())
            continue;

        decoded_ms += SIZETOTIME(data.size());

        emit decoded(decoded_ms, total_ms);

        emit rawAudio(data);
    }

    if (!m_running)
    {
        ffmpeg_app.terminate();
        ffmpeg_app.waitForFinished(-1);

        return;
    }

    ffmpeg_app.waitForFinished(-1);

    QByteArray debug_info = ffmpeg_app.readAllStandardError();

    if (!debug_info.isEmpty())
        qDebug() << qPrintable(debug_info);

    m_running = false;

    emit finished();
}

bool FFMPEGClass::testFFMPEG()
{
    QProcess ffmpeg_process;
    QProcess ffprobe_process;

    ffmpeg_process.setProgram(ffmpeg_path);
    ffprobe_process.setProgram(ffprobe_path);

    ffmpeg_process.start();
    ffprobe_process.start();

    if (!ffmpeg_process.waitForStarted(-1) || !ffprobe_process.waitForStarted(-1))
    {
        ffmpeg_process.terminate();
        ffprobe_process.terminate();

        ffmpeg_process.waitForFinished(-1);
        ffprobe_process.waitForFinished(-1);

        return false;
    }

    ffmpeg_process.terminate();
    ffprobe_process.terminate();

    ffmpeg_process.waitForFinished(-1);
    ffprobe_process.waitForFinished(-1);

    return true;
}

void FFMPEGClass::playPausePrivate()
{
    if (!m_reading)
    {
        msgBoxCritical("Error", "Server not consuming audio\nMaybe not started", this);
        return;
    }

    if (!testFFMPEG())
    {
        msgBoxCritical("Error", "FFMPEG not found", this);
        return;
    }

    if (m_paused_stopped)
    {
        if (!m_running)
        {
            if (media_list->count() == 0)
            {
                msgBoxCritical("Error", "No file to play", this);
                return;
            }

            stop->setEnabled(true);

            m_current_row = qMax(media_list->currentRow(), 0);

            media_list->setCurrentRow(m_current_row);

            QListWidgetItem *item = media_list->item(m_current_row);

            m_current_path = item->data(Qt::UserRole).toString();

            item->setSelected(true);

            item->setForeground(Qt::blue);

            start();
        }

        m_paused_stopped = false;

        emit mediaPlay();

        play_pause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
    else
    {
        m_paused_stopped = true;

        emit mediaPause();

        play_pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void FFMPEGClass::stopPrivate()
{
    if (!m_running)
        return;

    m_paused_stopped = true;
    m_begin = 0;

    m_running = false;

    m_semaphore.release();

    m_thread.waitForFinished();

    QListWidgetItem *item = media_list->item(m_current_row);

    item->setForeground(QApplication::palette().text().color());

    m_current_path = QString();
    m_current_row = 0;

    stop->setEnabled(false);

    info->clear();

    emit mediaStop();

    play_pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void FFMPEGClass::addPrivate()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, "Add file(s) to list",
                                                      QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                                      "Media files(*.*)");

    for (int i = 0; i < paths.size(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QFileInfo(paths[i]).fileName());
        item->setData(Qt::UserRole, paths[i]);

        media_list->addItem(item);
    }
}

void FFMPEGClass::removePrivate()
{
    QListWidgetItem *item = media_list->item(m_current_row);

    QList<QListWidgetItem*> selected = media_list->selectedItems();

    if (selected.contains(item))
        stopPrivate();

    for (QListWidgetItem *item : selected)
    {
        media_list->removeItemWidget(item);
        delete item;
    }

    if (!selected.contains(item))
        m_current_row = media_list->row(item);
}

void FFMPEGClass::doubleClickPrivate(const QModelIndex &index)
{
    Q_UNUSED(index)

    stopPrivate();
    playPausePrivate();
}

void FFMPEGClass::rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                            const QModelIndex &destinationParent, int destinationRow)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)

    if (sourceStart == m_current_row)
    {
        if (destinationRow > sourceStart)
            destinationRow--;

        m_current_row = destinationRow;
    }
    else if ((sourceStart < m_current_row) && (destinationRow > m_current_row))
    {
        m_current_row--;
    }
    else if ((sourceStart > m_current_row) && (destinationRow <= m_current_row))
    {
        m_current_row++;
    }

    m_current_row = qBound(0, m_current_row, media_list->count() - 1);
}

void FFMPEGClass::finishedPrivate()
{
    m_begin = 0;

    QListWidgetItem *item = media_list->item(m_current_row);
    QListWidgetItem *next_item = media_list->item(m_current_row + 1);

    if (!next_item && repeat->isChecked())
    {
        m_current_row = -1;
        next_item = media_list->item(m_current_row + 1);
    }

    info->clear();

    if (next_item)
    {
        item->setForeground(QApplication::palette().text().color());

        m_current_path = next_item->data(Qt::UserRole).toString();
        next_item->setSelected(true);
        media_list->setCurrentRow(m_current_row + 1);
        next_item->setSelected(true);
        next_item->setForeground(Qt::blue);

        m_current_row++;

        start();
    }
    else
    {
        stop->setEnabled(false);
        play_pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        m_current_path = QString();
        if (item) item->setForeground(QApplication::palette().text().color());
        media_list->setCurrentRow(0);
        m_paused_stopped = true;
        m_current_row = 0;

        emit allFinished();
    }
}

void FFMPEGClass::decodedPrivate(qint64 decoded_ms, qint64 total_ms)
{
    int percent = int(decoded_ms * 100 / total_ms);

    QString info_str = QString("Decoded: %0 / %1 - %2%").arg(timeToString(decoded_ms)).arg(timeToString(total_ms)).arg(percent);

    info->setText(info_str);
}
