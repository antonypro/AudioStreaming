#include "client.h"

Client::Client(QObject *parent) : AbstractClient(parent)
{
    m_size = 0;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Client::timeout);
    m_timer->setSingleShot(true);
}

Client::~Client()
{
    disconnectedPrivate();
}

void Client::abort()
{
    if (m_socket)
        m_socket->abort();
}

void Client::connectToHost(const QString &host, quint16 port,
                           const QByteArray &negotiation_string,
                           const QString &id,
                           const QByteArray &password)
{
    Q_UNUSED(password)

    if (m_socket)
        return;

    m_negotiation_string = negotiation_string.leftJustified(128, char(0), true);
    m_id = id;

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::disconnected, this, &Client::disconnectedPrivate);
    connect(m_socket, static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error), this, &Client::errorPrivate);

    connect(m_socket, &QTcpSocket::readyRead, this, &Client::readID);

    m_timer->start(TIMEOUT);

    m_socket->connectToHost(host, port);
}

void Client::writeCommandXML(const QByteArray &XML)
{
    Q_UNUSED(XML)
}

void Client::connectToPeer(const QString &peer_id)
{
    Q_UNUSED(peer_id)
}

void Client::disconnectFromPeer()
{

}

void Client::acceptSslCertificate()
{

}

void Client::acceptConnection()
{

}

void Client::rejectConnection()
{

}

void Client::timeout()
{
    emit error("Operation timed out");
    stop();
}

void Client::readID()
{
    if (m_socket->bytesAvailable() < MAX_ID_LENGTH)
        return;

    m_remote_id = QLatin1String(m_socket->read(MAX_ID_LENGTH));

    disconnect(m_socket, &QTcpSocket::readyRead, this, &Client::readID);

    connect(m_socket, &QTcpSocket::readyRead, this, &Client::readyReadPrivate);

    connectedPrivate();

    emit m_socket->readyRead();
}

void Client::connectedPrivate()
{
    m_socket->write(m_id.toLatin1());
    m_socket->write(m_negotiation_string);

    QHostAddress host = m_socket->peerAddress();
    qintptr descriptor = m_socket->socketDescriptor();

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    emit connected(pd, m_remote_id);

    m_timer->stop();
}

void Client::disconnectedPrivate()
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

void Client::errorPrivate(QAbstractSocket::SocketError e)
{
    Q_UNUSED(e)

    QString err = m_socket->errorString();

    emit error(err);
}

void Client::stop()
{
    m_timer->stop();

    m_size = 0;
    m_buffer.clear();

    if (m_socket)
    {
        m_socket->abort();
        m_socket->deleteLater();
    }
}

int Client::write(const QByteArray &data)
{
    if (!m_socket)
        return 0;

    m_socket->write(getBytes<qint32>(data.size()));
    m_socket->write(data);

    return 1;
}

void Client::readyReadPrivate()
{
    if (!m_socket)
        return;

    while (m_socket->bytesAvailable() > 0)
    {
        m_buffer.append(m_socket->readAll());

        while ((m_size == 0 && m_buffer.size() >= 4) || (m_size > 0 && m_buffer.size() >= m_size))
        {
            if (m_size == 0 && m_buffer.size() >= 4)
            {
                m_size = getValue<qint32>(m_buffer.mid(0, 4));
                m_buffer.remove(0, 4);

                if (m_size < 0 || m_size > MAX_NETWORK_CHUNK_SIZE)
                {
                    m_socket->abort();
                    return;
                }
            }
            if (m_size > 0 && m_buffer.size() >= m_size)
            {
                QByteArray data = m_buffer.mid(0, m_size);
                m_buffer.remove(0, m_size);
                m_size = 0;

                QHostAddress host = m_socket->peerAddress();
                qintptr descriptor = m_socket->socketDescriptor();

                PeerData pd;
                pd.data = data;
                pd.host = host;
                pd.descriptor = descriptor;

                emit readyRead(pd);
            }
        }
    }
}
