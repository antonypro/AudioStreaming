#include "sql.h"

Sql::Sql(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("../data/database.db");
}

bool Sql::open()
{
    if (!m_db.open())
    {
        DEBUG_FUNCTION(qPrintable(m_db.lastError().text()));
        m_db.close();
        return false;
    }

    m_query = QSqlQuery(m_db);

    if (!m_query.exec("select 1 from sqlite_master where type='table' and name='users';"))
        return false;

    if (!m_query.next())
    {
        bool result = m_query.exec("create table users (user text primary key, password text);");

        if (!result)
        {
            DEBUG_FUNCTION("Can't create table in database!");
            m_db.close();
            return false;
        }
    }

    return true;
}

bool Sql::createUser(const QString &user, const QString &password)
{
    if (user.size() < 3 || password.size() < 6)
        return false;

    if (user.size() > 20 || password.size() > 128)
        return false;

    QString user_db = user;

    m_query.prepare("select 1 from users where user = ?;");
    m_query.bindValue(0, user_db);

    if (!m_query.exec())
        return false;

    if (m_query.next())
        return false;

    QByteArray password_hash;

    QByteArray salt = OpenSslLib::RANDbytes(8);

    password_hash = OpenSslLib::SHA256(password.toLatin1(), salt);

    password_hash.prepend(salt);

    m_query.prepare("insert into users values(?, ?);");
    m_query.bindValue(0, QString(user_db));
    m_query.bindValue(1, QString(password_hash.toHex()));

    if (!m_query.exec())
        return false;

    return true;
}

bool Sql::loginUser(const QString &user, const QString &password)
{
    if (user.size() < 3 || password.size() < 6)
        return false;

    if (user.size() > 20 || password.size() > 128)
        return false;

    QString user_db = user;

    m_query.prepare("select password from users where user = ?;");
    m_query.bindValue(0, user_db);

    if (!m_query.exec())
        return false;

    if (!m_query.next())
        return false;

    QByteArray password_db = QByteArray::fromHex(m_query.value(0).toString().toLatin1());

    QByteArray password_hash;

    QByteArray salt = password_db.mid(0, 8);

    password_db.remove(0, 8);

    password_hash = OpenSslLib::SHA256(password.toLatin1(), salt);

    if (password_db != password_hash)
        return false;

    return true;
}

bool Sql::userExists(const QString &user)
{
    if (user.size() < 3)
        return false;

    if (user.size() > 20)
        return false;

    QString user_db = user;

    m_query.prepare("select 1 from users where user = ?;");
    m_query.bindValue(0, user_db);

    return (m_query.exec() && m_query.next());
}

bool Sql::deleteUser(const QString &user)
{
    if (user.size() < 3)
        return false;

    if (user.size() > 20)
        return false;

    QString user_db = user;

    m_query.prepare("delete from users where user = ?;");
    m_query.bindValue(0, user_db);

    if (!m_query.exec())
        return false;

    return true;
}

bool Sql::changePassword(const QString &user, const QString &password)
{
    if (user.size() < 3 || password.size() < 6)
        return false;

    if (user.size() > 20 || password.size() > 128)
        return false;

    QString user_db = user;

    QByteArray password_hash;

    QByteArray salt = OpenSslLib::RANDbytes(8);

    password_hash = OpenSslLib::SHA256(password.toLatin1(), salt);

    password_hash.prepend(salt);

    m_query.prepare("update users set password = ? where user = ?;");
    m_query.bindValue(0, QString(password_hash.toHex()));
    m_query.bindValue(1, QString(user_db));

    if (!m_query.exec())
        return false;

    return true;
}
