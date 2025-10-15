#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    bool connect();
    bool userExists(const QString &login);
    bool addUser(const QString &login, const QString &passwordHash, const QString &publicKey, const QString &encryptedPrivateKey);
    bool checkCredentials(const QString &login, const QString &passwordHash);

    qint64 getUserId(const QString &login);
    bool addMessage(qint64 senderId, qint64 receiverId, const QString &content);
    QList<QPair<QString, QString>> getMessages(qint64 user1Id, qint64 user2Id);

private:
    bool initDb();
    QSqlDatabase db;
};

#endif // DATABASE_H
