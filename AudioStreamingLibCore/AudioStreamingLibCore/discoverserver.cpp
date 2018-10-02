#include "discoverserver.h"

DiscoverServer::DiscoverServer(QObject *parent) : QObject(parent)
{
    m_server = nullptr;
}

DiscoverServer::~DiscoverServer()
{

}

void DiscoverServer::listen(quint16 port, const QByteArray &negotiation_string, const QString &id)
{
    m_server = new QUdpSocket(this);

    m_negotiation_string = negotiation_string.leftJustified(128, char(0), true);
    m_id = id;

    if (!m_server->bind(QHostAddress::AnyIPv4, port))
    {
        emit error(m_server->errorString());
        return;
    }

    connect(m_server, &QUdpSocket::readyRead, this, &DiscoverServer::readyRead);
}

void DiscoverServer::readyRead()
{
    while (m_server->hasPendingDatagrams())
    {
        QByteArray data;
        data.resize(int(m_server->pendingDatagramSize()));

        QHostAddress host;
        quint16 port;

        m_server->readDatagram(data.data(), data.size(), &host, &port);

        if (data == m_negotiation_string)
        {
            QByteArray data;
            data.append(m_negotiation_string);
            data.append(m_id.toLatin1());

            m_server->writeDatagram(data, host, port);
        }
    }
}
