#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include "abstractclient.h"
#include "openssllib.h"
#include "sslclient.h"
#include "common.h"

class WebClient : public AbstractClient
{
    Q_OBJECT
public:
    explicit WebClient(QObject *parent = nullptr);
    ~WebClient();

public slots:
    void abort();
    void connectToHost(const QString &host, quint16 port,
                       const QByteArray &negotiation_string,
                       const QString &id,
                       const QByteArray &password);
    void connectToPeer(const QString &peer_id);
    void stop();
    int write(const QByteArray &data);
    void acceptConnection();
    void rejectConnection();
    void acceptSslCertificate();

private slots:
    bool testSsl();
    void connectedToServerPrivate(const QByteArray &hash);
    void disconnectedFromServer();
    void pendingPrivate(quint32 host, const QByteArray &id);
    void connectToP2PServer(quint32 ip, const QByteArray &id, const QByteArray &password);
    void startP2PConnection();
    void readyToGetPort();
    void remotePort(quint16 port);
    void readyToConnectToPeer();
    void attemptToConnect();
    void connectP2P();
    void confirmP2PConnection();
    void confirmedP2PConnection();
    void connectedP2P();
    void disconnectedPrivate();
    void errorPrivate(QAbstractSocket::SocketError e);
    void readyReadPrivate();
    void timeout();

private:
    int m_i;
    qintptr m_descriptor;
    quint16 m_local_port;
    bool m_ssl_certificate_accepted;
    QTcpSocket *m_socket;
    OpenSslLib *m_openssl;
    SslClient *m_ssl_client;
    QByteArray m_buffer;
    qint32 m_size;
    QTimer *m_timer;
    QByteArray m_negotiation_string;
    QString m_id;
    QByteArray m_password;
    QString m_host;
    quint16 m_host_port;
    quint32 m_peer_ip;
    quint16 m_peer_port;
    QByteArray m_peer_id;
};

#endif // WEBCLIENT_H
