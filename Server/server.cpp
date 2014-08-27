#include "server.h"

Server::Server(quint16 port, QObject *parent) : QObject(parent)
{
    socket = 0;
    server = new QTcpServer(this);
    connect(server, SIGNAL(newConnection()), SLOT(newConnection()));
    server->listen(QHostAddress::Any, port);
}

void Server::newConnection()
{
    socket = server->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    connect(socket, SIGNAL(destroyed()), SLOT(zeropointer()));
}

void Server::zeropointer()
{
    socket = 0;
}

void Server::writeData(QByteArray data)
{
    if (socket)
        socket->write(data);
}
