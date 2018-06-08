#include "webclient.h"

WebClient::WebClient(QObject *parent) : AbstractClient(parent)
{
    m_i = 0;

    m_ssl_certificate_accepted = false;
    m_socket = nullptr;
    m_size = 0;
    m_openssl = nullptr;
    m_ssl_client = nullptr;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &WebClient::timeout);
    m_timer->setSingleShot(true);

    m_openssl = new OpenSslLib(this);
    SETTONULLPTR(m_openssl);
}

WebClient::~WebClient()
{
    disconnectedPrivate();
}

void WebClient::abort()
{
    if (m_socket)
        m_socket->abort();
}

bool WebClient::testSsl()
{
    if (!m_openssl->isLoaded())
        emit error("OpenSsl not loaded!");

    return m_openssl->isLoaded();
}

void WebClient::connectToHost(const QString &host, quint16 port,
                              const QByteArray &negotiation_string,
                              const QString &id,
                              const QByteArray &password)
{
    if (!testSsl())
        return;

    if (m_ssl_client)
        return;

    m_ssl_client = new SslClient(this);
    SETTONULLPTR(m_ssl_client);

    connect(m_ssl_client, &SslClient::connectedToServer, this, &WebClient::connectedToServerPrivate);
    connect(m_ssl_client, &SslClient::disconnected, this, &WebClient::disconnectedFromServer);
    connect(m_ssl_client, &SslClient::pending, this, &WebClient::pendingPrivate);
    connect(m_ssl_client, &SslClient::connectionInfo, this, &WebClient::connectToP2PServer);
    connect(m_ssl_client, &SslClient::remotePort, this, &WebClient::remotePort);

    m_negotiation_string = negotiation_string.leftJustified(128, (char)0, true);

    m_id = cleanString(id);

    m_password = cleanString(QLatin1String(password)).toLatin1();

    m_host = host;

    m_host_port = port + 1;

    m_timer->start(10 * 1000);

    m_ssl_client->connectToHost(host, port);
}

void WebClient::connectedToServerPrivate(const QByteArray &hash)
{
    if (!m_ssl_client)
        return;

    m_timer->stop();

    emit connectedToServer(hash);
}

void WebClient::disconnectedFromServer()
{
    stop();

    emit error("Connection with server lost!");
}

void WebClient::acceptSslCertificate()
{
    m_ssl_certificate_accepted = true;

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::PeerInfo));
    data.append(m_negotiation_string);
    data.append(m_id.toLatin1().leftJustified(20, (char)0, false));
    data.append(m_password.leftJustified(64, (char)0, false));

    m_ssl_client->write(data);
}

void WebClient::connectToPeer(const QString &peer_id)
{
    if (!m_ssl_client || !m_ssl_certificate_accepted)
        return;

    QByteArray id = cleanString(peer_id).toLatin1();

    id = id.leftJustified(20, (char)0, true);

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::PeerTryConnect));
    data.append(id);

    m_ssl_client->write(data);
}

void WebClient::pendingPrivate(quint32 host, const QByteArray &id)
{
    if (!m_ssl_certificate_accepted)
        return;

    emit pending(QHostAddress(host), id);
}

void WebClient::acceptConnection()
{
    if (!m_ssl_client || !m_ssl_certificate_accepted)
        return;

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::ConnectionAnswer));
    data.append(getBytes<bool>(true));

    m_ssl_client->write(data);
}

void WebClient::rejectConnection()
{
    if (!m_ssl_client || !m_ssl_certificate_accepted)
        return;

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::ConnectionAnswer));
    data.append(getBytes<bool>(false));

    m_ssl_client->write(data);
}

void WebClient::connectToP2PServer(quint32 ip, const QByteArray &id, const QByteArray &password)
{
    m_openssl->setPassword(password);

    m_peer_ip = ip;

    m_peer_id = id;

    startP2PConnection();
}

void WebClient::startP2PConnection()
{
    if (m_socket || !m_ssl_certificate_accepted)
        return;

    m_socket = new QTcpSocket(this);

    SETTONULLPTR(m_socket);

    connect(m_socket, &QTcpSocket::disconnected, this, &WebClient::disconnectedPrivate);
    connect(m_socket, static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error), this, &WebClient::errorPrivate);

    connect(m_socket, &QTcpSocket::connected, this, &WebClient::readyToGetPort);

    m_timer->start(10 * 1000);

    LIB_DEBUG_LEVEL_1("Trying to connect to P2P server...");

    m_socket->bind(0, QAbstractSocket::ShareAddress);

    m_socket->connectToHost(m_host, m_host_port);
}

void WebClient::readyToGetPort()
{
    if (!m_ssl_certificate_accepted)
        return;

    LIB_DEBUG_LEVEL_1("Sending port...");

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::Port));
    data.append(getBytes<quint16>(m_socket->localPort()));

    m_ssl_client->write(data);
}

void WebClient::remotePort(quint16 port)
{
    if (!m_ssl_certificate_accepted)
        return;

    m_peer_port = port;

    LIB_DEBUG_LEVEL_1("Peer port:" << m_peer_port);

    QTimer::singleShot(0, this, &WebClient::readyToConnectToPeer);
}

void WebClient::readyToConnectToPeer()
{
    if (!m_ssl_certificate_accepted)
        return;

    m_descriptor = m_socket->socketDescriptor();
    m_local_port = m_socket->localPort();

    m_socket->disconnect();
    m_socket->abort();
    m_socket->deleteLater();

    QCoreApplication::processEvents();

    m_socket = new QTcpSocket(this);

    SETTONULLPTR(m_socket);

    LIB_DEBUG_LEVEL_1("Attempt begin...");

    connect(m_socket, &QTcpSocket::connected, this, &WebClient::confirmP2PConnection);
    connect(m_socket, &QTcpSocket::readyRead, this, &WebClient::confirmedP2PConnection);

    m_socket->setSocketDescriptor(m_descriptor, QAbstractSocket::UnconnectedState);

    m_timer->start(10 * 1000);

    QTimer::singleShot(0, this, &WebClient::attemptToConnect);
}

void WebClient::attemptToConnect()
{
    if (!m_socket)
        return;

    LIB_DEBUG_LEVEL_1("Connecting..." << "Attempt" << ++m_i);

    m_socket->bind(m_local_port, QAbstractSocket::ShareAddress);

    QTimer::singleShot(50, this, &WebClient::connectP2P);

    QTimer::singleShot(300, this, &WebClient::connectedP2P);
}

void WebClient::connectP2P()
{
    if (!m_socket)
        return;

    if (m_socket->state() == QAbstractSocket::ConnectedState)
        return;

    if (!m_timer->isActive())
        return;

    m_socket->connectToHost(QHostAddress(m_peer_ip), m_peer_port);
}

void WebClient::confirmP2PConnection()
{
    QByteArray data = QByteArray(1, (char)0);

    m_socket->write(data);
}

void WebClient::confirmedP2PConnection()
{
    m_socket->read(1);

    QTimer::singleShot(0, this, &WebClient::connectedP2P);
}

void WebClient::connectedP2P()
{
    if (!m_socket)
        return;

    if (!m_timer->isActive())
        return;

    if (m_socket->state() == QAbstractSocket::ConnectedState)
    {
        LIB_DEBUG_LEVEL_1("Success on attempt" << m_i);
    }
    else
    {
        m_socket->abort();

        QTimer::singleShot(0, this, &WebClient::attemptToConnect);

        return;
    }

    m_timer->stop();

    LIB_DEBUG_LEVEL_1("Connected!");

    m_socket->disconnect();

    SETTONULLPTR(m_socket);

    connect(m_socket, &QTcpSocket::readyRead, this, &WebClient::readyReadPrivate);
    connect(m_socket, &QTcpSocket::disconnected, this, &WebClient::disconnectedPrivate);

    PeerData pd;
    pd.host = m_socket->peerAddress();
    pd.descriptor = m_socket->socketDescriptor();

    emit connectedToPeer(pd, m_peer_id);

    emit m_socket->readyRead();
}

void WebClient::disconnectedPrivate()
{
    if (!m_socket)
        return;

    QHostAddress host = m_socket->peerAddress();
    qintptr descriptor = m_socket->socketDescriptor();

    stop();

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    emit disconnected(pd);
}

void WebClient::errorPrivate(QAbstractSocket::SocketError e)
{
    if (e != QAbstractSocket::RemoteHostClosedError)
    {
        QString err = m_socket->errorString();
        emit error(err);
    }
    else
    {
        emit error(QString());
    }
}

void WebClient::stop()
{
    m_timer->stop();

    m_size = 0;
    m_buffer.clear();

    if (m_openssl)
        m_openssl->deleteLater();

    if (m_socket)
    {
        m_socket->abort();
        m_socket->deleteLater();
    }

    if (m_ssl_client)
    {
        m_ssl_client->deleteLater();
    }
}

int WebClient::write(const QByteArray &data)
{
    if (!m_socket)
        return 0;

    QByteArray encrypted = m_openssl->encrypt(data);
    QByteArray header = m_openssl->encrypt(getBytes<qint32>(encrypted.size()));

    m_socket->write(header);
    m_socket->write(encrypted);

    return 1;
}

void WebClient::readyReadPrivate()
{
    if (!m_socket)
        return;

    while (m_socket->bytesAvailable() > 0)
    {
        m_buffer.append(m_socket->readAll());

        while ((m_size == 0 && m_buffer.size() >= AES_PADDING) || (m_size > 0 && m_buffer.size() >= m_size))
        {
            if (m_size == 0 && m_buffer.size() >= AES_PADDING)
            {
                QByteArray decrypteddata = m_openssl->decrypt(m_buffer.mid(0, AES_PADDING));

                if (decrypteddata.size() != 4)
                {
                    m_socket->abort();
                    return;
                }

                m_size = getValue<qint32>(decrypteddata);
                m_buffer.remove(0, AES_PADDING);

                if (m_size < 0 || m_size > MAX_NETWORK_CHUNK_SIZE)
                {
                    m_socket->abort();
                    return;
                }
            }
            if (m_size > 0 && m_buffer.size() >= m_size)
            {
                QByteArray encrypteddata = m_buffer.mid(0, m_size);
                m_buffer.remove(0, m_size);
                m_size = 0;

                QByteArray decrypteddata = m_openssl->decrypt(encrypteddata);

                if (decrypteddata.isNull())
                {
                    m_socket->abort();
                    return;
                }

                QHostAddress host = m_socket->peerAddress();
                qintptr descriptor = m_socket->socketDescriptor();

                PeerData pd;
                pd.data = decrypteddata;
                pd.host = host;
                pd.descriptor = descriptor;

                emit readyRead(pd);
            }
        }
    }
}

void WebClient::timeout()
{
    emit error("Operation timed out");
    stop();
}
