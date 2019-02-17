#include "settings.h"

Settings::Settings(QWidget *parent) : QWidget(parent)
{
    initWidgets();
}

void Settings::initWidgets()
{
    label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);
    label->setText("<h1>You're disconnected!</h1>");

    QVBoxLayout *layout_main = new QVBoxLayout(this);
    layout_main->setMargin(0);
    layout_main->setSpacing(0);

    scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);

    QWidget *widget = new QWidget(this);

    layout = new QVBoxLayout(widget);

    layout->addWidget(label);
    label->show();

    createWebConfigPage();

    layout_main->addWidget(scroll_area);

    scroll_area->setWidget(widget);
}

void Settings::createWebConfigPage()
{
    m_connecting_connected = false;

    connect(this, &Settings::loggedIn, this, [=]{
        layout->removeWidget(label);
        label->hide();

        QByteArray xml_data = XMLWriter("REQUEST_SETTINGS");
        emit commandXML(xml_data);
    });

    connect(this, &Settings::loggedOut, this, [=]{
        if (settings_widget)
        {
            layout->removeWidget(settings_widget);
            settings_widget->deleteLater();
        }

        layout->addWidget(label);
        label->show();

        m_connecting_connected = false;
    });
}

void Settings::loadPage(const QByteArray &ui, const QByteArray &js)
{
    m_settings_ui = ui;
    m_settings_js = js;

    if (settings_widget)
    {
        layout->removeWidget(settings_widget);
        settings_widget->deleteLater();
    }

    layout->removeWidget(label);
    label->hide();

    QBuffer buffer(&m_settings_ui);
    buffer.open(QBuffer::ReadOnly);

    QUiLoader loader;
    settings_widget = loader.load(&buffer, this);

    buffer.close();

    for (QLineEdit *edit : settings_widget->findChildren<QLineEdit*>())
    {
        if (edit->isReadOnly())
            continue;

        edit->setInputMethodHints(edit->inputMethodHints() | Qt::ImhSensitiveData | Qt::ImhLowercaseOnly);

        connect(edit, &QLineEdit::textChanged, [=](const QString &text){
            edit->blockSignals(true);
            edit->setText(cleanString(text));
            edit->blockSignals(false);
        });
    }

    for (QPushButton *button : settings_widget->findChildren<QPushButton*>())
    {
        connect(button, &QPushButton::clicked, this, &Settings::buttonClicked);
    }

    layout->addWidget(settings_widget, 0, Qt::AlignLeft);
}

QVariant Settings::readProperty(const QVariant &name, const QVariant &property_name)
{
    QWidget *widget = settings_widget->findChild<QWidget*>(name.toString());

    if (!widget)
        return QVariant();

    return widget->property(qPrintable(property_name.toString()));
}

QVariant Settings::writeProperty(const QVariant &name, const QVariant &property_name, const QVariant &value)
{
    QWidget *widget = settings_widget->findChild<QWidget*>(name.toString());

    if (!widget)
        return QVariant();

    return widget->setProperty(qPrintable(property_name.toString()), value);
}

void Settings::buttonClicked()
{
    QString name = sender()->objectName();

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    QJSEngine engine;
    QJSValue script = engine.evaluate(m_settings_js);
    QJSValue settings = engine.newQObject(this);
    engine.globalObject().setProperty("settings", settings);

    QJSValueList args;
    args << name;
    script.call(args);
}

QVariant Settings::question(const QVariant &title, const QVariant &message)
{
    int result = msgBoxQuestion(title.toString(), message.toString(), this);

    return (result == QMessageBox::Yes);
}

void Settings::error(const QVariant &title, const QVariant &message)
{
    msgBoxCritical(title.toString(), message.toString(), this);
}

void Settings::write(const QVariant &cmd,
                     const QVariant &arg1,
                     const QVariant &arg2,
                     const QVariant &arg3,
                     const QVariant &arg4,
                     const QVariant &arg5)
{
    QByteArray xml_data = XMLWriter(cmd.toString().toLatin1(),
                                    arg1.toString().toLatin1(),
                                    arg2.toString().toLatin1(),
                                    arg3.toString().toLatin1(),
                                    arg4.toString().toLatin1(),
                                    arg5.toString().toLatin1());

    emit commandXML(xml_data);
}

void Settings::processCommandXML(const QByteArray &data)
{
    qint32 argc;

    QString cmd;

    QByteArray arg1, arg2, arg3, arg4, arg5;

    if (!XMLReader(data, &argc, &cmd,
                   &arg1, &arg2, &arg3, &arg4, &arg5))
    {
        msgBoxCritical("Error", "Invalid XML", this);
        return;
    }

    if (cmd == "SETTINGS" && argc == 2)
    {
        loadPage(arg1, arg2);
    }
    else if (cmd == "CODE" && argc == 2)
    {
        QLineEdit *edit = settings_widget->findChild<QLineEdit*>(QLatin1Literal(arg1));

        if (edit)
            edit->setText(QLatin1String(arg2));
    }
    else
    {
        msgBoxCritical("Error", "Invalid command", this);
        return;
    }
}
