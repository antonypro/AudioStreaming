#include "sslclient.h"

SslClient::SslClient(QObject *parent) : QObject(parent)
{
    m_size = 0;
    m_socket = nullptr;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SslClient::timeout);
    m_timer->setSingleShot(true);
}

SslClient::~SslClient()
{
    stop();
}

void SslClient::connectToHost(const QString &host, quint16 port)
{
    if (m_socket)
        return;

    m_socket = new QSslSocket(this);
    SETTONULLPTR(m_socket);

    connect(m_socket, &QSslSocket::encrypted, this, &SslClient::encrypted);
    connect(m_socket, &QSslSocket::disconnected, this, &SslClient::disconnectedPrivate);
    connect(m_socket, static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), this, &SslClient::sslErrors);
    connect(m_socket, static_cast<void(QSslSocket::*)(QSslSocket::SocketError)>(&QSslSocket::error), this, &SslClient::errorPrivate);

    m_timer->start(TIMEOUT);

    m_socket->setProtocol(QSsl::TlsV1_2OrLater);

    m_socket->connectToHostEncrypted(host, port);
}

void SslClient::sslErrors(QList<QSslError> errors)
{
    Q_UNUSED(errors)

    if (!m_socket)
        return;

    m_socket->ignoreSslErrors();
}

void SslClient::timeout()
{
    emit error("Operation timed out");
    stop();
}

void SslClient::encrypted()
{
    m_timer->stop();

    connect(m_socket, &QSslSocket::readyRead, this, &SslClient::readyReadPrivate);

    QSslCertificate certificate = m_socket->peerCertificate();

    QByteArray hash = certificate.digest(QCryptographicHash::Sha256).toHex().toUpper();

    QByteArray formatted_hash;

    for (int i = hash.size() - 2; i >= 0; i -= 2)
    {
        formatted_hash.prepend(hash.mid(i, 2));
        formatted_hash.prepend(":");
    }

    formatted_hash.remove(0, 1);

    emit connectedToServer(formatted_hash);
}

void SslClient::disconnectedPrivate()
{
    if (!m_socket)
        return;

    stop();

    emit disconnected();
}

void SslClient::errorPrivate(QAbstractSocket::SocketError e)
{
    Q_UNUSED(e)

    QString err = m_socket->errorString();

    emit error(err);

    disconnectedPrivate();

    stop();
}

void SslClient::stop()
{
    m_timer->stop();

    if (!m_socket)
        return;

    m_socket->blockSignals(true);

    m_socket->abort();
    m_socket->deleteLater();

    m_size = 0;
    m_buffer.clear();
}

void SslClient::disconnectFromPeer()
{
    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::DisconnectedFromPeer));

    write(data);
}

int SslClient::write(const QByteArray &data)
{
    if (!m_socket)
        return 0;

    m_socket->write(getBytes<qint32>(data.size()));
    m_socket->write(data);

    return 1;
}

void SslClient::readyReadPrivate()
{
    while (m_socket->bytesAvailable() > 0)
    {
        m_buffer.append(m_socket->readAll());

        while ((m_size == 0 && m_buffer.size() >= 4) || (m_size > 0 && m_buffer.size() >= m_size))
        {
            if (m_size == 0 && m_buffer.size() >= 4)
            {
                m_size = getValue<qint32>(m_buffer.mid(0, 4));
                m_buffer.remove(0, 4);
            }
            if (m_size > 0 && m_buffer.size() >= m_size)
            {
                QByteArray data = m_buffer.mid(0, m_size);
                m_buffer.remove(0, m_size);
                m_size = 0;
                processInput(data);
            }
        }
    }
}

void SslClient::processInput(const QByteArray &peer_data)
{
    QByteArray data = peer_data;

    quint8 command = getValue<quint8>(data.mid(0, 1));
    data.remove(0, 1);

    switch (command)
    {
    case ServerCommand::ConnectionRequested:
    {
        if (data.size() != 4 + 20)
        {
            m_socket->abort();
            return;
        }

        quint32 host = getValue<quint32>(data.mid(0, 4));
        data.remove(0, 4);

        emit pending(host, data);

        break;
    }
    case ServerCommand::LoggedIn:
    {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &SslClient::alive);
        timer->start(5000);

        emit webClientLoggedIn();

        break;
    }
    case ServerCommand::ConnectedToPeer:
    {
        if (data.size() != 20 + 32)
        {
            disconnectFromPeer();
            break;
        }

        emit connectedToPeer(data.mid(0, 20), data.mid(20, 32));

        break;
    }
    case ServerCommand::DisconnectedFromPeer:
    {
        emit disconnectedFromPeer();

        break;
    }
    case ServerCommand::P2PData:
    {
        emit P2PData(data);

        break;
    }
    case ServerCommand::Warning:
    {
        emit webClientWarning(QLatin1String(data));

        break;
    }
    default:
        break;
    }
}

void SslClient::alive()
{
    QByteArray result;
    result.append(getBytes<quint8>(ServerCommand::Alive));

    write(result);
}
