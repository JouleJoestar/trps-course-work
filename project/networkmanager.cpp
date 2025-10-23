#include "networkmanager.h"
#include <QUdpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDebug>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QTcpServer>
#include <QTcpSocket>

NetworkManager::NetworkManager(const QString &currentUserLogin, QObject *parent)
    : QObject(parent), m_currentUserLogin(currentUserLogin)
{
    udpSocket = new QUdpSocket(this);

    if (!udpSocket->bind(QHostAddress::AnyIPv4, broadcastPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "UDP Socket could not bind:" << udpSocket->errorString();
    } else {
        qDebug() << "UDP Socket is listening on port" << broadcastPort;
    }

    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::processPendingDatagrams);

    broadcastTimer = new QTimer(this);
    connect(broadcastTimer, &QTimer::timeout, this, &NetworkManager::sendBroadcast);
    broadcastTimer->start(5000);
    sendBroadcast();
    // ----------------------

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qWarning() << "TCP Server could not start on port" << tcpPort;
    } else {
        qDebug() << "TCP Server started, listening on port" << tcpPort;
    }
    connect(tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewTcpConnection);

    qDebug() << "NetworkManager initialized for user" << m_currentUserLogin;
}

void NetworkManager::sendMessage(const QString &receiverLogin, const QString &message)
{
    if (!m_discoveredUsers.contains(receiverLogin)) {
        qWarning() << "Cannot send message: user" << receiverLogin << "not found.";
        return;
    }

    QTcpSocket *socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

    QHostAddress receiverAddress = m_discoveredUsers.value(receiverLogin);
    socket->connectToHost(receiverAddress, tcpPort);

    if (socket->waitForConnected(3000)) {
        qDebug() << "Connected to" << receiverLogin << "to send a message.";
        QByteArray data = (m_currentUserLogin + ":" + message).toUtf8();
        socket->write(data);
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->disconnectFromHost();
        qDebug() << "Message sent.";
    } else {
        qWarning() << "Could not connect to" << receiverLogin << ":" << socket->errorString();
    }
}

void NetworkManager::onNewTcpConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    if (!clientSocket) return;

    qDebug() << "New TCP connection from" << clientSocket->peerAddress().toString();

    if (clientSocket->waitForReadyRead(1000)) {
        QByteArray data = clientSocket->readAll();
        QString rawData = QString::fromUtf8(data);
        qDebug() << "Received TCP data:" << rawData;

        int separatorIndex = rawData.indexOf(':');
        if (separatorIndex != -1) {
            QString senderLogin = rawData.left(separatorIndex);
            QString message = rawData.mid(separatorIndex + 1);
            emit messageReceived(senderLogin, message);
        }
    }

    clientSocket->disconnectFromHost();
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
}

void NetworkManager::sendBroadcast()
{
    QByteArray datagram = "DISCOVER:" + m_currentUserLogin.toUtf8();
    qDebug() << "Sending broadcast:" << datagram;
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, broadcastPort);
}

<<<<<<< Updated upstream
// В файле networkmanager.cpp

=======
void NetworkManager::sendBroadcastMessage(const QString &message)
{
    // Формируем пакет специального формата: "MSG_ALL:логин_отправителя:текст_сообщения"
    QByteArray datagram = "MSG_ALL:" + m_currentUserLogin.toUtf8() + ":" + message.toUtf8();

    // Используем нашу надежную функцию рассылки по всем интерфейсам
    sendBroadcastDatagram(datagram);
}

// --- ИЗМЕНЯЕМ sendBroadcast ---
// Мы вынесем логику рассылки в отдельную функцию, чтобы не дублировать код
void NetworkManager::sendBroadcastDatagram(const QByteArray &datagram)
{
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, broadcastPort);

    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if ((interface.flags() & QNetworkInterface::IsUp) && (interface.flags() & QNetworkInterface::CanBroadcast)) {
            for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    udpSocket->writeDatagram(datagram, entry.broadcast(), broadcastPort);
                }
            }
        }
    }
}

void NetworkManager::sendBroadcast()
{
    QByteArray datagram = "DISCOVER:" + m_currentUserLogin.toUtf8() + ":" + m_publicKey.toUtf8();
    sendBroadcastDatagram(datagram); // Теперь просто вызываем новую функцию
}


// --- ОБНОВЛЯЕМ processPendingDatagrams ---
>>>>>>> Stashed changes
void NetworkManager::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress senderAddress = datagram.senderAddress();

        // Мы больше не будем здесь выводить [RAW], чтобы не засорять лог.

        if (data.startsWith("DISCOVER:")) {
<<<<<<< Updated upstream
            QString discoveredUserLogin = QString::fromUtf8(data.mid(9));
            if (discoveredUserLogin == m_currentUserLogin)
                continue;

            QHostAddress cleanAddress(senderAddress.toIPv4Address());

            // --- НАЧАЛО НОВОЙ ЛОГИКИ "ОТВЕТНОГО ПАКЕТА" ---

            // Если мы впервые видим этого пользователя,
            // отправляем ему свой "DISCOVER" пакет в ответ напрямую.
            if (!m_discoveredUsers.contains(discoveredUserLogin)) {
                qDebug() << "Discovered NEW user:" << discoveredUserLogin << "at" << cleanAddress.toString();
                qDebug() << "Sending direct discovery response to" << cleanAddress.toString();

                QByteArray responseDatagram = "DISCOVER:" + m_currentUserLogin.toUtf8();
                udpSocket->writeDatagram(responseDatagram, cleanAddress, broadcastPort);
            }

            // Теперь обычная логика добавления/обновления
            if (!m_discoveredUsers.contains(discoveredUserLogin) || m_discoveredUsers.value(discoveredUserLogin) != cleanAddress) {
                m_discoveredUsers[discoveredUserLogin] = cleanAddress;
                emit userListUpdated(m_discoveredUsers.keys());
            }
            // --- КОНЕЦ НОВОЙ ЛОГИКИ ---
=======
            // ... (вся логика обнаружения остается без изменений) ...

        } else if (data.startsWith("MSG_ALL:")) { // <-- ДОБАВЛЕНА НОВАЯ ПРОВЕРКА
            // Разбираем пакет "MSG_ALL:логин:сообщение"
            QList<QByteArray> parts = data.mid(8).split(':');
            if (parts.size() < 2) continue;

            QString senderLogin = QString::fromUtf8(parts.takeFirst());
            // Игнорируем наши собственные сообщения, которые мы получаем по "эху"
            if (senderLogin == m_currentUserLogin) continue;

            QString message = QString::fromUtf8(parts.join(':'));

            qDebug() << "Received broadcast message from" << senderLogin << ":" << message;
            // Отправляем сигнал в главное окно
            emit broadcastMessageReceived(senderLogin, message);
>>>>>>> Stashed changes
        }
    }
}
