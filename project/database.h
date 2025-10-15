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
    // ИЗМЕНЕНО: Сигнатура функции обновлена
    bool addUser(const QString &login, const QString &passwordHash, const QString &publicKey, const QString &encryptedPrivateKey);
    bool checkCredentials(const QString &login, const QString &passwordHash);

private:
    // ДОБАВЛЕНО: Объявление функции initDb()
    bool initDb();
    QSqlDatabase db;
};

#endif // DATABASE_H
