#include "sslserver.h"

SslServer::SslServer(QObject *parent) : AbstractServer(parent)
{
    m_max_connections = 0;
    m_auto_accept = false;
    m_server = nullptr;
    m_pending_socket = nullptr;
    m_global_ssl = nullptr;
}

SslServer::~SslServer()
{
    stop();
}

void SslServer::abort(qintptr descriptor)
{
    QTcpSocket *socket = m_socket_hash.value(descriptor);
    socket->abort();
}

bool SslServer::testSsl()
{
    OpenSslLib ssl;

    if (!ssl.isLoaded())
        emit error("OpenSsl not loaded!");

    return ssl.isLoaded();
}

void SslServer::setKeys(const QByteArray &private_key, const QByteArray &public_key)
{
    m_private_key = private_key;
    m_public_key = public_key;
}

void SslServer::listen(quint16 port, bool auto_accept, int max_connections,
                       const QByteArray &negotiation_string,
                       const QString &id,
                       const QByteArray &password)
{
    if (!testSsl())
        return;

    if (m_server)
        return;

    m_auto_accept = auto_accept;

    m_negotiation_string = negotiation_string.leftJustified(128, (char)0, true);

    m_id = id;

    m_server = new TcpServer(this);

    setTonullptr(m_server);

    m_max_connections = max_connections;

    m_server->setMaxPendingConnections(m_max_connections);

    connect(m_server, &TcpServer::serverIncomingConnection, this, &SslServer::newConnectionPrivate);

    bool listening = m_server->listen(QHostAddress::AnyIPv4, port);

    if (port < 1)
    {
        emit error("Invalid port value!");
        stop();
        return;
    }

    if (!listening)
    {
        emit error(m_server->errorString());
        stop();
        return;
    }

    bool valid_keys = true;

    QByteArray test = QByteArray("TEST");
    QByteArray ret;

    if ((ret = OpenSslLib::publicEncrypt(m_public_key, test)) == QByteArray())
        valid_keys = false;
    else if (OpenSslLib::privateDecrypt(m_private_key, ret) != test)
        valid_keys = false;

    if (!valid_keys)
    {
        emit error("Invalid asymmetric keys!");
        stop();
        return;
    }

    m_global_ssl = new OpenSslLib(this);

    setTonullptr(m_global_ssl);

    m_global_ssl->EncryptInit(password, QByteArray());
    m_global_ssl->DecryptInit(password, QByteArray());
}

void SslServer::stop()
{
    while (!m_socket_list.isEmpty())
        removeSocket(m_socket_list.first());

    m_max_connections = 0;

    if (m_global_ssl)
        m_global_ssl->deleteLater();

    if (m_server)
    {
        m_server->close();
        m_server->deleteLater();
    }
}

void SslServer::rejectNewConnection()
{
    m_server->resumeAccepting();

    if (!m_pending_socket)
        return;

    QTcpSocket *socket = m_pending_socket;
    m_pending_socket = nullptr;

    socket->abort();
}

void SslServer::acceptNewConnection()
{
    m_server->resumeAccepting();

    if (!m_pending_socket)
        return;

    QTcpSocket *socket = m_pending_socket;
    m_pending_socket = nullptr;

    m_socket_list.append(socket);

    QHostAddress host = socket->peerAddress();
    qintptr descriptor = socket->socketDescriptor();

    connect(socket, &QTcpSocket::readyRead, this, &SslServer::readyReadPrivate);

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    emit connected(pd, m_id_hash.value(socket));

    emit socket->readyRead();
}

void SslServer::newConnectionPrivate(qintptr descriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(descriptor);

    if (m_max_connections == 0)
    {
        socket->abort();
        return;
    }

    //New connection done, set one less available connection
    m_max_connections--;

    QByteArray m_buffer;
    qint32 size = 0;
    bool ready = false;

    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &SslServer::timeout);
    timer->start(10 * 1000);

    m_descriptor_hash.insert(socket, descriptor);
    m_socket_hash.insert(descriptor, socket);
    m_buffer_hash.insert(socket, m_buffer);
    m_size_hash.insert(socket, size);
    m_timer_hash.insert(socket, timer);
    m_timer_socket_hash.insert(timer, socket);
    m_ready_hash.insert(socket, ready);

    connect(socket, &QTcpSocket::disconnected, this, &SslServer::disconnectedPrivate);

    connect(socket, &QTcpSocket::readyRead, this, &SslServer::readyBeginEncryption);

    socket->write(m_id.toLatin1());
    socket->write(m_global_ssl->Encrypt(m_public_key.leftJustified(1024, (char)0, true)));

    emit socket->readyRead();
}

void SslServer::readyBeginEncryption()
{
    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());

    if (socket->bytesAvailable() < MAX_ID_LENGTH + 512)
        return;

    QString id = QLatin1String(socket->read(MAX_ID_LENGTH));

    m_id_hash.insert(socket, id);

    QByteArray incomming_data = socket->read(512);

    QByteArray private_decrypted = OpenSslLib::privateDecrypt(m_private_key, incomming_data);

    QByteArray message = m_global_ssl->Decrypt(private_decrypted.mid(0, 128 + AES_BLOCK_SIZE));

    private_decrypted.remove(0, 128 + AES_BLOCK_SIZE);

    if (message.size() != 128)
    {
        socket->abort();
        return;
    }

    QByteArray pswd = private_decrypted.mid(0, 128);
    QByteArray salt = private_decrypted.mid(128, 128);

    OpenSslLib *ssl = new OpenSslLib(this);

    ssl->EncryptInit(pswd, salt);
    ssl->DecryptInit(pswd, salt);

    m_ssl_hash.insert(socket, ssl);

    disconnect(socket, &QTcpSocket::readyRead, this, &SslServer::readyBeginEncryption);

    connect(socket, &QTcpSocket::readyRead, this, &SslServer::verifier);

    emit socket->readyRead();
}

void SslServer::verifier()
{
    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());
    QTimer *timer = m_timer_hash.value(socket);
    OpenSslLib *ssl = m_ssl_hash.value(socket);

    if (socket->bytesAvailable() < 128 + AES_BLOCK_SIZE)
        return;

    bool negotiation_successfull = false;

    if (socket->bytesAvailable() == 128 + AES_BLOCK_SIZE)
    {
        QByteArray got_negotiation_string = ssl->Decrypt(socket->read(128 + AES_BLOCK_SIZE));

        if (got_negotiation_string == m_negotiation_string)
            negotiation_successfull = true;

        timer->stop();
    }

    if (!negotiation_successfull)
    {
        socket->abort();
        return;
    }

    disconnect(socket, &QTcpSocket::readyRead, this, &SslServer::verifier);

    m_pending_socket = socket;

    if (!m_auto_accept)
    {
        m_server->pauseAccepting();
        emit pending(socket->peerAddress(), m_id_hash.value(socket));
    }
    else
    {
        acceptNewConnection();
    }
}

void SslServer::timeout()
{
    QTimer *timer = static_cast<QTimer*>(sender());
    QTcpSocket *socket = m_timer_socket_hash.value(timer);

    socket->abort();
}

void SslServer::writeToHost(const QByteArray &plaindata, qintptr descriptor)
{
    if (!m_socket_hash.contains(descriptor))
        return;

    QTcpSocket *socket = m_socket_hash.value(descriptor);
    OpenSslLib *ssl = m_ssl_hash.value(socket);

    QByteArray encrypted = ssl->Encrypt(plaindata);

    socket->write(ssl->Encrypt(getBytes<qint32>(encrypted.size())));
    socket->write(encrypted);
}

int SslServer::writeToAll(const QByteArray &data)
{
    foreach (QTcpSocket *socket, m_socket_list)
    {
        qintptr descriptor = m_descriptor_hash.value(socket);
        writeToHost(data, descriptor);
    }

    return m_socket_list.size();
}

void SslServer::readyReadPrivate()
{
    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());

    QByteArray *m_buffer = &m_buffer_hash[socket];
    qint32 *size = &m_size_hash[socket];
    OpenSslLib *ssl = m_ssl_hash.value(socket);

    Q_UNUSED(size)
#define m_size *size
    while (socket->bytesAvailable() > 0)
    {
        m_buffer->append(socket->readAll());

        while ((m_size == 0 && m_buffer->size() >= AES_BLOCK_SIZE) || (m_size > 0 && m_buffer->size() >= m_size))
        {
            if (m_size == 0 && m_buffer->size() >= AES_BLOCK_SIZE)
            {
                m_size = getValue<qint32>(ssl->Decrypt(m_buffer->mid(0, AES_BLOCK_SIZE)));
                m_buffer->remove(0, AES_BLOCK_SIZE);

                if (m_size < 0 || m_size > MAX_NETWORK_CHUNK_SIZE)
                {
                    socket->abort();
                    return;
                }
            }
            if (m_size > 0 && m_buffer->size() >= m_size)
            {
                QByteArray encrypteddata = m_buffer->mid(0, m_size);
                m_buffer->remove(0, m_size);
                m_size = 0;

                QByteArray decrypteddata = ssl->Decrypt(encrypteddata);
                QHostAddress host = socket->peerAddress();
                qintptr descriptor = socket->socketDescriptor();

                PeerData pd;
                pd.data = decrypteddata;
                pd.host = host;
                pd.descriptor = descriptor;

                emit readyRead(pd);
            }
        }
    }
}

void SslServer::disconnectedPrivate()
{
    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());

    QHostAddress host = socket->peerAddress();
    qintptr descriptor = m_descriptor_hash.value(socket);

    removeSocket(socket);

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    //Peer disconnected, set one more available connection
    m_max_connections++;

    emit disconnected(pd);
}

void SslServer::removeSocket(QTcpSocket *socket)
{
    QTimer *timer = m_timer_hash.value(socket);
    OpenSslLib *ssl = m_ssl_hash.value(socket);

    if (m_pending_socket == socket)
        m_pending_socket = nullptr;

    qintptr descriptor = m_descriptor_hash.value(socket);

    m_socket_hash.remove(descriptor);
    m_descriptor_hash.remove(socket);
    m_id_hash.remove(socket);
    m_buffer_hash.remove(socket);
    m_size_hash.remove(socket);
    m_timer_hash.remove(socket);
    m_timer_socket_hash.remove(timer);
    m_ready_hash.remove(socket);
    m_ssl_hash.remove(socket);

    m_socket_list.removeAll(socket);

    socket->abort();


    if (timer) timer->deleteLater();
    if (ssl) ssl->deleteLater();
    socket->deleteLater();
}
