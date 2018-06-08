#include "sslserver.h"

QMutex record_mutex;
qint64 record_count;

SslServer::SslServer(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<PeerData>("PeerData");

    m_max_connections = 0;
    m_server = nullptr;
    m_server_p2p = nullptr;

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
            password.fill((char)0);
            confirm_password.fill((char)0);

            DEBUG_FUNCTION("Different passwords!");
            return;
        }

        confirm_password.fill((char)0);

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

    password.fill((char)0);

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

    quint16 port = 0;

    if (!global_settings->contains("port"))
        global_settings->setValue("port", 1024);

    if (!global_settings->contains("maxconnections"))
        global_settings->setValue("maxconnections", 30);

    port = global_settings->value("port").toInt();

    if (port < 1 || port > 65534)
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

    SETTONULLPTR(m_server);

    m_server->setMaxPendingConnections(m_max_connections);

    connect(m_server, &TcpServer::serverIncomingConnection, this, &SslServer::newConnectionPrivate);

    bool listening = m_server->listen(QHostAddress::AnyIPv4, port);

    if (!listening)
    {
        DEBUG_FUNCTION("Error:" << qPrintable(m_server->errorString()));
        stop();
        return;
    }

    m_server_p2p = new QTcpServer(this);
    SETTONULLPTR(m_server_p2p);

    connect(m_server_p2p, &QTcpServer::newConnection, this, &SslServer::newP2P);

    bool listeningp2p = m_server_p2p->listen(QHostAddress::AnyIPv4, port + 1);

    if (!listeningp2p)
    {
        DEBUG_FUNCTION("Error:" << qPrintable(m_server_p2p->errorString()));
        stop();
        return;
    }

    DEBUG_FUNCTION("Listening on ports:" << m_server->serverPort() << m_server_p2p->serverPort());
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

void SslServer::newP2P()
{
    while (m_server_p2p->hasPendingConnections())
    {
        QTcpSocket *socket = m_server_p2p->nextPendingConnection();
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        QTimer::singleShot(60 * 1000, socket, &QTcpSocket::abort);
        DEBUG_FUNCTION("P2P:" << qPrintable(socket->peerAddress().toString()) << "Port:" <<socket->peerPort());
    }
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

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SslServer::timeout);
    timer->setSingleShot(true);
    timer->setInterval(4 * 60 * 60 * 1000 /*4 hours*/);

    m_socket_list.append(socket);
    m_descriptor_hash.insert(socket, descriptor);
    m_socket_hash.insert(descriptor, socket);
    m_buffer_hash.insert(socket, m_buffer);
    m_size_hash.insert(socket, size);
    m_timer_hash.insert(socket, timer);

    connect(socket, &QSslSocket::encrypted, this, &SslServer::encrypted);
    connect(socket, &QSslSocket::disconnected, this, &SslServer::disconnectedPrivate);
    connect(socket, static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), this, &SslServer::sslErrors);

    timer->start();

    socket->startServerEncryption();
}

void SslServer::timeout()
{
    QTimer *timer = qobject_cast<QTimer*>(sender());

    QSslSocket *socket = m_timer_hash.key(timer);

    QHostAddress host = socket->peerAddress();

    removeSocket(socket);

    DEBUG_FUNCTION("Timed out" << qPrintable(QHostAddress(host.toIPv4Address()).toString()));
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

    DEBUG_FUNCTION("Connected:" << qPrintable(QHostAddress(host.toIPv4Address()).toString()));
}

void SslServer::readyReadPrivate()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());

    if (!socket)
        return;

    QByteArray *m_buffer = &m_buffer_hash[socket];
    qint32 *size = &m_size_hash[socket];
    Q_UNUSED(size)
#define m_size *size
    while (socket->bytesAvailable() > 0)
    {
        m_buffer->append(socket->readAll());

        while ((m_size == 0 && m_buffer->size() >= 4) || (m_size > 0 && m_buffer->size() >= m_size))
        {
            if (m_size == 0 && m_buffer->size() >= 4)
            {
                m_size = getValue<qint32>(m_buffer->mid(0, 4));
                m_buffer->remove(0, 4);
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
    case ServerCommand::PeerInfo:
    {
        QTimer *timer = m_timer_hash[socket];
        timer->stop();

        if (data.size() != 128 + 20 + 64)
        {
            removeSocket(socket);
            return;
        }

        QByteArray peer_negotiation_string = data.mid(0, 128);
        QByteArray peer_id = data.mid(128, 20);
        QByteArray password = data.mid(128 + 20, 64);

        if (peer_negotiation_string != NEGOTIATION_STRING)
        {
            removeSocket(socket);
            return;
        }

        if (m_id_list.contains(peer_id))
        {
            removeSocket(socket);
            return;
        }

        if (!m_sql.loginUser(cleanString(QLatin1String(peer_id)), cleanString(QLatin1String(password))))
        {
            removeSocket(socket);
            return;
        }

        m_id_list.append(peer_id);
        m_id_hash.insert(socket, peer_id);

        break;
    }
    case ServerCommand::PeerTryConnect:
    {
        if (data.size() != 20)
        {
            removeSocket(socket);
            return;
        }

        QByteArray peer_id = data;

        peer_id = cleanString(QLatin1String(peer_id)).toLatin1().leftJustified(peer_id.length(), (char)0, true);

        if (peer_id == m_id_hash[socket])
        {
            removeSocket(socket);
            return;
        }

        if (!m_id_list.contains(peer_id))
        {
            removeSocket(socket);
            return;
        }

        QByteArray data;
        data.append(getBytes<quint8>(ServerCommand::ConnectionRequested));
        data.append(getBytes<quint32>(socket->localAddress().toIPv4Address()));
        data.append(m_id_hash[socket]);

        QSslSocket *target_socket = m_id_hash.key(peer_id);

        m_accept_hash.insert(target_socket, socket);

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

        if (!socket_starter)
        {
            removeSocket(socket);
            return;
        }

        bool accepted = getValue<bool>(data.mid(0, 1));

        if (!accepted)
        {
            removeSocket(socket_starter);
            return;
        }

        QSslSocket *socket1 = socket;
        QSslSocket *socket2 = socket_starter;

        quint32 ip1 = socket1->peerAddress().toIPv4Address();
        quint32 ip2 = socket2->peerAddress().toIPv4Address();

        QByteArray data1;
        QByteArray data2;

        data1.append(getBytes<quint8>(ServerCommand::ConnectionInfo));
        data2.append(getBytes<quint8>(ServerCommand::ConnectionInfo));

        QByteArray password = OpenSslLib::RANDbytes(32);

        data1.append(getBytes<quint32>(ip1));
        data1.append(m_id_hash[socket1]);
        data1.append(password);

        data2.append(getBytes<quint32>(ip2));
        data2.append(m_id_hash[socket2]);
        data2.append(password);

        socket1->write(getBytes<qint32>(data2.size()));
        socket1->write(data2);

        socket2->write(getBytes<qint32>(data1.size()));
        socket2->write(data1);

        break;
    }
    case ServerCommand::Port:
    {
        if (data.size() != 2)
        {
            removeSocket(socket);
            return;
        }

        if (!m_port_hash.contains(socket))
            m_port_hash.insert(socket, data);

        QSslSocket *target_socket = m_accept_hash.value(socket);

        if (!target_socket)
            target_socket = m_accept_hash.key(socket);

        if (!target_socket)
            return;

        QSslSocket *socket1 = socket;
        QSslSocket *socket2 = target_socket;

        QByteArray data_port_1 = m_port_hash.value(socket1);
        QByteArray data_port_2 = m_port_hash.value(socket2);

        if (data_port_1.size() != 2 || data_port_2.size() != 2)
            return;

        data_port_1.prepend(getBytes<quint8>(ServerCommand::Port));
        data_port_2.prepend(getBytes<quint8>(ServerCommand::Port));

        socket1->write(getBytes<qint32>(data_port_2.size()));
        socket1->write(data_port_2);

        socket2->write(getBytes<qint32>(data_port_1.size()));
        socket2->write(data_port_1);

        m_port_hash.remove(socket1);
        m_port_hash.remove(socket2);

        break;
    }
    default:
    {
        removeSocket(socket);
        return;
    }
    }
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

    m_socket_hash.remove(descriptor);
    m_descriptor_hash.remove(socket);
    m_buffer_hash.remove(socket);
    m_size_hash.remove(socket);
    m_timer_hash[socket]->deleteLater();
    m_timer_hash.remove(socket);
    m_accept_hash.remove(m_accept_hash.key(socket));
    m_port_hash.remove(socket);

    m_socket_list.removeAll(socket);
    m_id_list.removeAll(m_id_hash[socket]);
    m_id_hash.remove(socket);

    socket->blockSignals(true);
    socket->abort();
    socket->deleteLater();

    //Peer disconnected, set one more available connection
    m_max_connections++;

    DEBUG_FUNCTION("Disconnected:" << qPrintable(QHostAddress(host.toIPv4Address()).toString()));
}
