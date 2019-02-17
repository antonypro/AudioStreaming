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

#endif // COMMON_H
