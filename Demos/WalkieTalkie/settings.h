#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtQml>
#include <QtUiTools>
#include "common.h"

class Settings : public QWidget
{
    Q_OBJECT
public:
    explicit Settings(QWidget *parent = nullptr);

signals:
    void commandXML(QByteArray);
    void logOut();
    void loggedIn();
    void loggedOut();

public slots:
    void loadPage(const QByteArray &ui, const QByteArray &js);
    void processCommandXML(const QByteArray &data);

    QVariant readProperty(const QVariant &name, const QVariant &property_name);
    QVariant writeProperty(const QVariant &name, const QVariant &property_name, const QVariant &value);
    QVariant question(const QVariant &title, const QVariant &message);
    void error(const QVariant &title, const QVariant &message);
    void write(const QVariant &cmd,
               const QVariant &arg1 = QVariant(),
               const QVariant &arg2 = QVariant(),
               const QVariant &arg3 = QVariant(),
               const QVariant &arg4 = QVariant(),
               const QVariant &arg5 = QVariant());

private slots:
    void initWidgets();
    void createWebConfigPage();
    void buttonClicked();

private:
    bool m_connecting_connected;

    QScrollArea *scroll_area;
    QVBoxLayout *layout;

    QPointer<QWidget> settings_widget;
    QLabel *label;

    QByteArray m_settings_ui;
    QByteArray m_settings_js;
};

#endif // SETTINGS_H
