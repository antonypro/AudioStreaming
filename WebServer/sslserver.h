#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QtCore>
#include <QtNetwork>
#include "tcpserver.h"
#include "certificate.h"
#include "openssllib.h"
#include "sql.h"
#include "common.h"

class SslServer : public QObject
{
    Q_OBJECT
public:
    explicit SslServer(QObject *parent = nullptr);
    ~SslServer();

public slots:
    void listen();
    void stop();

private slots:
    void newP2P();
    void newConnectionPrivate(qintptr descriptor);
    void timeout();
    void sslErrors(QList<QSslError> errors);
    void encrypted();
    void readyReadPrivate();
    void process(const PeerData &pd);
    void disconnectedPrivate();
    void removeSocket(QSslSocket *socket);

private:
    //Functions
    void incomingConnection(qintptr handle);

    //Variables
    TcpServer *m_server;
    QTcpServer *m_server_p2p;

    Sql m_sql;

    QSslKey m_key;
    QSslCertificate m_cert;

    QList<QSslSocket*> m_socket_list;
    QByteArrayList m_id_list;
    QHash<qintptr, QSslSocket*> m_socket_hash;
    QHash<QSslSocket*, QByteArray> m_id_hash;
    QHash<QSslSocket*, qintptr> m_descriptor_hash;
    QHash<QSslSocket*, QByteArray> m_buffer_hash;
    QHash<QSslSocket*, qint32> m_size_hash;
    QHash<QSslSocket*, QTimer*> m_timer_hash;
    QHash<QSslSocket*, QSslSocket*> m_accept_hash;
    QHash<QSslSocket*, QByteArray> m_port_hash;

    int m_max_connections;
};

#endif // SSLSERVER_H
