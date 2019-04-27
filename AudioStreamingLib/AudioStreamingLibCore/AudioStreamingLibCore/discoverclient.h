#ifndef DISCOVERCLIENT_H
#define DISCOVERCLIENT_H

#include <QtCore>
#include <QtNetwork>

//\cond HIDDEN_SYMBOLS
class DiscoverClient : public QObject
{
    Q_OBJECT
public:
    explicit DiscoverClient(QObject *parent = nullptr);

signals:
    void peerFound(QHostAddress,QString);
    void error(QString);

public slots:
    void discover(quint16 port, const QByteArray &negotiation_string);

private slots:
    void updateListEndpoints();
    void write();
    void readyRead();

private:
    QPointer<QUdpSocket> m_socket;
    QByteArray m_negotiation_string;
    QList<quint32> m_got_list;
    quint16 m_port;
    QList<QHostAddress> m_address_list;
};
//\endcond

#endif // DISCOVERCLIENT_H
