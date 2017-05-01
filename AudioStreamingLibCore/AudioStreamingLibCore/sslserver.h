#ifndef SSLSERVER_H
#define SSLSERVER_H

#include "abstractserver.h"
#include "openssllib.h"

class SslServer : public AbstractServer
{
    Q_OBJECT
public:
    explicit SslServer(QObject *parent = 0);
    ~SslServer();

public slots:
    void abort(qintptr descriptor);
    void setKeys(const QByteArray &private_key, const QByteArray &public_key);
    void listen(quint16 port, bool auto_accept, int max_connections,
                const QByteArray &negotiation_string,
                const QString &id,
                const QByteArray &password);
    void stop();
    void rejectNewConnection();
    void acceptNewConnection();
    void writeToHost(const QByteArray &data, qintptr descriptor);
    int writeToAll(const QByteArray &data);

private slots:
    bool testSsl();
    void newConnectionPrivate(qintptr descriptor);
    void readyBeginEncryption();
    void verifier();
    void timeout();
    void readyReadPrivate();
    void disconnectedPrivate();
    void removeSocket(QTcpSocket *socket);

private:
    TcpServer *m_server;

    QTcpSocket *m_pending_socket;
    OpenSslLib *m_global_ssl;

    QList<QTcpSocket*> m_socket_list;
    QHash<qintptr, QTcpSocket*> m_socket_hash;
    QHash<QTcpSocket*, qintptr> m_descriptor_hash;
    QHash<QTcpSocket*, QString> m_id_hash;
    QHash<QTcpSocket*, QByteArray> m_buffer_hash;
    QHash<QTcpSocket*, qint32> m_size_hash;
    QHash<QTcpSocket*, QTimer*> m_timer_hash;
    QHash<QTimer*, QTcpSocket*> m_timer_socket_hash;
    QHash<QTcpSocket*, bool> m_ready_hash;
    QHash<QTcpSocket*, OpenSslLib*> m_ssl_hash;

    QByteArray m_public_key;
    QByteArray m_private_key;
    int m_max_connections;
    bool m_auto_accept;
    QByteArray m_negotiation_string;
    QString m_id;
};

#endif // SSLSERVER_H
