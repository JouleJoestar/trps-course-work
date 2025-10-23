#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QMap>
#include <QString>
#include <QByteArray>
#include <QDateTime>

class QUdpSocket;
class QTimer;
class QTcpServer;
class QTcpSocket;

struct UserInfo {
    QHostAddress ipAddress;
    QDateTime lastSeen;
};

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(const QString &currentUserLogin, const QString& publicKey, QObject *parent = nullptr);
    void sendMessage(const QString &receiverLogin, const QByteArray &encryptedMessage);
    void sendBroadcastMessage(const QString& message);
    QString getPublicKeyForUser(const QString& login);
    void sendBroadcastDatagram(const QByteArray &datagram);
    void checkInactiveUsers();

signals:
    void userListUpdated(const QStringList &users);
    void messageReceived(const QString &senderLogin, const QByteArray &encryptedMessage);
    void broadcastMessageReceived(const QString& senderLogin, const QString& message);

private slots:
    void sendBroadcast();
    void processPendingDatagrams();
    void onNewTcpConnection();

private:
    QString m_currentUserLogin;
    QString m_publicKey;

    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    QTimer *cleanupTimer;
    QTcpServer *tcpServer;

    QMap<QString, UserInfo> m_discoveredUsers;
    QMap<QString, QString> m_userKeys;

    static const quint16 broadcastPort = 45454;
    static const quint16 tcpPort = 45455;
};

#endif // NETWORKMANAGER_H
