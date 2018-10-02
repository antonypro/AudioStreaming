#ifndef SQL_H
#define SQL_H

#include <QtCore>
#include <QtSql>
#include "openssllib.h"
#include "common.h"

class Sql : public QObject
{
    Q_OBJECT
public:
    explicit Sql(QObject *parent = nullptr);

signals:

public slots:
    bool open();
    bool createUser(const QString &user, const QString &password);
    bool loginUser(const QString &user, const QString &password);
    bool userExists(const QString &user);

private:
    QSqlDatabase m_db;
    QSqlQuery m_query;
};

#endif // SQL_H
