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

qint64 Database::getUserId(const QString &login)
{
    if (!db.isOpen()) return -1;

    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE login = :login");
    query.bindValue(":login", login);

    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return -1; // Возвращаем -1, если пользователь не найден
}

bool Database::addMessage(qint64 senderId, qint64 receiverId, const QString &content)
{
    if (!db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO messages (sender_id, receiver_id, content) "
                  "VALUES (:sender_id, :receiver_id, :content)");
    query.bindValue(":sender_id", senderId);
    query.bindValue(":receiver_id", receiverId);
    query.bindValue(":content", content); // В будущем здесь будет зашифрованный текст

    if (!query.exec()) {
        qWarning() << "Failed to add message:" << query.lastError().text();
        return false;
    }
    return true;
}

// Загружает все сообщения между двумя пользователями
QList<QPair<QString, QString>> Database::getMessages(qint64 user1Id, qint64 user2Id)
{
    QList<QPair<QString, QString>> messages;
    if (!db.isOpen()) return messages;

    QSqlQuery query;
    // Сложный запрос, который выбирает сообщения, где user1 - отправитель, а user2 - получатель,
    // ИЛИ наоборот. Также он сразу подтягивает логин отправителя через JOIN.
    query.prepare("SELECT u.login, m.content "
                  "FROM messages m "
                  "JOIN users u ON m.sender_id = u.id "
                  "WHERE (m.sender_id = :user1 AND m.receiver_id = :user2) "
                  "   OR (m.sender_id = :user2 AND m.receiver_id = :user1) "
                  "ORDER BY m.timestamp ASC");
    query.bindValue(":user1", user1Id);
    query.bindValue(":user2", user2Id);

    if (!query.exec()) {
        qWarning() << "Failed to get messages:" << query.lastError().text();
        return messages;
    }

    while (query.next()) {
        QString senderLogin = query.value(0).toString();
        QString content = query.value(1).toString();
        messages.append(qMakePair(senderLogin, content));
    }

    return messages;
}
