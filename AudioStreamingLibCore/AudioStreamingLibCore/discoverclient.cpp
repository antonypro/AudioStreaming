#include "discoverclient.h"
#include "audiostreaminglibcore.h"

DiscoverClient::DiscoverClient(QObject *parent) : QObject(parent)
{
    m_port = 0;
}

void DiscoverClient::discover(quint16 port, const QByteArray &negotiation_string)
{
    m_socket = new QUdpSocket(this);

    m_port = port;

    m_negotiation_string = negotiation_string.leftJustified(128, char(0), true);

    connect(m_socket, &QUdpSocket::readyRead, this, &DiscoverClient::readyRead);

    updateListEndpoints();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DiscoverClient::write);
    timer->start(100);
}

void DiscoverClient::updateListEndpoints()
{
    m_address_list.clear();

    for (const QHostAddress &address : QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
            quint32 ip = address.toIPv4Address();

            quint8 octets[4];
            quint32 parts[4];

            for (int i = 1; i <= 4; i++)
            {
                quint32 num = (ip / quint32(qPow(256, 4 - i)));
                ip = (ip - num * quint32(qPow(256, 4 - i)));
                octets[i - 1] = quint8(num);
            }

            for (int i = 1; i <= 4; i++)
            {
                parts[i - 1] = (octets[i - 1] * quint32(qPow(256, 4 - i)));
            }

            parts[4 - 1] = 255;

            quint32 new_ip = (parts[0] + parts[1] + parts[2] + parts[3]);

            QHostAddress address(ip);

            QHostAddress address_broadcast(new_ip);

            m_address_list.append(address_broadcast);
        }
    }
}

void DiscoverClient::write()
{
    for (const QHostAddress &address : m_address_list)
        m_socket->writeDatagram(m_negotiation_string, address, m_port);
}

void DiscoverClient::readyRead()
{
    while (m_socket->hasPendingDatagrams())
    {
        QByteArray data;
        data.resize(int(m_socket->pendingDatagramSize()));

        QHostAddress host;
        quint16 port;

        m_socket->readDatagram(data.data(), data.size(), &host, &port);

        if (data.size() != 128 + MAX_ID_LENGTH)
            continue;

        QByteArray negotiation_string = data.mid(0, 128);
        data.remove(0, 128);

        QString id = QLatin1String(data.mid(0, MAX_ID_LENGTH));
        data.remove(0, MAX_ID_LENGTH);

        QList<quint32> ip_list;

        for (const QHostAddress &host : QNetworkInterface::allAddresses())
            ip_list.append(host.toIPv4Address());

        ip_list.removeAll(QHostAddress(QHostAddress::LocalHost).toIPv4Address());

        if (!ip_list.contains(host.toIPv4Address()) &&
                !m_got_list.contains(host.toIPv4Address())
                && negotiation_string == m_negotiation_string)
        {
            m_got_list.append(host.toIPv4Address());
            emit peerFound(host, id);
        }
    }
}
