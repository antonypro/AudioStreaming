#ifndef DISCOVERCLIENT_H
#define DISCOVERCLIENT_H

#include <QtCore>
#include <QtNetwork>

class DiscoverClient : public QObject
{
    Q_OBJECT
public:
    explicit DiscoverClient(QObject *parent = 0);

signals:
    void peerFound(QHostAddress,QString);
    void error(QString);

public slots:
    void discover(quint16 port, const QByteArray &negotiation_string);

private slots:
    void write();
    void readyRead();

private:
    QUdpSocket *m_socket;
    QByteArray m_negotiation_string;
    QList<quint32> m_got_list;
    quint16 m_port;
    QList<QHostAddress> m_address_list;
};

#endif // DISCOVERCLIENT_H
