#ifndef CLIENT_H
#define CLIENT_H

#include "abstractclient.h"

class Client : public AbstractClient
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    ~Client();

public slots:
    void abort();
    void connectToHost(const QString &host, quint16 port,
                       const QByteArray &negotiation_string,
                       const QString &id,
                       const QByteArray &password);
    void stop();
    int write(const QByteArray &data);

private slots:
    void timeout();
    void readID();
    void connectedPrivate();
    void disconnectedPrivate();
    void errorPrivate(QAbstractSocket::SocketError e);
    void readyReadPrivate();

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    qint32 m_size;

    QTimer *m_timer;
    QByteArray m_negotiation_string;
    QString m_remote_id;
    QString m_id;
};

#endif // CLIENT_H
