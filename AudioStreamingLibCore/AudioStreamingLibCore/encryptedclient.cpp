#include "encryptedclient.h"

EncryptedClient::EncryptedClient(QObject *parent) : AbstractClient(parent)
{
    m_socket = nullptr;
    m_size = 0;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &EncryptedClient::timeout);
    m_timer->setSingleShot(true);

    m_openssl = new OpenSslLib(this);
    SETTONULLPTR(m_openssl);
}

EncryptedClient::~EncryptedClient()
{
    disconnectedPrivate();
}

void EncryptedClient::abort()
{
    if (m_socket)
        m_socket->abort();
}

bool EncryptedClient::testSsl()
{
    if (!m_openssl->isLoaded())
        emit error("OpenSsl not loaded!");

    return m_openssl->isLoaded();
}

void EncryptedClient::connectToHost(const QString &host, quint16 port,
                                    const QByteArray &negotiation_string,
                                    const QString &id,
                                    const QByteArray &password)
{
    if (!testSsl())
        return;

    if (m_socket)
        return;

    m_negotiation_string = negotiation_string.leftJustified(128, (char)0, true);
    m_id = id;

    m_socket = new QTcpSocket(this);

    SETTONULLPTR(m_socket);

    connect(m_socket, &QTcpSocket::disconnected, this, &EncryptedClient::disconnectedPrivate);
    connect(m_socket, static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error), this, &EncryptedClient::errorPrivate);

    connect(m_socket, &QTcpSocket::readyRead, this, &EncryptedClient::readyBeginEncryption);

    m_timer->start(10 * 1000);

    m_openssl->setPassword(password);

    emit m_socket->readyRead();

    m_socket->connectToHost(host, port);
}

void EncryptedClient::connectToPeer(const QString &peer_id)
{
    Q_UNUSED(peer_id)
}

void EncryptedClient::acceptSslCertificate()
{

}

void EncryptedClient::acceptConnection()
{

}

void EncryptedClient::rejectConnection()
{

}

void EncryptedClient::timeout()
{
    emit error("Operation timed out");
    stop();
}

void EncryptedClient::readyBeginEncryption()
{
    if (m_socket->bytesAvailable() < MAX_ID_LENGTH)
        return;

    m_remote_id = QLatin1String(m_socket->read(MAX_ID_LENGTH));

    disconnect(m_socket, &QTcpSocket::readyRead, this, &EncryptedClient::readyBeginEncryption);

    connect(m_socket, &QTcpSocket::readyRead, this, &EncryptedClient::readyReadPrivate);

    m_socket->write(m_id.toLatin1());
    m_socket->write(m_openssl->encrypt(m_negotiation_string));

    connectedPrivate();

    emit m_socket->readyRead();

    m_timer->stop();
}

void EncryptedClient::connectedPrivate()
{
    QHostAddress host = m_socket->peerAddress();
    qintptr descriptor = m_socket->socketDescriptor();

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    emit connected(pd, m_remote_id);
}

void EncryptedClient::disconnectedPrivate()
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

void EncryptedClient::errorPrivate(QAbstractSocket::SocketError e)
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

void EncryptedClient::stop()
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
}

int EncryptedClient::write(const QByteArray &data)
{
    if (!m_socket)
        return 0;

    QByteArray encrypted = m_openssl->encrypt(data);

    m_socket->write(m_openssl->encrypt(getBytes<qint32>(encrypted.size())));
    m_socket->write(encrypted);

    return 1;
}

void EncryptedClient::readyReadPrivate()
{
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
