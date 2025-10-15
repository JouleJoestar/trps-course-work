#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSqlDriver>
Database::Database(QObject *parent) : QObject(parent)
{
}
Database::~Database()
{
    if (db.isOpen()) {
        db.close();
    }
}
bool Database::initDb()
{
    QSqlQuery query;

    // Включаем поддержку внешних ключей для SQLite
    if (db.driver()->hasFeature(QSqlDriver::Transactions)) {
        db.transaction();
    }
    if (!query.exec("PRAGMA foreign_keys = ON;")) {
        qWarning() << "Failed to enable foreign keys:" << query.lastError().text();
    }
    if (db.driver()->hasFeature(QSqlDriver::Transactions)) {
        if (!db.commit()) {
            qWarning() << "Failed to commit foreign key pragma:" << db.lastError().text();
        }
    }


    // Таблица пользователей
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "login TEXT UNIQUE NOT NULL,"
                    "password_hash TEXT NOT NULL,"
                    "public_key TEXT NOT NULL,"
                    "private_key_encrypted TEXT NOT NULL"
                    ")")) {
        qWarning() << "Failed to create 'users' table:" << query.lastError().text();
        return false;
    }

    // Таблица сообщений
    if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "sender_id INTEGER NOT NULL,"
                    "receiver_id INTEGER NOT NULL,"
                    "content TEXT NOT NULL,"
                    "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "FOREIGN KEY(sender_id) REFERENCES users(id),"
                    "FOREIGN KEY(receiver_id) REFERENCES users(id)"
                    ")")) {
        qWarning() << "Failed to create 'messages' table:" << query.lastError().text();
        return false;
    }

    // Таблица контактов
    if (!query.exec("CREATE TABLE IF NOT EXISTS contacts ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "owner_id INTEGER NOT NULL,"
                    "contact_id INTEGER NOT NULL,"
                    "FOREIGN KEY(owner_id) REFERENCES users(id),"
                    "FOREIGN KEY(contact_id) REFERENCES users(id),"
                    "UNIQUE(owner_id, contact_id)"
                    ")")) {
        qWarning() << "Failed to create 'contacts' table:" << query.lastError().text();
        return false;
    }

    return true;
}


bool Database::connect()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("messenger.db");

    if (!db.open()) {
        qWarning() << "Database connection error:" << db.lastError().text();
        return false;
    }

    // Вызываем инициализацию таблиц при подключении
    return initDb();
}

bool Database::userExists(const QString &login)
{
    QSqlQuery query;
    query.prepare("SELECT login FROM users WHERE login = :login");
    query.bindValue(":login", login);
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}
bool Database::addUser(const QString &login, const QString &passwordHash, const QString &publicKey, const QString &encryptedPrivateKey)
{
    if (userExists(login)) {
        return false; // Пользователь уже существует
    }

    QSqlQuery query;
    query.prepare("INSERT INTO users (login, password_hash, public_key, private_key_encrypted) "
                  "VALUES (:login, :password_hash, :public_key, :private_key_encrypted)");
    query.bindValue(":login", login);
    query.bindValue(":password_hash", passwordHash);
    query.bindValue(":public_key", publicKey);
    query.bindValue(":private_key_encrypted", encryptedPrivateKey);

    if (!query.exec()) {
        qWarning() << "Failed to add user:" << query.lastError().text();
        return false;
    }
    return true;
}

// Обновленный метод checkCredentials
bool Database::checkCredentials(const QString &login, const QString &passwordHash)
{
    QSqlQuery query;
    query.prepare("SELECT password_hash FROM users WHERE login = :login");
    query.bindValue(":login", login);

    if (query.exec() && query.next()) {
        return query.value(0).toString() == passwordHash;
    }
    return false;
}
