#include "server.h"

Server::Server(QObject *parent) : AbstractServer(parent)
{
    m_max_connections = 0;
    m_auto_accept = false;
}

Server::~Server()
{
    stop();
}

void Server::abort(qintptr descriptor)
{
    QTcpSocket *socket = m_socket_hash.value(descriptor);
    socket->abort();
}

void Server::listen(quint16 port, bool auto_accept, int max_connections,
                    const QByteArray &negotiation_string,
                    const QString &id,
                    const QByteArray &password)
{
    Q_UNUSED(password)

    if (m_server)
        return;

    m_auto_accept = auto_accept;

    m_negotiation_string = negotiation_string.leftJustified(128, char(0), true);

    m_id = id;

    m_server = new TcpServer(this);

    m_max_connections = max_connections;

    m_server->setMaxPendingConnections(m_max_connections);

    connect(m_server, &TcpServer::serverIncomingConnection, this, &Server::newConnectionPrivate);

    bool listening = m_server->listen(QHostAddress::AnyIPv4, port);

    if (port < 1)
    {
        emit error("Invalid port value");
        stop();
        return;
    }

    if (!listening)
    {
        emit error(m_server->errorString());
        stop();
        return;
    }
}

void Server::stop()
{
    while (!m_socket_list.isEmpty())
        removeSocket(m_socket_list.first());

    m_max_connections = 0;

    if (m_server)
    {
        m_server->close();
        m_server->deleteLater();
    }
}

void Server::rejectNewConnection()
{
    m_server->resumeAccepting();

    if (!m_pending_socket)
        return;

    QTcpSocket *socket = m_pending_socket;
    m_pending_socket = nullptr;

    socket->abort();
}

void Server::acceptNewConnection()
{
    m_server->resumeAccepting();

    if (!m_pending_socket)
        return;

    QTcpSocket *socket = m_pending_socket;
    m_pending_socket = nullptr;

    m_socket_list.append(socket);

    QHostAddress host = socket->peerAddress();
    qintptr descriptor = socket->socketDescriptor();

    connect(socket, &QTcpSocket::readyRead, this, &Server::readyReadPrivate);

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    emit connected(pd, m_id_hash.value(socket));
}

void Server::newConnectionPrivate(qintptr descriptor)
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

    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &Server::timeout);
    timer->start(TIMEOUT);

    m_descriptor_hash.insert(socket, descriptor);
    m_socket_hash.insert(descriptor, socket);
    m_buffer_hash.insert(socket, m_buffer);
    m_size_hash.insert(socket, size);
    m_timer_hash.insert(socket, timer);
    m_timer_socket_hash.insert(timer, socket);

    connect(socket, &QTcpSocket::disconnected, this, &Server::disconnectedPrivate);
    connect(socket, &QTcpSocket::readyRead, this, &Server::verifier);

    socket->write(m_id.toLatin1());
}

void Server::verifier()
{
    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());
    QTimer *timer = m_timer_hash.value(socket);

    if (socket->bytesAvailable() < MAX_ID_LENGTH + 128)
        return;

    bool negotiation_successfull = false;

    if (socket->bytesAvailable() == MAX_ID_LENGTH + 128)
    {
        QString id = QLatin1String(socket->read(MAX_ID_LENGTH));

        m_id_hash.insert(socket, id);

        QByteArray got_negotiation_string = socket->read(128);

        if (got_negotiation_string == m_negotiation_string)
            negotiation_successfull = true;

        timer->stop();
    }
    else
    {
        return;
    }

    if (!negotiation_successfull)
    {
        socket->abort();
        return;
    }

    disconnect(socket, &QTcpSocket::readyRead, this, &Server::verifier);

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

void Server::timeout()
{
    QTimer *timer = static_cast<QTimer*>(sender());
    QTcpSocket *socket = m_timer_socket_hash.value(timer);

    socket->abort();
}

void Server::writeToHost(const QByteArray &data, qintptr descriptor)
{
    if (!m_socket_hash.contains(descriptor))
        return;

    QTcpSocket *socket = m_socket_hash.value(descriptor);

    socket->write(getBytes<qint32>(data.size()));
    socket->write(data);
}

int Server::writeToAll(const QByteArray &data)
{
    for (QTcpSocket *socket : m_socket_list)
    {
        socket->write(getBytes<qint32>(data.size()));
        socket->write(data);
    }

    return m_socket_list.size();
}

void Server::readyReadPrivate()
{
    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());

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

                emit readyRead(pd);
            }
        }
    }
}

void Server::disconnectedPrivate()
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

void Server::removeSocket(QTcpSocket *socket)
{
    QTimer *timer = m_timer_hash.value(socket);

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

    m_socket_list.removeAll(socket);

    socket->abort();

    if (timer) timer->deleteLater();
    socket->deleteLater();
}
