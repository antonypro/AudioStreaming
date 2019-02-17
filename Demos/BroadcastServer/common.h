#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

static inline int msgBoxQuestion(const QString &title, const QString &message, QWidget *parent = nullptr)
{
    QMessageBox msgbox(parent);

#ifdef Q_OS_MACOS
    msgbox.setWindowModality(Qt::WindowModal);
#endif
    msgbox.setIcon(QMessageBox::Question);
    msgbox.setStandardButtons(QMessageBox::Yes);
    msgbox.addButton(QMessageBox::No);
    msgbox.setDefaultButton(QMessageBox::Yes);
    msgbox.setWindowTitle(title);
    msgbox.setText(message);

    return msgbox.exec();
}

static inline void msgBoxWarning(const QString &title, const QString &message, QWidget *parent = nullptr)
{
    QMessageBox msgbox(parent);

#ifdef Q_OS_MACOS
    msgbox.setWindowModality(Qt::WindowModal);
#endif
    msgbox.setIcon(QMessageBox::Warning);
    msgbox.setWindowTitle(title);
    msgbox.setText(message);

    msgbox.exec();
}

static inline void msgBoxCritical(const QString &title, const QString &message, QWidget *parent = nullptr)
{
    QMessageBox msgbox(parent);

#ifdef Q_OS_MACOS
    msgbox.setWindowModality(Qt::WindowModal);
#endif
    msgbox.setIcon(QMessageBox::Critical);
    msgbox.setWindowTitle(title);
    msgbox.setText(message);

    msgbox.exec();
}

static inline QString timeToString(qint64 ms)
{
    qint64 hh = ms / 3600000;
    ms = ms - 3600000 * hh;

    qint64 mm = ms / 60000;
    ms = ms - 60000 * mm;

    qint64 ss = ms / 1000;
    ms = ms - 1000 * ss;

    return QString("%0:%1:%2.%3")
            .arg(hh, 2, 10, QChar('0'))
            .arg(mm, 2, 10, QChar('0'))
            .arg(ss, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
}

#define BUFFERTIME 5 * 1000 //ms

#define SIZETOTIME(size) (size / (48 * 2 * int(sizeof(float))))

#define TIMETOSIZE(time) (time * (48 * 2 * int(sizeof(float))))

#endif // COMMON_H
