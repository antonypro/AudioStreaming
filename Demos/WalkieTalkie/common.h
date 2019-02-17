#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

static inline QString cleanString(const QString &s)
{
    QString diacriticLetters;
    QStringList noDiacriticLetters;
    QStringList acceptedCharacters;

    diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
    noDiacriticLetters << "S"<<"OE"<<"Z"<<"s"<<"oe"<<"z"<<"Y"<<"Y"<<"u"<<"A"<<"A"<<"A"<<"A"<<"A"<<"A"<<"AE"<<"C"
                        <<"E"<<"E"<<"E"<<"E"<<"I"<<"I"<<"I"<<"I"<<"D"<<"N"<<"O"<<"O"<<"O"<<"O"<<"O"<<"O"<<"U"<<"U"
                       <<"U"<<"U"<<"Y"<<"s"<<"a"<<"a"<<"a"<<"a"<<"a"<<"a"<<"ae"<<"c"<<"e"<<"e"<<"e"<<"e"<<"i"<<"i"
                      <<"i"<<"i"<<"o"<<"n"<<"o"<<"o"<<"o"<<"o"<<"o"<<"o"<<"u"<<"u"<<"u"<<"u"<<"y"<<"y";

    acceptedCharacters << "0" << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9"
                       << "a" << "b" << "c" << "d" << "e" << "f" << "g" << "h" << "i" << "j"
                       << "k" << "l" << "m" << "n" << "o" << "p" << "q" << "r" << "s" << "t"
                       << "u" << "v" << "w" << "x" << "y" << "z";

    QString output_tmp;

    for (int i = 0; i < s.length(); i++)
    {
        QChar c = s[i];
        int dIndex = diacriticLetters.indexOf(c);
        if (dIndex < 0)
        {
            output_tmp.append(c);
        }
        else
        {
            QString replacement = noDiacriticLetters[dIndex];
            output_tmp.append(replacement);
        }
    }

    output_tmp = output_tmp.toLower();

    QString output;

    for (int i = 0; i < output_tmp.length(); i++)
    {
        QChar c = output_tmp[i];

        if (acceptedCharacters.contains(c))
            output.append(c);
    }

    return output;
}

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

template <typename T>
static inline QByteArray getBytes(T input)
{
    QByteArray tmp;
    QDataStream data(&tmp, QIODevice::WriteOnly);
    data << input;
    return tmp;
}

template <typename T>
static inline T getValue(QByteArray bytes)
{
    T tmp;
    QDataStream data(&bytes, QIODevice::ReadOnly);
    data >> tmp;
    return tmp;
}

/*** BEGIN XML ***/

static inline QByteArray XMLWriter(const QString &cmd,
                                   const QByteArray &arg1 = QByteArray(),
                                   const QByteArray &arg2 = QByteArray(),
                                   const QByteArray &arg3 = QByteArray(),
                                   const QByteArray &arg4 = QByteArray(),
                                   const QByteArray &arg5 = QByteArray())
{
    qint32 argc = 0;

    if (!arg5.isNull() && !arg4.isNull() && !arg3.isNull() && !arg2.isNull() && !arg1.isNull())
        argc = 5;
    else if (!arg4.isNull() && !arg3.isNull() && !arg2.isNull() && !arg1.isNull())
        argc = 4;
    else if (!arg3.isNull() && !arg2.isNull() && !arg1.isNull())
        argc = 3;
    else if (!arg2.isNull() && !arg1.isNull())
        argc = 2;
    else if (!arg1.isNull())
        argc = 1;

    QByteArray xml_data;

    QXmlStreamWriter writer(&xml_data);
    writer.setAutoFormatting(true);

    writer.writeStartDocument();

    writer.writeStartElement("ACTION_WEB_SERVER");

    writer.writeTextElement("CMD", cmd);

    if (argc >= 1)
    {
        writer.writeTextElement("arg1", QString(arg1.toBase64()));
    }
    if (argc >= 2)
    {
        writer.writeTextElement("arg2", QString(arg2.toBase64()));
    }
    if (argc >= 3)
    {
        writer.writeTextElement("arg3", QString(arg3.toBase64()));
    }
    if (argc >= 4)
    {
        writer.writeTextElement("arg4", QString(arg4.toBase64()));
    }
    if (argc >= 5)
    {
        writer.writeTextElement("arg5", QString(arg5.toBase64()));
    }

    writer.writeEndElement();

    writer.writeEndDocument();

    QByteArray compressed_xml_data = qCompress(xml_data, 9);

    return compressed_xml_data;
}

static inline bool XMLReader(const QByteArray &xml_data, qint32 *argc, QString *cmd,
                             QByteArray *arg1, QByteArray *arg2, QByteArray *arg3, QByteArray *arg4, QByteArray *arg5)
{
    QByteArray uncompressed_xml_data = qUncompress(xml_data);

    QXmlStreamReader reader(uncompressed_xml_data);

    QString m_cmd;

    if (reader.atEnd()
            || reader.hasError()
            || !reader.readNextStartElement()
            || reader.name() != "ACTION_WEB_SERVER")
    {
        return false;
    }

    if (!reader.atEnd() && !reader.hasError() && reader.readNextStartElement())
    {
        if (reader.name() == "CMD")
            m_cmd = reader.readElementText();
    }
    else
    {
        return false;
    }

    QByteArray m_arg1, m_arg2, m_arg3, m_arg4, m_arg5;

    int m_argc = 0;

    while (!reader.atEnd() && !reader.hasError() && reader.readNextStartElement())
    {
        switch (m_argc)
        {
        case 0:
            m_argc++;
            m_arg1 = QByteArray::fromBase64(reader.readElementText().toLatin1());
            break;
        case 1:
            m_argc++;
            m_arg2 = QByteArray::fromBase64(reader.readElementText().toLatin1());
            break;
        case 2:
            m_argc++;
            m_arg3 = QByteArray::fromBase64(reader.readElementText().toLatin1());
            break;
        case 3:
            m_argc++;
            m_arg4 = QByteArray::fromBase64(reader.readElementText().toLatin1());
            break;
        case 4:
            m_argc++;
            m_arg5 = QByteArray::fromBase64(reader.readElementText().toLatin1());
            break;
        default:
            break;
        }
    }

    if (reader.hasError())
    {
        return false;
    }

    *argc = m_argc;

    *cmd = m_cmd;

    if (m_argc >= 1)
    {
        *arg1 = m_arg1;
    }

    if (m_argc >= 2)
    {
        *arg2 = m_arg2;
    }

    if (m_argc >= 3)
    {
        *arg3 = m_arg3;
    }

    if (m_argc >= 4)
    {
        *arg4 = m_arg4;
    }

    if (m_argc >= 5)
    {
        *arg5 = m_arg5;
    }

    return true;
}

/*** END XML ***/

#endif // COMMON_H
