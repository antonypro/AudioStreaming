#ifndef ENCRYPTEDCLIENT_H
#define ENCRYPTEDCLIENT_H

#include "abstractclient.h"
#include "openssllib.h"

class EncryptedClient : public AbstractClient
{
    Q_OBJECT
public:
    explicit EncryptedClient(QObject *parent = nullptr);
    ~EncryptedClient();

public slots:
    void abort();
    void connectToHost(const QString &host, quint16 port,
                       const QByteArray &negotiation_string,
                       const QString &id,
                       const QByteArray &password);
    void connectToPeer(const QString &peer_id);
    void stop();
    int write(const QByteArray &data);
    void acceptSslCertificate();
    void acceptConnection();
    void rejectConnection();

private slots:
    bool testSsl();
    void timeout();
    void readyBeginEncryption();
    void connectedPrivate();
    void disconnectedPrivate();
    void errorPrivate(QAbstractSocket::SocketError e);
    void readyReadPrivate();

private:
    QTcpSocket *m_socket;
    OpenSslLib *m_openssl;
    QByteArray m_buffer;
    qint32 m_size;
    QTimer *m_timer;
    QByteArray m_negotiation_string;
    QString m_id;
    QString m_remote_id;
};

#endif // ENCRYPTEDCLIENT_H
