#include "webclient.h"

WebClient::WebClient(QObject *parent) : AbstractClient(parent)
{
    m_ssl_certificate_accepted = false;

    m_openssl = new OpenSslLib(this);
}

WebClient::~WebClient()
{
    stop();
}

void WebClient::abort()
{

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
    Q_UNUSED(negotiation_string)
    Q_UNUSED(id)
    Q_UNUSED(password)

    if (!testSsl())
        return;

    if (m_ssl_client)
        return;

    m_ssl_client = new SslClient(this);

    connect(m_ssl_client, &SslClient::connectedToServer, this, &WebClient::connectedToServerPrivate);
    connect(m_ssl_client, &SslClient::disconnected, this, &WebClient::disconnectedFromServer);
    connect(m_ssl_client, &SslClient::pending, this, &WebClient::pendingPrivate);
    connect(m_ssl_client, &SslClient::webClientLoggedIn, this, &WebClient::webClientLoggedIn);
    connect(m_ssl_client, &SslClient::P2PData, this, &WebClient::readyReadPrivate);
    connect(m_ssl_client, &SslClient::connectedToPeer, this, &WebClient::connectedToPeerPrivate);
    connect(m_ssl_client, &SslClient::disconnectedFromPeer, this, &WebClient::disconnectedFromPeerPrivate);
    connect(m_ssl_client, &SslClient::webClientWarning, this, &WebClient::webClientWarning);
    connect(m_ssl_client, &SslClient::commandXML, this, &WebClient::commandXML);
    connect(m_ssl_client, &SslClient::error, this, &WebClient::error);

    m_ssl_client->connectToHost(host, port);
}

void WebClient::writeCommandXML(const QByteArray &XML)
{
    if (!m_ssl_client || !m_ssl_certificate_accepted)
        return;

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::XML));
    data.append(XML);

    m_ssl_client->write(data);
}

void WebClient::connectedToServerPrivate(const QByteArray &hash)
{
    if (!m_ssl_client)
        return;

    emit connectedToServer(hash);
}

void WebClient::disconnectedFromServer()
{
    emit error("Connection with server lost!");
    stop();
}

void WebClient::acceptSslCertificate()
{
    m_ssl_certificate_accepted = true;
}

void WebClient::connectToPeer(const QString &peer_id)
{
    if (!m_ssl_client || !m_ssl_certificate_accepted)
        return;

    QByteArray id = cleanString(peer_id).toLatin1();

    id = id.leftJustified(20, char(0), true);

    QByteArray data;
    data.append(getBytes<quint8>(ServerCommand::PeerTryConnect));
    data.append(id);

    m_ssl_client->write(data);
}

void WebClient::disconnectFromPeer()
{
    if (!m_ssl_client || !m_ssl_certificate_accepted)
        return;

    m_ssl_client->disconnectFromPeer();
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

void WebClient::connectedToPeerPrivate(const QByteArray &id, const QByteArray &password)
{
    m_openssl->setPassword(password);

    m_peer_id = id;

    emit connectedToPeer(QLatin1String(id));
}

void WebClient::disconnectedFromPeerPrivate()
{
    emit disconnectedFromPeer();
}

void WebClient::stop()
{
    if (m_openssl)
        m_openssl->deleteLater();

    if (m_ssl_client)
        m_ssl_client->deleteLater();
}

int WebClient::write(const QByteArray &data)
{
    if (!m_ssl_client)
        return 0;

    QByteArray encrypted = m_openssl->encrypt(data);

    QByteArray data1;
    data1.append(getBytes<quint8>(ServerCommand::P2PData));
    data1.append(encrypted);

    m_ssl_client->write(data1);

    return 1;
}

void WebClient::readyReadPrivate(const QByteArray &data)
{
    QByteArray decrypteddata = m_openssl->decrypt(data);

    if (decrypteddata.isNull())
    {
        disconnectFromPeer();
        return;
    }

    QHostAddress host = QHostAddress();
    qintptr descriptor = 0;

    PeerData pd;
    pd.data = decrypteddata;
    pd.host = host;
    pd.descriptor = descriptor;

    emit readyRead(pd);
}
