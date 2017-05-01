#ifndef SECUREBYTEARRAY_H
#define SECUREBYTEARRAY_H

#include <QByteArray>

class SecureByteArray : public QByteArray
{
public:
    SecureByteArray();
    SecureByteArray(const char *data, int size);
    SecureByteArray(int size, char ch);
    SecureByteArray(const QByteArray &other);
    SecureByteArray(QByteArray &&other);

    ~SecureByteArray();
};

#endif // SECUREBYTEARRAY_H
