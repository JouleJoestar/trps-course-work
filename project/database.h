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

    bool addMessage(const QString &senderLogin, const QString &receiverLogin, const QString &content);
    QList<QPair<QString, QString>> getMessages(const QString &user1Login, const QString &user2Login);

    QString getPublicKey(const QString &login);
    QString getEncryptedPrivateKey(const QString &login);

private:
    bool initDb();
    QSqlDatabase db;
};

#endif // DATABASE_H
