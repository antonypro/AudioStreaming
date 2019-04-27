#include "sslserver.h"

QMutex record_mutex;
qint64 record_count;

SslServer::SslServer(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<PeerData>("PeerData");

    m_max_connections = 0;
    m_server = nullptr;

    QByteArray password;

    if (!QFileInfo("../data/cert.pem").exists() || !QFileInfo("../data/key.pem").exists())
    {
        QByteArray confirm_password;

        password = getPassword("Enter password to encrypt private key:");

        confirm_password = getPassword("Repeat password to encrypt private key:");

        printf("\n");
        fflush(stdout);

        if (password != confirm_password)
        {
            password.fill(char(0));
            confirm_password.fill(char(0));

            DEBUG_FUNCTION("Different passwords!");
            return;
        }

        confirm_password.fill(char(0));

        DEBUG_FUNCTION("Generating certificate...");

        Certificate certificate_genarator;

        bool success = certificate_genarator.generate("US", "Server", "127.0.0.1", password);

        if (!success)
        {
            DEBUG_FUNCTION("Keys not generated!" << "Error(s):" << qPrintable(QString("\n%0").arg(certificate_genarator.errorString())));
            return;
        }

        DEBUG_FUNCTION("Keys generated!");
    }

    QFile key_file("../data/key.pem");

    if (!key_file.open(QFile::ReadOnly))
    {
        DEBUG_FUNCTION("Private key file could not be openned!");
        return;
    }

    QFile cert_file("../data/cert.pem");

    if (!cert_file.open(QFile::ReadOnly))
    {
        DEBUG_FUNCTION("Public key file could not be openned!");
        return;
    }

    if (password.isNull())
        password = getPassword("Enter password to decrypt private key:");

    printf("\n");
    fflush(stdout);

    m_key = QSslKey(&key_file, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, password);

    password.fill(char(0));

    if (m_key.isNull())
    {
        DEBUG_FUNCTION("Private key is null!");
        return;
    }

    m_cert = QSslCertificate(&cert_file, QSsl::Pem);

    if (m_cert.isNull())
    {
        DEBUG_FUNCTION("Public key is null!");
        return;
    }

    QByteArray hash = m_cert.digest(QCryptographicHash::Sha256).toHex().toUpper();

    QByteArray formatted_hash;

    for (int i = hash.size() - 2; i >= 0; i -= 2)
    {
        formatted_hash.prepend(hash.mid(i, 2));
        formatted_hash.prepend(":");
    }

    formatted_hash.remove(0, 1);

    DEBUG_FUNCTION("Certificate fingerprint:" << qPrintable(formatted_hash));

    key_file.close();
    cert_file.close();

    DEBUG_FUNCTION("Openning database...");

    if (!m_sql.open())
    {
        DEBUG_FUNCTION("Could not open database!");
        return;
    }

    DEBUG_FUNCTION("Database openned!");

    listen();
}

SslServer::~SslServer()
{
    stop();
}

void SslServer::listen()
{
    if (m_server)
        return;

    int port = 0;

    if (!global_settings->contains("port"))
        global_settings->setValue("port", 1024);

    if (!global_settings->contains("maxconnections"))
        global_settings->setValue("maxconnections", 30);

    port = global_settings->value("port").toInt();

    if (port < 1 || port > 65535)
    {
        DEBUG_FUNCTION("Error:" << "Invalid port!");
        return;
    }

    m_max_connections = global_settings->value("maxconnections").toInt();

    if (m_max_connections < 1)
    {
        DEBUG_FUNCTION("Invalid max connections" << m_max_connections);
        return;
    }

    m_server = new TcpServer(this);

    m_server->setMaxPendingConnections(m_max_connections);

    connect(m_server, &TcpServer::serverIncomingConnection, this, &SslServer::newConnectionPrivate);

    bool listening = m_server->listen(QHostAddress::AnyIPv4, quint16(port));

    if (!listening)
    {
        DEBUG_FUNCTION("Error:" << qPrintable(m_server->errorString()));
        stop();
        return;
    }

    DEBUG_FUNCTION("Listening on port:" << m_server->serverPort());

    QTimer *timer_test = new QTimer(this);
    connect(timer_test, &QTimer::timeout, this, &SslServer::testConnections);
    timer_test->start(1000);
}

void SslServer::stop()
{
    while (!m_socket_list.isEmpty())
        removeSocket(m_socket_list.first());

    m_max_connections = 0;

    if (!m_server)
        return;

    m_server->close();
    m_server->deleteLater();
}

void SslServer::newConnectionPrivate(qintptr descriptor)
{
    QSslSocket *socket = new QSslSocket(this);
    socket->setSocketDescriptor(descriptor);

    if (m_max_connections == 0)
    {
        socket->abort();
        return;
    }

    socket->setProtocol(QSsl::TlsV1_2OrLater);

    socket->addCaCertificate(m_cert);
    socket->setLocalCertificate(m_cert);
    socket->setPrivateKey(m_key);

    //New connection done, set one less available connection
    m_max_connections--;

    QByteArray m_buffer;
    qint32 size = 0;

    m_socket_list.append(socket);
    m_descriptor_hash.insert(socket, descriptor);
    m_socket_hash.insert(descriptor, socket);
    m_buffer_hash.insert(socket, m_buffer);
    m_size_hash.insert(socket, size);

    connect(socket, &QSslSocket::encrypted, this, &SslServer::encrypted);
    connect(socket, &QSslSocket::disconnected, this, &SslServer::disconnectedPrivate);
    connect(socket, static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), this, &SslServer::sslErrors);

    m_alive_hash[socket].start();

    socket->startServerEncryption();
}

void SslServer::timeout(QSslSocket *socket)
{
    QHostAddress host = socket->peerAddress();

    DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "Timed out!");

    removeSocket(socket);
}

void SslServer::testConnections()
{
    QList<QSslSocket*> socket_list = m_socket_list;

    for (int i = 0; i < socket_list.size(); i++)
    {
        QSslSocket *socket = socket_list[i];

        if (!m_alive_hash.contains(socket))
            continue;

        if (m_alive_hash[socket].elapsed() > TIMEOUT)
            timeout(socket);
    }
}

void SslServer::sslErrors(QList<QSslError> errors)
{
    Q_UNUSED(errors)

    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());

    socket->ignoreSslErrors();
}

void SslServer::encrypted()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());

    if (!socket)
        return;

    connect(socket, &QSslSocket::readyRead, this, &SslServer::readyReadPrivate);

    QHostAddress host = socket->peerAddress();

    DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "Connected with encryption!");
}

void SslServer::readyReadPrivate()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());

    if (!socket)
        return;

    QByteArray *m_buffer = &m_buffer_hash[socket];
    qint32 &m_size = m_size_hash[socket];

    while (socket->bytesAvailable() > 0)
    {
        m_buffer->append(socket->readAll());

        while ((m_size == 0 && m_buffer->size() >= 4) || (m_size > 0 && m_buffer->size() >= m_size))
        {
            if (m_size == 0 && m_buffer->size() >= 4)
            {
                m_size = getValue<qint32>(m_buffer->mid(0, 4));
                m_buffer->remove(0, 4);

                if (m_size < 0 || m_size > MAX_NETWORK_CHUNK_SIZE)
                {
                    socket->abort();
                    return;
                }
            }
            if (m_size > 0 && m_buffer->size() >= m_size)
            {
                QByteArray data = m_buffer->mid(0, m_size);
                m_buffer->remove(0, m_size);
                m_size = 0;

                QHostAddress host = socket->peerAddress();
                qintptr descriptor = socket->socketDescriptor();

                PeerData pd;
                pd.data = data;
                pd.host = host;
                pd.descriptor = descriptor;

                QMetaObject::invokeMethod(this, "process", Qt::QueuedConnection, Q_ARG(PeerData, pd));
            }
        }
    }
}

void SslServer::process(const PeerData &pd)
{
    QByteArray data = pd.data;

    if (data.isEmpty())
        return;

    quint8 command = getValue<quint8>(data.mid(0, 1));
    data.remove(0, 1);

    QSslSocket *socket = m_socket_hash[pd.descriptor];

    if (!socket)
        return;

    switch (command)
    {
    case ServerCommand::PeerTryConnect:
    {
        if (data.size() != 20)
        {
            removeSocket(socket);
            return;
        }

        QByteArray peer_id = data;

        peer_id = cleanString(QLatin1String(peer_id)).toLatin1().leftJustified(peer_id.length(), char(0), true);

        if (m_unavailable_list.contains(peer_id))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User target:" << qPrintable(peer_id) << "Attempt to connect to unavailable user!");
            writeWarning(socket, "You're trying to connect to a unavailable user!");
            return;
        }

        if (peer_id == m_id_hash[socket])
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User target:" << qPrintable(peer_id) << "Attempt to connect to itself!");
            writeWarning(socket, "You're trying to connect to itself!");
            return;
        }

        if (!m_id_list.contains(peer_id))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User target:" << qPrintable(peer_id) << "Attempt to connect to non connected user!");
            writeWarning(socket, "You're trying to connect to a non connected user!");
            return;
        }

        QSslSocket *target_socket = m_id_hash.key(peer_id);

        if (m_connecting_connected_list.contains(target_socket))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User target:" << qPrintable(peer_id) << "Attempt to connect to already connected user!");
            writeWarning(socket, "User already connected!");
            return;
        }

        m_connecting_connected_list.append(socket);
        m_connecting_connected_list.append(target_socket);

        QByteArray data;
        data.append(getBytes<quint8>(ServerCommand::ConnectionRequested));
        data.append(getBytes<quint32>(socket->localAddress().toIPv4Address()));
        data.append(m_id_hash[socket]);

        m_accept_hash.insert(target_socket, socket);

        DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                       << "User target:" << qPrintable(peer_id) << "Trying to connect to peer!");

        target_socket->write(getBytes<qint32>(data.size()));
        target_socket->write(data);

        break;
    }
    case ServerCommand::ConnectionAnswer:
    {
        if (data.size() != 1)
        {
            removeSocket(socket);
            return;
        }

        QSslSocket *socket_starter = m_accept_hash[socket];

        bool accepted = getValue<bool>(data.mid(0, 1));

        if (!socket_starter)
        {
            m_connecting_connected_list.removeAll(socket_starter);
            m_connecting_connected_list.removeAll(socket);

            if (accepted)
            {
                DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                               << "User disconnected or canceled operation!");
                writeWarning(socket, "Can't connect to user because it's disconnected or canceled operation!");
            }

            return;
        }

        if (!accepted)
        {
            QSslSocket *socket1 = socket_starter;
            QSslSocket *socket2 = socket;

            m_accept_hash.remove(socket1);
            m_accept_hash.remove(socket2);

            m_connecting_connected_list.removeAll(socket1);
            m_connecting_connected_list.removeAll(socket2);

            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User target:" << qPrintable(m_id_hash.value(socket_starter)) << "User refused connection!");
            writeWarning(socket_starter, "User refused connection!");

            return;
        }

        QSslSocket *socket1 = socket;
        QSslSocket *socket2 = socket_starter;

        m_connected_1.insert(socket1, socket2);
        m_connected_2.insert(socket2, socket1);

        QByteArray data1;
        QByteArray data2;

        data1.append(getBytes<quint8>(ServerCommand::ConnectedToPeer));
        data2.append(getBytes<quint8>(ServerCommand::ConnectedToPeer));

        QByteArray password = OpenSslLib::RANDbytes(32);

        data1.append(m_id_hash[socket1]);
        data1.append(password);

        data2.append(m_id_hash[socket2]);
        data2.append(password);

        socket1->write(getBytes<qint32>(data2.size()));
        socket1->write(data2);

        socket2->write(getBytes<qint32>(data1.size()));
        socket2->write(data1);

        DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket_starter))
                       << "User target:" << qPrintable(m_id_hash.value(socket)) << "Connected!");

        break;
    }
    case ServerCommand::DisconnectedFromPeer:
    {
        QSslSocket *target_socket = m_connected_1.value(socket);

        if (!target_socket)
            target_socket = m_connected_2.value(socket);

        DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                       << "User target:" << qPrintable(m_id_hash.value(target_socket)) << "Disconnected from peer!");

        disconnectedFromPeer(socket);

        break;
    }
    case ServerCommand::P2PData:
    {
        QSslSocket *target_socket = m_connected_1.value(socket);

        if (!target_socket)
            target_socket = m_connected_2.value(socket);

        if (!target_socket)
            break;

        data.prepend(getBytes<quint8>(ServerCommand::P2PData));

        target_socket->write(getBytes<qint32>(data.size()));
        target_socket->write(data);

        m_alive_hash[socket].restart();

        m_alive_hash[target_socket].restart();

        break;
    }
    case ServerCommand::Alive:
    {
        m_alive_hash[socket].restart();

        break;
    }
    case ServerCommand::XML:
    {
        processCommandXML(socket, data);

        break;
    }
    default:
    {
        removeSocket(socket);
        return;
    }
    }
}

void SslServer::processCommandXML(QSslSocket *socket, const QByteArray &data)
{
    qint32 argc;

    QString cmd;

    QByteArray arg1, arg2, arg3, arg4, arg5,
            arg6, arg7, arg8, arg9, arg10;

    if (!XMLReader(data, &argc, &cmd,
                   &arg1, &arg2, &arg3, &arg4, &arg5,
                   &arg6, &arg7, &arg8, &arg9, &arg10))
    {
        writeWarning(socket, "Invalid XML", true);
        return;
    }

    if (cmd == "LOGIN")
    {
        if (arg1.isEmpty() || arg2.isEmpty()
                || arg3.isEmpty() || arg4.isEmpty()
                || arg5.isEmpty())
        {
            writeWarning(socket, "Invalid XML", true);
            return;
        }

        QByteArray peer_negotiation_string = arg1;
        QByteArray peer_id = arg2;
        QByteArray password = arg3;
        bool user_unavailable = getValue<bool>(arg4);
        bool new_user = getValue<bool>(arg5);
        QString code = QLatin1String(arg6);

        if (peer_negotiation_string != NEGOTIATION_STRING)
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                           << "Invalid NEGOTIATION STRING!");
            writeWarning(socket, "Invalid NEGOTIATION STRING!", true);
            return;
        }

        if (m_id_list.contains(peer_id))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                           << "This user is already connected!");
            writeWarning(socket, "This user is already connected!", true);
            return;
        }

        if (new_user)
        {
            if (m_sql.userExists("root"))
            {
                if (!m_code_list.contains(code))
                {
                    DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                                   << "Invalid code!");
                    writeWarning(socket, "Invalid code!", true);
                    return;
                }
                else
                {
                    m_code_list.removeAll(code);
                }
            }
            else if (cleanString(QLatin1String(peer_id)) != "root")
            {
                DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                               << "Need to create root account first!");
                writeWarning(socket, "Need to create root account first!", true);
                return;
            }

            if (m_sql.userExists(cleanString(QLatin1String(peer_id))))
            {
                DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                               << "User already exists!");
                writeWarning(socket, "User already exists!", true);
                return;
            }
            else if (!m_sql.createUser(cleanString(QLatin1String(peer_id)), cleanString(QLatin1String(password))))
            {
                DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                               << "Can't create new user!");
                writeWarning(socket, "Can't create new user!", true);
                return;
            }
        }

        if (!m_sql.loginUser(cleanString(QLatin1String(peer_id)), cleanString(QLatin1String(password))))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                           << "Can't login with this user and password!");
            writeWarning(socket, "Can't login with this user and password!", true);
            return;
        }

        DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(peer_id)
                       << "User logged in!");

        m_id_list.append(peer_id);
        m_id_hash.insert(socket, peer_id);

        if (user_unavailable)
            m_unavailable_list.append(peer_id);

        QByteArray data;
        data.append(getBytes<quint8>(ServerCommand::LoggedIn));

        socket->write(getBytes<qint32>(data.size()));
        socket->write(data);
    }
    else if (cmd == "REQUEST_SETTINGS")
    {
        QString ui_path;
        QString js_path;

        QString id = QLatin1String(m_id_hash[socket]);

        if (id != "root")
        {
            ui_path = "../data/settings.ui";
            js_path = "../data/settings.js";
        }
        else
        {
            ui_path = "../data/settings_root.ui";
            js_path = "../data/settings_root.js";
        }

        QFile ui_file(ui_path);
        QFile js_file(js_path);

        if (!ui_file.open(QFile::ReadOnly) || !js_file.open(QFile::ReadOnly))
        {
            DEBUG_FUNCTION("Can't send settings file");
            writeWarning(socket, "Can't send settings file", true);
            return;
        }

        QByteArray ui_data = ui_file.readAll();
        QByteArray js_data = js_file.readAll();

        QByteArray xml_data = XMLWriter("SETTINGS", ui_data, js_data);
        writeCommandXML(socket, xml_data);
    }
    else if (cmd == "DELETE_ACCOUNT")
    {
        QString id = QLatin1String(m_id_hash[socket]);

        if (id == "root")
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "Invalid operation!");
            writeWarning(socket, "Invalid operation!", true);
            return;
        }

        if (m_sql.deleteUser(id))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User deleted sucessfully!");
            writeWarning(socket, "User deleted sucessfully!", true);
        }
        else
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "Error deleting user!");
            writeWarning(socket, "Error deleting user!", true);
        }
    }
    else if (cmd == "CHANGE_ACCOUNT_PASSWORD")
    {
        if (arg1.isEmpty())
        {
            writeWarning(socket, "Invalid XML", true);
            return;
        }

        QByteArray password = arg1;

        if (m_sql.changePassword(m_id_hash[socket], cleanString(QLatin1String(password))))
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "User password changed sucessfully!");
            writeWarning(socket, "User password changed sucessfully!", true);
        }
        else
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "Error changing user password!");
            writeWarning(socket, "Error changing user password!", true);
        }
    }
    else if (cmd == "REQUEST_CODE")
    {
        QByteArray id = m_id_hash[socket];

        if (QLatin1String(id) != "root")
        {
            DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                           << "Invalid operation!");
            writeWarning(socket, "Invalid operation!", true);
            return;
        }

        QString code;

        do
        {
            code = QString(OpenSslLib::RANDbytes(4).toHex());
            code.prepend("#");
        }
        while (m_code_list.contains(code));

        m_code_list.append(code);

        QTimer::singleShot(60 * 60 * 1000, [=]{
            m_code_list.removeAll(code);
        });

        DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                       << "Sending code!");

        QByteArray xml_data = XMLWriter("CODE", "edit_code", code.toLatin1());
        writeCommandXML(socket, xml_data);
    }
    else
    {
        DEBUG_FUNCTION("IP:" << qPrintable(socket->peerAddress().toString()) << "User origin:" << qPrintable(m_id_hash.value(socket))
                       << "Invalid operation!");
        writeWarning(socket, "Invalid operation!", true);
    }
}

void SslServer::writeCommandXML(QSslSocket *socket, const QByteArray &XML)
{
    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::XML));
    data.append(XML);

    socket->write(getBytes<qint32>(data.size()));
    socket->write(data);
}

void SslServer::disconnectedPrivate()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());

    if (!socket)
        return;

    removeSocket(socket);
}

void SslServer::removeSocket(QSslSocket *socket)
{
    if (!socket)
        return;

    qintptr descriptor = m_descriptor_hash[socket];

    QHostAddress host = socket->peerAddress();

    QString peer_id = m_id_hash.value(socket);

    disconnectedFromPeer(socket);

    m_socket_hash.remove(descriptor);
    m_descriptor_hash.remove(socket);
    m_buffer_hash.remove(socket);
    m_size_hash.remove(socket);

    m_socket_list.removeAll(socket);
    m_id_list.removeAll(m_id_hash[socket]);
    m_unavailable_list.removeAll(m_id_hash[socket]);
    m_id_hash.remove(socket);
    m_alive_hash.remove(socket);

    socket->blockSignals(true);
    socket->abort();
    socket->deleteLater();

    //Peer disconnected, set one more available connection
    m_max_connections++;

    DEBUG_FUNCTION("IP:" << qPrintable(host.toString()) << "User origin:" << qPrintable(peer_id));
}

void SslServer::writeWarning(QSslSocket *socket, const QString &message, bool remove_socket)
{
    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::Warning));
    data.append(message.toLatin1());

    socket->write(getBytes<qint32>(data.size()));
    socket->write(data);

    if (remove_socket)
    {
        QTimer::singleShot(0, [=]{removeSocket(socket);});
    }
}

void SslServer::disconnectedFromPeer(QSslSocket *socket)
{
    QSslSocket *target_socket = m_accept_hash.value(socket);

    if (!target_socket)
        target_socket = m_accept_hash.key(socket);

    QSslSocket *socket1 = socket;
    QSslSocket *socket2 = target_socket;

    m_connected_1.remove(socket1);
    m_connected_1.remove(socket2);
    m_connected_2.remove(socket1);
    m_connected_2.remove(socket2);

    m_accept_hash.remove(socket1);
    m_accept_hash.remove(socket2);

    m_connecting_connected_list.removeAll(socket1);
    m_connecting_connected_list.removeAll(socket2);

    QByteArray data1;
    data1.append(getBytes<quint8>(ServerCommand::DisconnectedFromPeer));

    if (socket1)
    {
        socket1->write(getBytes<qint32>(data1.size()));
        socket1->write(data1);
    }

    if (socket2)
    {
        socket2->write(getBytes<qint32>(data1.size()));
        socket2->write(data1);
    }
}
