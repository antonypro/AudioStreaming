#include "server.h"

Server::Server(QObject *parent) : QObject(parent)
{

}

void Server::start(quint16 port)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::newConnection);
    bool listening = server->listen(QHostAddress::Any, port);

    if (!listening)
        emit error(server->errorString());
}

void Server::setHeader(const QByteArray &data)
{
    header = data;
}

void Server::newConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();
    connect(socket, &QTcpSocket::disconnected, this, &Server::disconnected);
    socket->write(header);
    sockets.append(socket);
}

void Server::disconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    sockets.removeAll(socket);
    socket->deleteLater();
}

void Server::writeData(const QByteArray &data)
{
    foreach (QTcpSocket *socket, sockets)
    {
#ifdef OPUS
        socket->write(IntToArray(data.size()));
#endif
        socket->write(data);
        socket->flush();
    }
}

QByteArray Server::IntToArray(qint32 value)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << value;
    return data;
}
