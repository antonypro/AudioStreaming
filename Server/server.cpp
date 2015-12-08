#include "server.h"

Server::Server(quint16 port, QObject *parent) : QObject(parent)
{
    server = new QTcpServer(this);
    connect(server, SIGNAL(newConnection()), SLOT(newConnection()));
    server->listen(QHostAddress::Any, port);
}

void Server::setHeader(QByteArray data)
{
    header = data;
}

void Server::newConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
    socket->write(header);
    sockets.append(socket);
}

void Server::disconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    sockets.removeAll(socket);
    socket->deleteLater();
}

void Server::writeData(QByteArray data)
{
    foreach (QTcpSocket *socket, sockets)
        socket->write(data);
}
