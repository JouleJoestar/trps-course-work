#include "database.h"

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

    if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "sender_login TEXT NOT NULL,"
                    "receiver_login TEXT NOT NULL,"
                    "content TEXT NOT NULL,"
                    "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                    ")")) {
        qWarning() << "Failed to create 'messages' table:" << query.lastError().text();
        return false;
    }

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
        return false;
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

bool Database::addMessage(const QString &senderLogin, const QString &receiverLogin, const QString &content)
{
    if (!db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO messages (sender_login, receiver_login, content) "
                  "VALUES (:sender_login, :receiver_login, :content)");
    query.bindValue(":sender_login", senderLogin);
    query.bindValue(":receiver_login", receiverLogin);
    query.bindValue(":content", content);

    if (!query.exec()) {
        qWarning() << "Failed to add message:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<Message> Database::getMessages(const QString &user1Login, const QString &user2Login)
{
    QList<Message> messages;
    if (!db.isOpen()) return messages;

    QSqlQuery query;
    query.prepare("SELECT sender_login, content, timestamp "
                  "FROM messages "
                  "WHERE (sender_login = :user1 AND receiver_login = :user2) "
                  "   OR (sender_login = :user2 AND receiver_login = :user1) "
                  "ORDER BY timestamp ASC");
    query.bindValue(":user1", user1Login);
    query.bindValue(":user2", user2Login);

    if (!query.exec()) {
        qWarning() << "Failed to get messages:" << query.lastError().text();
        return messages;
    }

    while (query.next()) {
        Message msg;
        msg.senderLogin = query.value(0).toString();
        msg.content = query.value(1).toString();
        msg.timestamp = query.value(2).toDateTime();
        messages.append(msg);
    }

    return messages;
}

QString Database::getPublicKey(const QString &login)
{
    if (!db.isOpen()) return {};
    QSqlQuery query;
    query.prepare("SELECT public_key FROM users WHERE login = :login");
    query.bindValue(":login", login);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

QString Database::getEncryptedPrivateKey(const QString &login)
{
    if (!db.isOpen()) return {};
    QSqlQuery query;
    query.prepare("SELECT private_key_encrypted FROM users WHERE login = :login");
    query.bindValue(":login", login);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

QStringList Database::getAllChatPartners(const QString &currentUserLogin)
{
    QStringList partners;
    if (!db.isOpen()) return partners;

    QSqlQuery query;
    query.prepare("SELECT DISTINCT receiver_login FROM messages WHERE sender_login = :user "
                  "UNION "
                  "SELECT DISTINCT sender_login FROM messages WHERE receiver_login = :user");
    query.bindValue(":user", currentUserLogin);

    if (!query.exec()) {
        qWarning() << "Failed to get all chat partners:" << query.lastError().text();
        return partners;
    }

    while (query.next()) {
        partners.append(query.value(0).toString());
    }
    return partners;
}
