#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSqlDriver>

struct Message {
    QString senderLogin;
    QString content;
    QDateTime timestamp;
};

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
    QStringList getAllChatPartners(const QString& currentUserLogin);
    bool addMessage(const QString &senderLogin, const QString &receiverLogin, const QString &content);
    QList<Message> getMessages(const QString &user1Login, const QString &user2Login);

    QString getPublicKey(const QString &login);
    QString getEncryptedPrivateKey(const QString &login);

private:
    bool initDb();
    QSqlDatabase db;
};

#endif // DATABASE_H
