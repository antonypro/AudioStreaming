#ifndef DISCOVERSERVER_H
#define DISCOVERSERVER_H

#include <QtCore>
#include <QtNetwork>

class DiscoverServer : public QObject
{
    Q_OBJECT
public:
    explicit DiscoverServer(QObject *parent = nullptr);
    ~DiscoverServer();

signals:
    void error(QString);

public slots:
    void listen(quint16 port, const QByteArray &negotiation_string, const QString &id);

private slots:
    void readyRead();

private:
    QUdpSocket *m_server;
    QByteArray m_negotiation_string;
    QString m_id;
};

#endif // DISCOVERSERVER_H
