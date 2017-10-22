#ifndef SSLCLIENT_H
#define SSLCLIENT_H

#include "abstractclient.h"
#include "openssllib.h"

class SslClient : public AbstractClient
{
    Q_OBJECT
public:
    explicit SslClient(QObject *parent = nullptr);
    ~SslClient();

public slots:
    void abort();
    void connectToHost(const QString &host, quint16 port,
                       const QByteArray &negotiation_string,
                       const QString &id,
                       const QByteArray &password);
    void stop();
    int write(const QByteArray &data);

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
    OpenSslLib *m_global_ssl;
    QByteArray m_buffer;
    qint32 m_size;
    OpenSslLib *ssl;
    QTimer *m_timer;
    QByteArray m_negotiation_string;
    QString m_id;
    QString m_remote_id;
    QByteArray m_rnd;
};

#endif // SSLCLIENT_H
