#include "securebytearray.h"

SecureByteArray::SecureByteArray() : QByteArray()
{

}

SecureByteArray::SecureByteArray(const char *data, int size) : QByteArray(data, size)
{

}

SecureByteArray::SecureByteArray(int size, char ch) : QByteArray(size, ch)
{

}

SecureByteArray::SecureByteArray(const QByteArray &other) : QByteArray(other)
{

}

SecureByteArray::SecureByteArray(QByteArray &&other) : QByteArray(other)
{

}

SecureByteArray::~SecureByteArray()
{
    fill((char)0);
}
