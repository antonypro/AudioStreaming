#include "tcpserver.h"

TcpServer::TcpServer(QObject *parent) : QTcpServer(parent)
{
    qRegisterMetaType<qintptr>("qintptr");
}

void TcpServer::incomingConnection(qintptr handle)
{
    emit serverIncomingConnection(handle);
}
