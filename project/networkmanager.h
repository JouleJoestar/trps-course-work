#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QMap>
<<<<<<< Updated upstream
=======
#include <QString>
#include <QByteArray>
>>>>>>> Stashed changes

class QUdpSocket;
class QTimer;
class QTcpServer;
class QTcpSocket;

class NetworkManager : public QObject
{
    Q_OBJECT
public:
<<<<<<< Updated upstream
    explicit NetworkManager(const QString &currentUserLogin, QObject *parent = nullptr);
    void sendMessage(const QString &receiverLogin, const QString &message);

signals:
    void userListUpdated(const QStringList &users);
    void messageReceived(const QString &senderLogin, const QString &message);
=======
    explicit NetworkManager(const QString &currentUserLogin, const QString& publicKey, QObject *parent = nullptr);

    void sendMessage(const QString &receiverLogin, const QByteArray &encryptedMessage);
    void sendBroadcastMessage(const QString& message);

    QString getPublicKeyForUser(const QString& login);

signals:
    void userListUpdated(const QStringList &users);
    void messageReceived(const QString &senderLogin, const QByteArray &encryptedMessage);
    void broadcastMessageReceived(const QString& senderLogin, const QString& message);
>>>>>>> Stashed changes

private slots:
    void sendBroadcast();
    void processPendingDatagrams();
    void onNewTcpConnection();

private:
    QString m_currentUserLogin;
<<<<<<< Updated upstream
=======
    QString m_publicKey;
>>>>>>> Stashed changes

    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    QTcpServer *tcpServer;

    QMap<QString, QHostAddress> m_discoveredUsers;
<<<<<<< Updated upstream
=======
    QMap<QString, QString> m_userKeys;
>>>>>>> Stashed changes

    static const quint16 broadcastPort = 45454;
    static const quint16 tcpPort = 45455;
};

#endif // NETWORKMANAGER_H
