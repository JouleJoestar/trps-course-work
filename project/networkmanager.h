#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QMap>

class QUdpSocket;
class QTimer;
class QTcpServer;
class QTcpSocket;

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(const QString &currentUserLogin, QObject *parent = nullptr);
    void sendMessage(const QString &receiverLogin, const QString &message);

signals:
    void userListUpdated(const QStringList &users);
    void messageReceived(const QString &senderLogin, const QString &message);

private slots:
    void sendBroadcast();
    void processPendingDatagrams();
    void onNewTcpConnection();

private:
    QString m_currentUserLogin;

    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    QTcpServer *tcpServer;

    QMap<QString, QHostAddress> m_discoveredUsers;

    static const quint16 broadcastPort = 45454;
    static const quint16 tcpPort = 45455;
};

#endif // NETWORKMANAGER_H
