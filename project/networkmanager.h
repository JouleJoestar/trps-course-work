#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QMap>
#include <QString> // <-- Добавлены инклюды
#include <QByteArray>

class QUdpSocket;
class QTimer;
class QTcpServer;
class QTcpSocket;

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    // ИЗМЕНЕНИЕ: Принимаем publicKey
    explicit NetworkManager(const QString &currentUserLogin, const QString& publicKey, QObject *parent = nullptr);
    // ИЗМЕНЕНИЕ: Отправляем QByteArray
    void sendMessage(const QString &receiverLogin, const QByteArray &encryptedMessage);

    // ИЗМЕНЕНИЕ: Новый метод для получения ключа
    QString getPublicKeyForUser(const QString& login);

signals:
    void userListUpdated(const QStringList &users);
    // ИЗМЕНЕНИЕ: Отправляем QByteArray
    void messageReceived(const QString &senderLogin, const QByteArray &encryptedMessage);

private slots:
    void sendBroadcast();
    void processPendingDatagrams();
    void onNewTcpConnection();

private:
    QString m_currentUserLogin;
    QString m_publicKey; // Наш публичный ключ

    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    QTcpServer *tcpServer;

    QMap<QString, QHostAddress> m_discoveredUsers;
    QMap<QString, QString> m_userKeys; // Хранилище публичных ключей

    static const quint16 broadcastPort = 45454;
    static const quint16 tcpPort = 45455;
};

#endif // NETWORKMANAGER_H
