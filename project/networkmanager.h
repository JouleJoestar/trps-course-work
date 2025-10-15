#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QMap>

// Предварительные объявления, чтобы не подключать тяжелые заголовки здесь
class QUdpSocket;
class QTimer;

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(const QString &currentUserLogin, QObject *parent = nullptr);

signals:
    // Сигнал, который отправляет обновленный список имен пользователей в главное окно
    void userListUpdated(const QStringList &users);

private slots:
    void sendBroadcast(); // Слот для отправки "приветствия" по таймеру
    void processPendingDatagrams(); // Слот для обработки полученных "приветствий"

private:
    QString m_currentUserLogin;
    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    // Храним карту "имя пользователя -> его IP адрес"
    QMap<QString, QHostAddress> m_discoveredUsers;
    static const quint16 broadcastPort = 45454; // Выбираем порт для общения
};

#endif // NETWORKMANAGER_H
