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
    void newConnectionPrivate(qintptr descriptor);
    void timeout(QSslSocket *socket);
    void testConnections();
    void sslErrors(QList<QSslError> errors);
    void encrypted();
    void readyReadPrivate();
    void process(const PeerData &pd);
    void processCommandXML(QSslSocket *socket, const QByteArray &data);
    void writeCommandXML(QSslSocket *socket, const QByteArray &XML);
    void disconnectedPrivate();
    void removeSocket(QSslSocket *socket);
    void writeWarning(QSslSocket *socket, const QString &message, bool remove_socket = false);
    void disconnectedFromPeer(QSslSocket *socket);

private:
    //Functions
    void incomingConnection(qintptr handle);

    //Variables
    QPointer<TcpServer> m_server;

    Sql m_sql;

    QSslKey m_key;
    QSslCertificate m_cert;

    QList<QSslSocket*> m_socket_list;
    QList<QSslSocket*> m_connecting_connected_list;
    QStringList m_code_list;
    QByteArrayList m_id_list;
    QByteArrayList m_unavailable_list;
    QHash<qintptr, QSslSocket*> m_socket_hash;
    QHash<QSslSocket*, QByteArray> m_id_hash;
    QHash<QSslSocket*, qintptr> m_descriptor_hash;
    QHash<QSslSocket*, QByteArray> m_buffer_hash;
    QHash<QSslSocket*, qint32> m_size_hash;
    QHash<QSslSocket*, QSslSocket*> m_accept_hash;
    QHash<QSslSocket*, QSslSocket*> m_connected_1;
    QHash<QSslSocket*, QSslSocket*> m_connected_2;
    QHash<QSslSocket*, QElapsedTimer> m_alive_hash;

    int m_max_connections;
};

#endif // SSLSERVER_H
