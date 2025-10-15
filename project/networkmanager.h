#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QMap>

// Предварительные объявления для классов Qt, чтобы не делать тяжелых #include
class QUdpSocket;
class QTimer;
class QTcpServer;
class QTcpSocket;

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(const QString &currentUserLogin, QObject *parent = nullptr);

    // --> ДОБАВЬТЕ ЭТУ СТРОКУ
    // Объявляет публичный метод для отправки сообщений, который вызывается из MainWindow
    void sendMessage(const QString &receiverLogin, const QString &message);

signals:
    void userListUpdated(const QStringList &users);

    // --> ДОБАВЬТЕ ЭТУ СТРОКУ
    // Объявляет сигнал, который будет отправляться при получении нового сообщения
    void messageReceived(const QString &senderLogin, const QString &message);

private slots:
    // Слоты для внутренней работы класса
    void sendBroadcast();
    void processPendingDatagrams();

    // --> ДОБАВЬТЕ ЭТУ СТРОКУ
    // Слот для обработки новых TCP-подключений
    void onNewTcpConnection();

private:
    QString m_currentUserLogin;

    // Объекты для работы с сетью
    QUdpSocket *udpSocket;
    QTimer *broadcastTimer;
    QTcpServer *tcpServer; // <-- Убедитесь, что этот указатель объявлен

    QMap<QString, QHostAddress> m_discoveredUsers;

    // Константы для портов
    static const quint16 broadcastPort = 45454;
    static const quint16 tcpPort = 45455; // <-- Убедитесь, что эта константа объявлена
};

#endif // NETWORKMANAGER_H
