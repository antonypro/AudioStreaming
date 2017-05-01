#include "sslclient.h"

SslClient::SslClient(QObject *parent) : AbstractClient(parent)
{
    m_socket = nullptr;
    m_size = 0;
    m_global_ssl = nullptr;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SslClient::timeout);
    m_timer->setSingleShot(true);

    ssl = new OpenSslLib(this);
}

SslClient::~SslClient()
{
    disconnectedPrivate();
}

void SslClient::abort()
{
    if (m_socket)
        m_socket->abort();
}

bool SslClient::testSsl()
{
    if (!ssl->isLoaded())
        emit error("OpenSsl not loaded!");

    return ssl->isLoaded();
}

void SslClient::connectToHost(const QString &host, quint16 port,
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

    setTonullptr(m_socket);

    connect(m_socket, &QTcpSocket::disconnected, this, &SslClient::disconnectedPrivate);
    connect(m_socket, static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error), this, &SslClient::errorPrivate);

    connect(m_socket, &QTcpSocket::readyRead, this, &SslClient::readyBeginEncryption);

    m_timer->start(10 * 1000);

    m_socket->connectToHost(host, port);

    m_global_ssl = new OpenSslLib(this);

    setTonullptr(m_global_ssl);

    m_global_ssl->EncryptInit(password, QByteArray());
    m_global_ssl->DecryptInit(password, QByteArray());

    emit m_socket->readyRead();
}

void SslClient::timeout()
{
    emit error("Operation timed out");
    stop();
}

void SslClient::readyBeginEncryption()
{
    if (m_socket->bytesAvailable() < MAX_ID_LENGTH + 1024 + AES_BLOCK_SIZE)
        return;

    m_remote_id = QLatin1String(m_socket->read(MAX_ID_LENGTH));

    QByteArray incomming_data = m_socket->readAll();

    if (incomming_data.size() > 1024 + AES_BLOCK_SIZE)
    {
        emit error("Ssl handshake failed!");
        return;
    }

    QByteArray public_key = m_global_ssl->Decrypt(incomming_data);

    if (public_key.size() != 1024)
    {
        emit error("Password incorrect!");
        return;
    }

    m_rnd = OpenSslLib::RANDbytes(256);

    QByteArray data;
    data.append(m_global_ssl->Encrypt(OpenSslLib::RANDbytes(128)));
    data.append(m_rnd);

    QByteArray public_encrypted = OpenSslLib::publicEncrypt(public_key, data);

    m_socket->write(m_id.toLatin1());
    m_socket->write(public_encrypted);

    QByteArray pswd = m_rnd.mid(0, 128);
    QByteArray salt = m_rnd.mid(128, 128);

    ssl->EncryptInit(pswd, salt);
    ssl->DecryptInit(pswd, salt);

    disconnect(m_socket, &QTcpSocket::readyRead, this, &SslClient::readyBeginEncryption);

    connect(m_socket, &QTcpSocket::readyRead, this, &SslClient::readyReadPrivate);

    m_socket->write(ssl->Encrypt(m_negotiation_string));

    connectedPrivate();

    emit m_socket->readyRead();

    m_timer->stop();
}

void SslClient::connectedPrivate()
{
    QHostAddress host = m_socket->peerAddress();
    qintptr descriptor = m_socket->socketDescriptor();

    PeerData pd;
    pd.host = host;
    pd.descriptor = descriptor;

    emit connected(pd, m_remote_id);
}

void SslClient::disconnectedPrivate()
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

void SslClient::errorPrivate(QAbstractSocket::SocketError e)
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

void SslClient::stop()
{
    m_timer->stop();

    m_size = 0;
    m_buffer.clear();

    if (m_global_ssl)
        m_global_ssl->deleteLater();

    if (m_socket)
    {
        m_socket->abort();
        m_socket->deleteLater();
    }
}

int SslClient::write(const QByteArray &data)
{
    if (!m_socket)
        return 0;

    QByteArray encrypted = ssl->Encrypt(data);

    m_socket->write(ssl->Encrypt(getBytes<qint32>(encrypted.size())));
    m_socket->write(encrypted);

    return 1;
}

void SslClient::readyReadPrivate()
{
    while (m_socket->bytesAvailable() > 0)
    {
        m_buffer.append(m_socket->readAll());

        while ((m_size == 0 && m_buffer.size() >= AES_BLOCK_SIZE) || (m_size > 0 && m_buffer.size() >= m_size))
        {
            if (m_size == 0 && m_buffer.size() >= AES_BLOCK_SIZE)
            {
                m_size = getValue<qint32>(ssl->Decrypt(m_buffer.mid(0, AES_BLOCK_SIZE)));
                m_buffer.remove(0, AES_BLOCK_SIZE);

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

                QByteArray decrypteddata = ssl->Decrypt(encrypteddata);
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
